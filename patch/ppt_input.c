/*
 * Pocket Pro Tracker controller patch v0.2.8 / PT2 clone 1.92
 * Add-on module, does not replace the original renderer, audio engine or file format.
 * BSD-3-Clause, 2026. Upstream 8bitbubsy BSD-3-Clause credits preserved.
 * Raw key mapping confirmed on one RG35XX Pro with muOS (controls.json).
 * DEVICE TEST REQUIRED: the analog axes, display backend, audio and editing semantics
 * still have to be validated on real hardware.
 */
#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <limits.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include "pt2_mouse.h"
#include "pt2_header.h"
#include "pt2_structs.h"
#include "pt2_visuals.h"
#include "pt2_replayer.h"
#include "pt2_helpers.h"
#include "ppt_input.h"

#define DEVICES 16
#define INITIAL_DELAY 280
#define REPEAT_DELAY 85
#define MOUSE_STEP 6
#define MOUSE_DEADZONE 5500
#define MOUSE_SPEED_NUM 7
#define MOUSE_SPEED_DEN 10
#define SCREEN_MOUSE_W 320
#define SCREEN_MOUSE_H 255
#define C_COUNT 11

enum {PPT_BTN_SELECT, PPT_BTN_START, PPT_BTN_L1, PPT_BTN_L2, PPT_BTN_R1, PPT_BTN_R2, PPT_BTN_A, PPT_BTN_B, PPT_BTN_X, PPT_BTN_Y, PPT_BTN_MENU};
static const int rawMap[C_COUNT] = {310, 311, 308, 314, 309, 315, 304, 305, 307, 306, 354};
static int fds[DEVICES];
static bool held[C_COUNT], dir[4], dirRepeat[4];
static uint32_t nextAt[4];
static int lastPitch=24; /* C-3, original PT2 uses C-1..B-3 */
static uint8_t lastCmd=0, lastParam=0;
/* L1/R1 act on release, so a chord never switches patterns unintentionally. */
static bool shoulderUsed[2];
/* The console can report analog sticks as ABS_X/Y or ABS_RX/RY,
   with widely differing physical ranges. Never assume +/-32767. */
static int clamp(int x,int lo,int hi);
typedef struct {int min, max, value; bool valid, seen;} axis_t;
/* muOS/RG35XX Pro: user confirmed left-stick vertical is ABS_RX (3).
   Left-stick horizontal is mapped to ABS_Z (2) pending its on-device
   verification. The old right-stick mouse axes 4/5 are deliberately ignored.
   Index by evdev axis code, not conventional Linux stick names. */
#define TRACKED_AXES 6
static axis_t axis[TRACKED_AXES];
static int mousePosX=160, mousePosY=127;
static bool virtualMouse;
static int mouseFracX, mouseFracY;
static FILE *inputLog;
static int logDirections, logAxes, logButtons, logUnmapped;
static int analogByDevice[DEVICES][TRACKED_AXES];
/* The MENU/M button may use a non-standard evdev code on muOS.
   If the firmware consumes the key before evdev we cannot receive it. */
static bool menuRawCode(unsigned code){
  return code==312 || code==354 || code==KEY_MENU || code==BTN_MODE;
}
static void diagnostic(const char *kind,int code,int value){
  if(!inputLog)return;
  int *counter=&logButtons;
  if(kind[0]=='D')counter=&logDirections;
  if(kind[0]=='A')counter=&logAxes;
  if((*counter)++>=24)return;
  fprintf(inputLog,"PPT 0.2.8 %s code=%d value=%d\n",kind,code,value);
  fflush(inputLog);
}
static int axisIndex(unsigned code){
  /* ABS_X=0, ABS_Y=1, ABS_Z=2, ABS_RX=3, ABS_RY=4, ABS_RZ=5 */
  return code < TRACKED_AXES ? (int)code : -1;
}
static int axisAmount(int i){
  if(i<0||i>=TRACKED_AXES||!axis[i].seen)return 0;
  const axis_t *a=&axis[i];
  int lo=a->valid?a->min:-32768, hi=a->valid?a->max:32767;
  if(hi<=lo)return 0;
  int center=lo+(hi-lo)/2, half=(hi-lo)/2;
  if(!half)return 0;
  int64_t normalized=((int64_t)a->value-center)*32767/half;
  return clamp((int)normalized,-32767,32767);
}
static void moveVirtualMouse(int dx,int dy){
  if(!dx&&!dy)return;
  virtualMouse=true;
  mousePosX=clamp(mousePosX+dx,0,SCREEN_MOUSE_W-1);
  mousePosY=clamp(mousePosY+dy,0,SCREEN_MOUSE_H-1);
}
static void applyVirtualMouse(void){
  if(!virtualMouse || !video.window)return;
  /* PT2 reads OS mouse coordinates before our adapter runs. Under KMSDRM,
     SDL_WarpMouseInWindow can fail to update the software pointer. Set the
     actual PT2 software mouse and its sprite, independent of SDL warping. */
  mouse.x=mousePosX; mouse.y=mousePosY;
  mouse.rawX=mouse.absX=mousePosX;
  mouse.rawY=mouse.absY=mousePosY;
  setSpritePos(SPRITE_MOUSE_POINTER,mousePosX,mousePosY);
}
static void updateAnalogMouse(void){
  /* Left stick ONLY. Axis 3 (ABS_RX) was confirmed as left up/down.
     Axis 2 (ABS_Z) is the expected adjacent horizontal axis; if it is
     different on this firmware, the new log records axes 0..5 for diagnosis.
     Fractional accumulation gives a true 30% reduction even at low speeds. */
  const int x=axisAmount(ABS_Z), y=axisAmount(ABS_RX);
  const int moveX=abs(x)>MOUSE_DEADZONE ? (x<0?-1:1)*(1+abs(x)/9000) : 0;
  const int moveY=abs(y)>MOUSE_DEADZONE ? (y<0?-1:1)*(1+abs(y)/9000) : 0;
  if(!moveX)mouseFracX=0;
  if(!moveY)mouseFracY=0;
  mouseFracX+=moveX*MOUSE_SPEED_NUM;
  mouseFracY+=moveY*MOUSE_SPEED_NUM;
  int dx=mouseFracX/MOUSE_SPEED_DEN, dy=mouseFracY/MOUSE_SPEED_DEN;
  mouseFracX-=dx*MOUSE_SPEED_DEN;
  mouseFracY-=dy*MOUSE_SPEED_DEN;
  moveVirtualMouse(dx,dy);
}
static void processHatAxis(unsigned code,int value);

static bool active;
static const uint16_t periods[36] = {
  856,808,762,720,678,640,604,570,538,508,480,453,
  428,404,381,360,339,320,302,285,269,254,240,226,
  214,202,190,180,170,160,151,143,135,127,120,113
};

static int clamp(int x,int lo,int hi){return x<lo?lo:(x>hi?hi:x);}
static int wrap(int n,int size){n%=size;return n<0?n+size:n;}
static int pitchOf(uint16_t period){
  if (!period) return lastPitch;
  int best=0,dist=INT_MAX;
  for (int i=0;i<36;i++){int d=abs((int)period-(int)periods[i]);if(d<dist){dist=d;best=i;}}
  return best;
}
static note_t *currentNote(void){
  if(!song || song->currPattern>=MAX_PATTERNS || !song->patterns[song->currPattern])return NULL;
  return &song->patterns[song->currPattern][(int)song->currRow*PAULA_VOICES + (int)cursor.channel];
}
/* Use the SAME native PT2 Paula audition path as typing a note on keyboard. */
static void changed(void){
  if(!song)return;
  song->modified=true;
  ui.updatePatternData=true;
  ui.updateCurrSampleNum=true;
  /* The upstream engine does not require replacing its own UI or rendering. */
}
static void navigate(int d){
  if(!song || ui.diskOpScreenShown || ui.samplerScreenShown || ui.editTextFlag || ui.askBoxShown)return;
  if(d==0) song->currRow=wrap((int)song->currRow-1,64);
  else if(d==1) song->currRow=wrap((int)song->currRow+1,64);
  else if(d==2) {cursor.pos=wrap((int)cursor.pos-1,24);cursor.channel=cursor.pos/6;}
  else {cursor.pos=wrap((int)cursor.pos+1,24);cursor.channel=cursor.pos/6;}
  ui.updatePatternData=true;
  updateCursorPos();
}
extern void pptPreviewInsertedNote(int8_t pitch, int8_t sampleIndex);
/* L1/R1 match PT2's PATTERN number arrows at the current song order.
   This changes the order-table pattern assignment, but never modifies any
   pattern note data. Bounds match the original PT2 buttons (00..99). */
static void changePattern(int step){
  if(!song || editor.songPlaying || ui.askBoxShown || ui.editTextFlag ||
     ui.diskOpScreenShown || ui.samplerScreenShown)return;
  if(song->currPos>=128)return;
  const int current=song->header.patternTable[song->currPos];
  const int next=clamp(current+step,0,MAX_PATTERNS-1);
  if(next==current)return;
  song->header.patternTable[song->currPos]=(uint16_t)next;
  setPattern((int16_t)next);
  ui.updateSongPattern=true;
  ui.updatePatternData=true;
  updateWindowTitle(true);
}
/* Use PT2's own sample change functions, which also update the sample info. */
static void changeActiveSample(int step){
  if(!song || ui.askBoxShown || ui.editTextFlag || ui.diskOpScreenShown)return;
  if(step<0) sampleDownButton();
  else sampleUpButton();
}
static void previewNote(const note_t *n){
  if(!n || !n->period)return;
  const int sample=(n->sample>=1 && n->sample<=31)?n->sample-1:clamp(editor.currSample,0,30);
  pptPreviewInsertedNote((int8_t)pitchOf(n->period),(int8_t)sample);
}
static void edit(int d){
  if(!song || ui.diskOpScreenShown || ui.samplerScreenShown || ui.editTextFlag || ui.askBoxShown)return;
  note_t *n=currentNote();if(!n)return;
  const int delta=(d==2)?-1:(d==3)?1:(d==0)?-12:12;
  switch(cursor.pos%6){
    case CURSOR_NOTE: {
      int next=clamp(pitchOf(n->period)+delta,0,35);
      lastPitch=next;n->period=periods[next];
      /* Pitch remembered, sample number never cached by the adapter. */
      previewNote(n);
    }break;
    case CURSOR_SAMPLE1:
    case CURSOR_SAMPLE2: {
      int step=(d<2)?16:1;
      int sign=(d==0 || d==2)?-1:1;
      const int selected=clamp(editor.currSample+1,1,31);
      const int num=clamp((n->sample?n->sample:selected)+sign*step,1,31);
      n->sample=(uint8_t)num;
      editor.currSample=(int8_t)(num-1);
      updateCurrSample();
    }break;
    case CURSOR_CMD:
      lastCmd=(uint8_t)wrap((int)n->command+((d<2)?(d==0?-4:4):(d==2?-1:1)),16);
      n->command=lastCmd;break;
    case CURSOR_PARAM1:
    case CURSOR_PARAM2:
      lastParam=(uint8_t)wrap((int)n->param+delta,256);
      n->param=lastParam;break;
  }
  changed();
}
static void insert(void){
  note_t *n=currentNote(); if(!n || ui.editTextFlag || ui.askBoxShown)return;
  switch(cursor.pos%6){
    case CURSOR_NOTE:
      if(!n->period)n->period=periods[lastPitch];
      if(!n->sample)n->sample=(uint8_t)(clamp(editor.currSample,0,30)+1);
      /* Same note and (possibly existing) sample as the inserted cell. */
      previewNote(n);
      break;
    case CURSOR_SAMPLE1:
    case CURSOR_SAMPLE2: if(!n->sample)n->sample=(uint8_t)(clamp(editor.currSample,0,30)+1);break;
    case CURSOR_CMD: n->command=lastCmd;break;
    case CURSOR_PARAM1:
    case CURSOR_PARAM2: n->param=lastParam;break;
  }
  changed();
}
static void erase(void){
  note_t *n=currentNote();if(!n)return;
  switch(cursor.pos%6){
    case CURSOR_NOTE:n->period=0;break;
    case CURSOR_SAMPLE1:
    case CURSOR_SAMPLE2:n->sample=0;break;
    case CURSOR_CMD:n->command=0;break;
    case CURSOR_PARAM1:
    case CURSOR_PARAM2:n->param=0;break;
  }
  changed();
}
/* A minimal per-field clipboard. Rectangular multi-row LSDJ selection is NOT ready yet. */
static bool clipboardSet;
static note_t clipboard;
static int clipField;
static void copyField(void){
  note_t *n=currentNote();if(!n)return;
  clipboard=*n;clipField=cursor.pos%6;clipboardSet=true;
}
static void pasteField(void){
  note_t *n=currentNote();if(!n || !clipboardSet)return;
  int f=cursor.pos%6;
  if(f==CURSOR_NOTE)n->period=clipboard.period;
  else if(f==CURSOR_SAMPLE1 || f==CURSOR_SAMPLE2)n->sample=clipboard.sample;
  else if(f==CURSOR_CMD)n->command=clipboard.command;
  else n->param=clipboard.param;
  changed();
}
/* Playback actions use the original PT2 replayer, preserving MOD semantics.
   START = PLAY PATTERN, or STOP if anything is already playing.
   SELECT (physical SHIFT)+START = PLAY SONG from current order AND ROW. */
static void startPlayback(bool songFromRow){
  if(!song || ui.askBoxShown || ui.editTextFlag)return;
  if(editor.songPlaying){
    modStop();
    editor.currMode=MODE_IDLE;
    pointerSetMode(POINTER_MODE_IDLE,DO_CARRY);
    statusAllRight();
    return;
  }
  if(songFromRow){
    editor.playMode=PLAY_MODE_NORMAL;
    modPlay(DONT_SET_PATTERN,song->currPos,(int8_t)song->currRow);
  }else{
    editor.playMode=PLAY_MODE_PATTERN;
    modPlay(song->currPattern,DONT_SET_ORDER,DONT_SET_ROW);
  }
  editor.currMode=MODE_PLAY;
  pointerSetMode(POINTER_MODE_PLAY,DO_CARRY);
  statusAllRight();
}
/* Dispatch the real Escape key to PT2's existing keyboard handler. In the
   main view this opens REALLY QUIT ?, from subviews it navigates back.
   Never force editor.programRunning=false and never bypass the confirmation. */
static bool exitChordSent, startPending, startWithShift;
static void requestEscape(void){
  if(exitChordSent)return;
  exitChordSent=true;
  SDL_Event e;memset(&e,0,sizeof(e));
  e.type=SDL_KEYDOWN;e.key.state=SDL_PRESSED;
  e.key.keysym.sym=SDLK_ESCAPE;
  e.key.keysym.scancode=SDL_SCANCODE_ESCAPE;
  SDL_PushEvent(&e);
}
/* PT2 has an independent SDL loop for its modal confirmation dialogs.
   Push standard key events so every askBox can be operated even without a mouse.
   A = ENTER (default option), B = ESCAPE (cancel), specifically while a dialog
   is open. Never insert notes or change the song behind the dialog. */
static void modalKey(SDL_Keycode key){
  SDL_Event e;memset(&e,0,sizeof(e));
  e.type=SDL_KEYDOWN;
  e.key.state=SDL_PRESSED;
  e.key.keysym.sym=key;
  SDL_PushEvent(&e);
}
static void buttonEvent(int b,bool down){
  if(b<0||b>=C_COUNT)return;
  if(held[b]==down)return;
  held[b]=down;
  if(!down)return;
  if(ui.askBoxShown){
    if(b==PPT_BTN_A){modalKey(SDLK_RETURN);return;}
    if(b==PPT_BTN_B){modalKey(SDLK_ESCAPE);return;}
    /* Keep left/right clicks (including legacy L2/R2) in native modals. */
    if(b!=PPT_BTN_R2 && b!=PPT_BTN_L2 && b!=PPT_BTN_X && b!=PPT_BTN_Y)return;
  }
  /* Physical M/MENU (raw 312 confirmed on RG35XX Pro) + L1:
     native ESC / REALLY QUIT confirmation. Keep L1+START as fallback on
     firmware that swallows MENU at system level. */
  if((b==PPT_BTN_MENU && held[PPT_BTN_L1]) ||
     (b==PPT_BTN_L1 && held[PPT_BTN_MENU])){
    shoulderUsed[0]=true;
    requestEscape();
    return;
  }
  if(b==PPT_BTN_MENU)return;
  if(b==PPT_BTN_START){
    /* Start on RELEASE. This allows a near-simultaneous L1+START chord
       to take precedence even if START was physically pressed first. */
    if(held[PPT_BTN_L1]){startPending=false;shoulderUsed[0]=true;requestEscape();}
    else {startPending=true;startWithShift=held[PPT_BTN_SELECT];}
    return;
  }
  if(b==PPT_BTN_L1 && held[PPT_BTN_START]){
    startPending=false;
    shoulderUsed[0]=true;
    requestEscape();
    return;
  }
  if(b==PPT_BTN_L1){shoulderUsed[0]=false;return;}
  if(b==PPT_BTN_R1){shoulderUsed[1]=false;return;}
  if((b==PPT_BTN_L2 || b==PPT_BTN_R2) && !ui.askBoxShown &&
     !ui.diskOpScreenShown && !ui.editTextFlag){
    changeActiveSample(b==PPT_BTN_L2?-1:1);
    return;
  }
  if(b==PPT_BTN_A){
    if(held[PPT_BTN_B]){copyField();erase();}
    else if(held[PPT_BTN_SELECT])pasteField();
    else insert();
  }else if(b==PPT_BTN_B){
    if(held[PPT_BTN_SELECT])copyField();
    else if(held[PPT_BTN_A]){copyField();erase();}
    else copyField();
  }
  /* X/Y are the reliable mouse clicks in all screens; legacy L2/R2
     clicks are kept while an askBox or disk operation screen is open. */
  const bool legacy=(ui.askBoxShown || ui.diskOpScreenShown || ui.editTextFlag);
  if(video.window && (b==PPT_BTN_X || b==PPT_BTN_Y ||
     (legacy && (b==PPT_BTN_R2 || b==PPT_BTN_L2)))){
    SDL_Event evt;memset(&evt,0,sizeof(evt));
    evt.type=SDL_MOUSEBUTTONDOWN;
    evt.button.button=(b==PPT_BTN_X || b==PPT_BTN_R2)?SDL_BUTTON_LEFT:SDL_BUTTON_RIGHT;
    evt.button.state=SDL_PRESSED;SDL_PushEvent(&evt);
  }
}
static void releaseButton(int b){
  if(b<0||b>=C_COUNT || !held[b])return;
  /* Execute START when released so L1+START cannot accidentally PLAY. */
  const bool doStart=(b==PPT_BTN_START && startPending && !exitChordSent && !held[PPT_BTN_L1] && !ui.askBoxShown);
  const bool doSong=startWithShift;
  const bool doPattern=(b==PPT_BTN_L1 || b==PPT_BTN_R1) &&
      !shoulderUsed[b==PPT_BTN_R1] && !held[PPT_BTN_START] &&
      !held[PPT_BTN_MENU] && !ui.askBoxShown;
  held[b]=false;
  if(b==PPT_BTN_START){startPending=false;exitChordSent=false;}
  if(b==PPT_BTN_L1 || b==PPT_BTN_MENU)exitChordSent=false;
  if(doStart)startPlayback(doSong);
  if(doPattern)changePattern(b==PPT_BTN_L1?-1:1);
  if(b==PPT_BTN_L1 || b==PPT_BTN_R1)shoulderUsed[b==PPT_BTN_R1]=false;
  const bool legacy=(ui.askBoxShown || ui.diskOpScreenShown || ui.editTextFlag);
  if(video.window && (b==PPT_BTN_X || b==PPT_BTN_Y ||
     (legacy && (b==PPT_BTN_R2 || b==PPT_BTN_L2)))){
    SDL_Event evt;memset(&evt,0,sizeof(evt));
    evt.type=SDL_MOUSEBUTTONUP;
    evt.button.button=(b==PPT_BTN_X || b==PPT_BTN_R2)?SDL_BUTTON_LEFT:SDL_BUTTON_RIGHT;
    evt.button.state=SDL_RELEASED;SDL_PushEvent(&evt);
  }
}
static void directionAction(int d){
  /* During askBox(), the D-pad controls the pointer, not the pattern.
     The askBox owns a separate event loop but still calls ppt_input_update(). */
  if(held[PPT_BTN_R1])shoulderUsed[1]=true;
  if(ui.askBoxShown || held[PPT_BTN_R1]){
    int dx=(d==2?-MOUSE_STEP:(d==3?MOUSE_STEP:0));
    int dy=(d==0?-MOUSE_STEP:(d==1?MOUSE_STEP:0));
    moveVirtualMouse(dx,dy);
  }else if(held[PPT_BTN_A])edit(d);
  else navigate(d);
}
static void directionEvent(int d,bool down){
  if(d<0||d>3)return;
  if(!down){dir[d]=false;dirRepeat[d]=false;return;}
  if(dir[d])return;
  dir[d]=true;dirRepeat[d]=false;
  nextAt[d]=SDL_GetTicks()+INITIAL_DELAY;
  directionAction(d);
}
static void processHatAxis(unsigned code,int value){
  diagnostic("DPAD-HAT",(int)code,value);
  if(code==ABS_HAT0X){
    directionEvent(2,value<0);
    directionEvent(3,value>0);
  }else if(code==ABS_HAT0Y){
    directionEvent(0,value<0);
    directionEvent(1,value>0);
  }
}
void ppt_input_init(void){
  memset(held,0,sizeof(held));memset(dir,0,sizeof(dir));
  memset(axis,0,sizeof(axis));
  const char *logPath=getenv("PPT_DIAG_LOG");
  if(logPath && logPath[0])inputLog=fopen(logPath,"a");
  if(inputLog){fprintf(inputLog,"PPT 0.2.8 input init\n");fflush(inputLog);}
  for(int i=0;i<DEVICES;i++){
    fds[i]=-1;
    char path[64];snprintf(path,sizeof(path),"/dev/input/event%d",i);
    fds[i]=open(path,O_RDONLY|O_NONBLOCK|O_CLOEXEC);
    if(fds[i]<0)continue;
    if(inputLog){
      char devName[128]={0};
      if(ioctl(fds[i],EVIOCGNAME(sizeof(devName)),devName)<0)
        strcpy(devName,"unknown");
      fprintf(inputLog,"open %s OK name=%s\n",path,devName);
      fflush(inputLog);
    }
    for(int a=0;a<TRACKED_AXES;a++){
      static const unsigned ids[]={ABS_X,ABS_Y,ABS_Z,ABS_RX,ABS_RY,ABS_RZ};
      struct input_absinfo info;
      if(ioctl(fds[i],EVIOCGABS(ids[a]),&info)==0 && info.maximum>info.minimum){
        axis[a].min=info.minimum;axis[a].max=info.maximum;
        axis[a].value=info.value;axis[a].valid=true;
        if(inputLog){fprintf(inputLog,"axis %u min=%d max=%d center=%d\n",ids[a],info.minimum,info.maximum,info.value);fflush(inputLog);}
      }
    }
  }
  active=true;
}
void ppt_input_close(void){
  for(int i=0;i<DEVICES;i++){if(fds[i]>=0){close(fds[i]);fds[i]=-1;}}
  active=false;
  if(inputLog){fclose(inputLog);inputLog=NULL;}
}
void ppt_input_update(void){
  if(!active)return;
  struct input_event ev;
  for(int i=0;i<DEVICES;i++){
    if(fds[i]<0)continue;
    int limit=256;
    while(limit-- && read(fds[i],&ev,sizeof(ev))==(ssize_t)sizeof(ev)){
      if(ev.type==EV_KEY){
        bool down=ev.value!=0;
        int b=-1;
        for(int j=0;j<C_COUNT;j++)if((int)ev.code==rawMap[j]){b=j;break;}
        if(b<0 && menuRawCode(ev.code))b=PPT_BTN_MENU;
        if(b>=0){diagnostic("BUTTON",(int)ev.code,ev.value);if(down)buttonEvent(b,true);else releaseButton(b);continue;}
        int d=-1;
        if(ev.code==KEY_UP || ev.code==BTN_DPAD_UP)d=0;
        else if(ev.code==KEY_DOWN || ev.code==BTN_DPAD_DOWN)d=1;
        else if(ev.code==KEY_LEFT || ev.code==BTN_DPAD_LEFT)d=2;
        else if(ev.code==KEY_RIGHT || ev.code==BTN_DPAD_RIGHT)d=3;
        if(d>=0){diagnostic("DPAD-KEY",(int)ev.code,ev.value);directionEvent(d,down);}
        else if(inputLog && logUnmapped++<60){
          fprintf(inputLog,"PPT 0.2.8 OTHER-KEY event%d code=%u value=%d L1=%d\n",
                  i,(unsigned)ev.code,(int)ev.value,(int)held[PPT_BTN_L1]);
          fflush(inputLog);
        }
      } else if(ev.type==EV_ABS){
        if(ev.code==ABS_HAT0X || ev.code==ABS_HAT0Y)processHatAxis(ev.code,(int)ev.value);
        else {
          int a=axisIndex(ev.code);
          if(a>=0){axis[a].value=ev.value;axis[a].seen=true;
            /* Limit axis samples per device so logs remain readable. */
            if(inputLog && analogByDevice[i][a]++ < 16){
              fprintf(inputLog,"PPT 0.2.7 ANALOG event%d code=%d value=%d\n",i,(int)ev.code,(int)ev.value);
              fflush(inputLog);
            }}
        }
      }
    }
  }
  uint32_t now=SDL_GetTicks();
  for(int d=0;d<4;d++){
    if(dir[d] && (int32_t)(now-nextAt[d])>=0){
      directionAction(d);
      nextAt[d]=now+REPEAT_DELAY;dirRepeat[d]=true;
    }
  }
  updateAnalogMouse();
  applyVirtualMouse();
}
