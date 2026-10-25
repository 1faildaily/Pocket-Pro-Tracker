/*
 * Pocket Pro Tracker controller prototype v0.2 / PT2 clone 1.92
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
#include "pt2_header.h"
#include "pt2_structs.h"
#include "pt2_visuals.h"
#include "pt2_replayer.h"
#include "ppt_input.h"

#define DEVICES 16
#define INITIAL_DELAY 280
#define REPEAT_DELAY 85
#define MOUSE_STEP 6
#define C_COUNT 10

enum {PPT_BTN_SELECT, PPT_BTN_START, PPT_BTN_L1, PPT_BTN_L2, PPT_BTN_R1, PPT_BTN_R2, PPT_BTN_A, PPT_BTN_B, PPT_BTN_X, PPT_BTN_Y};
static const int rawMap[C_COUNT] = {310, 311, 308, 314, 309, 315, 304, 305, 307, 306};
static int fds[DEVICES];
static bool held[C_COUNT], dir[4], dirRepeat[4];
static uint32_t nextAt[4];
static int lastPitch=24; /* C-3, original PT2 uses C-1..B-3 */
static int lastSample=1; /* MOD sample number, one-based */
static uint8_t lastCmd=0, lastParam=0;
static int axes[2];
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
static void directionEvent(int d,bool down){
  if(d<0||d>3)return;
  if(!down){dir[d]=false;dirRepeat[d]=false;return;}
  if(dir[d])return;
  dir[d]=true;dirRepeat[d]=false;
  nextAt[d]=SDL_GetTicks()+INITIAL_DELAY;
  if(held[PPT_BTN_A])edit(d);else navigate(d);
}
void ppt_input_init(void){
  memset(held,0,sizeof(held));memset(dir,0,sizeof(dir));
  for(int i=0;i<DEVICES;i++){
    fds[i]=-1;
    char path[64];snprintf(path,sizeof(path),"/dev/input/event%d",i);
    fds[i]=open(path,O_RDONLY|O_NONBLOCK|O_CLOEXEC);
  }
  active=true;
}
void ppt_input_close(void){
  for(int i=0;i<DEVICES;i++){if(fds[i]>=0){close(fds[i]);fds[i]=-1;}}
  active=false;
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
        if(b>=0){if(down)buttonEvent(b,true);else releaseButton(b);continue;}
        if(ev.code==KEY_UP)directionEvent(0,down);
        else if(ev.code==KEY_DOWN)directionEvent(1,down);
        else if(ev.code==KEY_LEFT)directionEvent(2,down);
        else if(ev.code==KEY_RIGHT)directionEvent(3,down);
      } else if(ev.type==EV_ABS){
        if(ev.code==ABS_RX)axes[0]=(int)ev.value;
        else if(ev.code==ABS_RY)axes[1]=(int)ev.value;
      }
    }
  }
  uint32_t now=SDL_GetTicks();
  for(int d=0;d<4;d++){
    if(dir[d] && (int32_t)(now-nextAt[d])>=0){
      if(held[PPT_BTN_A])edit(d);else navigate(d);
      nextAt[d]=now+REPEAT_DELAY;dirRepeat[d]=true;
    }
  }
  /* optional mouse from right stick; evdev ABS_RX/ABS_RY calibration is pending */
  if(video.window && (abs(axes[0])>6000 || abs(axes[1])>6000)){
    int x,y;SDL_GetMouseState(&x,&y);
    int dx=(abs(axes[0])>6000)?clamp(axes[0]/7000,-MOUSE_STEP,MOUSE_STEP):0;
    int dy=(abs(axes[1])>6000)?clamp(axes[1]/7000,-MOUSE_STEP,MOUSE_STEP):0;
    SDL_WarpMouseInWindow(video.window,x+dx,y+dy);
  }
}
