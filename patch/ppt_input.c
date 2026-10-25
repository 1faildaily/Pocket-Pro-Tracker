/*
 * Pocket Pro Tracker controller patch v0.2.1 / PT2 clone 1.92
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
#include "ppt_input.h"

#define DEVICES 16
#define INITIAL_DELAY 280
#define REPEAT_DELAY 85
#define MOUSE_STEP 6
#define MOUSE_DEADZONE 5500
#define SCREEN_MOUSE_W 320
#define SCREEN_MOUSE_H 255
#define C_COUNT 10

enum {PPT_BTN_SELECT, PPT_BTN_START, PPT_BTN_L1, PPT_BTN_L2, PPT_BTN_R1, PPT_BTN_R2, PPT_BTN_A, PPT_BTN_B, PPT_BTN_X, PPT_BTN_Y};
static const int rawMap[C_COUNT] = {310, 311, 308, 314, 309, 315, 304, 305, 307, 306};
static int fds[DEVICES];
static bool held[C_COUNT], dir[4], dirRepeat[4];
static uint32_t nextAt[4];
static int lastPitch=24; /* C-3, original PT2 uses C-1..B-3 */
static int lastSample=1; /* MOD sample number, one-based */
static uint8_t lastCmd=0, lastParam=0;
/* The console can report analog sticks as ABS_X/Y or ABS_RX/RY,
   with widely differing physical ranges. Never assume +/-32767. */
static int clamp(int x,int lo,int hi);
typedef struct {int min, max, value; bool valid, seen;} axis_t;
static axis_t axis[4]; /* RX, RY, X, Y */
static int mousePosX=160, mousePosY=127;
static bool virtualMouse;
static FILE *inputLog;
static int logDirections, logAxes, logButtons;
static void diagnostic(const char *kind,int code,int value){
  if(!inputLog)return;
  int *counter=&logButtons;
  if(kind[0]=='D')counter=&logDirections;
  if(kind[0]=='A')counter=&logAxes;
  if((*counter)++>=24)return;
  fprintf(inputLog,"PPT 0.2.1 %s code=%d value=%d\n",kind,code,value);
  fflush(inputLog);
}
static int axisIndex(unsigned code){
  if(code==ABS_RX)return 0;
  if(code==ABS_RY)return 1;
  if(code==ABS_X)return 2;
  if(code==ABS_Y)return 3;
  return -1;
}
static int axisAmount(int i){
  if(i<0||i>=4||!axis[i].seen)return 0;
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
  int x=0,y=0;
  /* Prefer right stick; use X/Y only if RX/RY never delivered events. */
  if(axis[0].seen||axis[1].seen){x=axisAmount(0);y=axisAmount(1);}
  else {x=axisAmount(2);y=axisAmount(3);}
  int dx=0,dy=0;
  if(abs(x)>MOUSE_DEADZONE)dx=(x<0?-1:1)*(1+abs(x)/9000);
  if(abs(y)>MOUSE_DEADZONE)dy=(y<0?-1:1)*(1+abs(y)/9000);
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
static void changed(void){
  if(!song)return;
  song->modified=true;
  ui.updatePatternData=true;
  ui.updateCurrSampleNum=true;
  /* The upstream engine does not require replacing its own UI or rendering. */
}
static void navigate(int d){
  if(!song || ui.diskOpScreenShown || ui.samplerScreenShown || ui.editTextFlag)return;
  if(d==0) song->currRow=wrap((int)song->currRow-1,64);
  else if(d==1) song->currRow=wrap((int)song->currRow+1,64);
  else if(d==2) {cursor.pos=wrap((int)cursor.pos-1,24);cursor.channel=cursor.pos/6;}
  else {cursor.pos=wrap((int)cursor.pos+1,24);cursor.channel=cursor.pos/6;}
  ui.updatePatternData=true;
  updateCursorPos();
}
static void edit(int d){
  if(!song || ui.diskOpScreenShown || ui.samplerScreenShown || ui.editTextFlag || ui.askBoxShown)return;
  note_t *n=currentNote();if(!n)return;
  const int delta=(d==2)?-1:(d==3)?1:(d==0)?-12:12;
  switch(cursor.pos%6){
    case CURSOR_NOTE: {
      int next=clamp(pitchOf(n->period)+delta,0,35);
      lastPitch=next;n->period=periods[next];
      if(n->sample==0)n->sample=(uint8_t)lastSample;
    }break;
    case CURSOR_SAMPLE1:
    case CURSOR_SAMPLE2: {
      int step=(d<2)?16:1;
      int sign=(d==0 || d==2)?-1:1;
      lastSample=clamp((n->sample?n->sample:lastSample)+sign*step,1,31);
      n->sample=(uint8_t)lastSample;
      editor.currSample=(int8_t)(lastSample-1);
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
      if(!n->sample)n->sample=(uint8_t)lastSample;
      break;
    case CURSOR_SAMPLE1:
    case CURSOR_SAMPLE2: if(!n->sample)n->sample=(uint8_t)lastSample;break;
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
static void buttonEvent(int b,bool down){
  if(b<0||b>=C_COUNT)return;
  if(held[b]==down)return;
  held[b]=down;
  if(!down)return;
  if(b==PPT_BTN_A){
    if(held[PPT_BTN_B]){copyField();erase();}
    else if(held[PPT_BTN_SELECT])pasteField();
    else insert();
  }else if(b==PPT_BTN_B){
    if(held[PPT_BTN_SELECT])copyField();
    else if(held[PPT_BTN_A]){copyField();erase();}
    else copyField();
  }else if(b==PPT_BTN_L1 && held[PPT_BTN_START]){
    editor.programRunning=false;
  }else if(b==PPT_BTN_START && held[PPT_BTN_L1]){
    editor.programRunning=false;
  }
  /* R2/L2 mouse buttons handled below as SDL events. */
  if((b==PPT_BTN_R2 || b==PPT_BTN_L2) && video.window){
    SDL_Event evt;memset(&evt,0,sizeof(evt));
    evt.type=SDL_MOUSEBUTTONDOWN;evt.button.button=(b==PPT_BTN_R2)?SDL_BUTTON_LEFT:SDL_BUTTON_RIGHT;
    evt.button.state=SDL_PRESSED; SDL_PushEvent(&evt);
  }
}
static void releaseButton(int b){
  if(b<0||b>=C_COUNT || !held[b])return;
  held[b]=false;
  if((b==PPT_BTN_R2 || b==PPT_BTN_L2) && video.window){
    SDL_Event evt;memset(&evt,0,sizeof(evt));
    evt.type=SDL_MOUSEBUTTONUP;evt.button.button=(b==PPT_BTN_R2)?SDL_BUTTON_LEFT:SDL_BUTTON_RIGHT;
    evt.button.state=SDL_RELEASED; SDL_PushEvent(&evt);
  }
}
static void directionAction(int d){
  /* R1+D-pad doubles as a reliable, no-analog-required mouse control. */
  if(held[PPT_BTN_R1]){
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
  if(inputLog){fprintf(inputLog,"PPT 0.2.1 input init\n");fflush(inputLog);}
  for(int i=0;i<DEVICES;i++){
    fds[i]=-1;
    char path[64];snprintf(path,sizeof(path),"/dev/input/event%d",i);
    fds[i]=open(path,O_RDONLY|O_NONBLOCK|O_CLOEXEC);
    if(fds[i]<0)continue;
    if(inputLog){fprintf(inputLog,"open %s OK\n",path);fflush(inputLog);}
    for(int a=0;a<4;a++){
      static const unsigned ids[]={ABS_RX,ABS_RY,ABS_X,ABS_Y};
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
        if(b>=0){diagnostic("BUTTON",(int)ev.code,ev.value);if(down)buttonEvent(b,true);else releaseButton(b);continue;}
        int d=-1;
        if(ev.code==KEY_UP || ev.code==BTN_DPAD_UP)d=0;
        else if(ev.code==KEY_DOWN || ev.code==BTN_DPAD_DOWN)d=1;
        else if(ev.code==KEY_LEFT || ev.code==BTN_DPAD_LEFT)d=2;
        else if(ev.code==KEY_RIGHT || ev.code==BTN_DPAD_RIGHT)d=3;
        if(d>=0){diagnostic("DPAD-KEY",(int)ev.code,ev.value);directionEvent(d,down);}
      } else if(ev.type==EV_ABS){
        if(ev.code==ABS_HAT0X || ev.code==ABS_HAT0Y)processHatAxis(ev.code,(int)ev.value);
        else {
          int a=axisIndex(ev.code);
          if(a>=0){axis[a].value=ev.value;axis[a].seen=true;
            diagnostic("ANALOG",(int)ev.code,(int)ev.value);}
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
