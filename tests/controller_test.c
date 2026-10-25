#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "pt2_header.h"
#include "pt2_structs.h"
module_t state,*song=&state;
editor_t editor;
audio_t audio;
ui_t ui;
mouse_t mouse;
cursor_t cursor;
video_t video;
static int playCalls, playPatt, playPos, playRow, stopCalls;
static int pointerModeCalls, statusCalls;
static int previewCalls,previewPitch,previewSample;
static int sampleUIUpdates, patternUIUpdates, titleUpdates;
void pptPreviewInsertedNote(int8_t pitch,int8_t sampleIndex){assert(!audio.locked);previewCalls++;previewPitch=pitch;previewSample=sampleIndex;}
void modPlay(int16_t patt,int16_t pos,int8_t row){playCalls++;playPatt=patt;playPos=pos;playRow=row;editor.songPlaying=true;}
void modStop(void){stopCalls++;editor.songPlaying=false;}
void setPattern(int16_t patt){state.currPattern=(uint16_t)patt;patternUIUpdates++;}
void incPatt(void){state.currPattern=(state.currPattern+1)%MAX_PATTERNS;patternUIUpdates++;}
void decPatt(void){state.currPattern=(state.currPattern+MAX_PATTERNS-1)%MAX_PATTERNS;patternUIUpdates++;}
static int lockCalls, unlockCalls;
void lockAudio(void){assert(!audio.locked);audio.locked=true;lockCalls++;}
void unlockAudio(void){assert(audio.locked);audio.locked=false;unlockCalls++;}
void displayMsg(const char *msg){(void)msg;}

void updateWindowTitle(bool modified){state.modified=modified;titleUpdates++;}
void updateCurrSample(void){sampleUIUpdates++;}
void sampleUpButton(void){if(editor.currSample<30)editor.currSample++;updateCurrSample();}
void sampleDownButton(void){if(editor.currSample>0)editor.currSample--;updateCurrSample();}
static int pointerModeLast;
void pointerSetMode(uint8_t mode,bool carry){assert((mode==POINTER_MODE_PLAY || mode==POINTER_MODE_IDLE || mode==POINTER_MODE_EDIT) && carry==DO_CARRY);pointerModeLast=mode;pointerModeCalls++;}
void statusAllRight(void){statusCalls++;}
void updateCursorPos(void){}
void setSpritePos(int s,int x,int y){(void)s;(void)x;(void)y;}
unsigned int SDL_GetTicks(void){return 0;}
static SDL_Event lastEvent; static int eventsPushed;
int SDL_PushEvent(SDL_Event *event){lastEvent=*event;eventsPushed++;return 1;}
unsigned int SDL_GetMouseState(int*x,int*y){*x=0;*y=0;return 0;}
void SDL_WarpMouseInWindow(SDL_Window*w,int x,int y){(void)w;(void)x;(void)y;}
#include "../patch/ppt_input.c"
int main(void){
  static note_t patt[64*4];
  memset(&state,0,sizeof(state));
  state.patterns[0]=patt;
  cursor.pos=0;
  editor.currSample=0;
  /* Physical calibration unchanged. */
  assert(rawMap[PPT_BTN_SELECT]==310 && rawMap[PPT_BTN_START]==311);
  assert(rawMap[PPT_BTN_L1]==308 && rawMap[PPT_BTN_R1]==309);
  assert(menuRawCode(312));
  /* Original ProTracker ALT+LEFT/RIGHT semantics: PATTERN changes; ORDER TABLE doesn't. */
  state.header.patternTable[0]=17;
  state.currPattern=7;
  buttonEvent(PPT_BTN_R1,true);releaseButton(PPT_BTN_R1);
  assert(state.currPattern==8 && state.header.patternTable[0]==17);
  buttonEvent(PPT_BTN_L1,true);releaseButton(PPT_BTN_L1);
  assert(state.currPattern==7 && state.header.patternTable[0]==17);
  /* Up increases octave/numeric values. Down decreases. */
  state.currPattern=0;
  insert();assert(patt[0].period==214 && patt[0].sample==1);
  edit(0);assert(patt[0].period==113); /* clamped to max */
  int afterUp=lastPitch; assert(afterUp>24);
  edit(1);assert(lastPitch==23 && patt[0].period==226);
  cursor.pos=1;state.currRow=2;
  insert(); int beforeSample=patt[8].sample;
  edit(0);assert(patt[8].sample>beforeSample);
  edit(1);assert(patt[8].sample==beforeSample);
  cursor.pos=5;state.currRow=3;
  edit(0);assert(patt[12].param==12);
  edit(1);assert(patt[12].param==0);
  /* SELECT alone while music plays switches to edit without modStop/modPlay. */
  editor.currMode=MODE_PLAY;editor.songPlaying=true;
  int started=playCalls, stopped=stopCalls;
  buttonEvent(PPT_BTN_SELECT,true);releaseButton(PPT_BTN_SELECT);
  assert(editor.currMode==MODE_PLAY && liveEditEnabled);
  assert(editor.songPlaying && playCalls==started && stopCalls==stopped);
  /* LIVE EDIT keeps the original MODE_PLAY and pointer state intact. */
  /* Editing while playing locks the audio callback for atomic data update. */
  state.currRow=4;cursor.pos=0;
  int beforeLocks=lockCalls;
  editor.currSample=4;
  patt[16].period=periods[0];patt[16].sample=2;
  insert();assert(lockCalls>beforeLocks && lockCalls==unlockCalls);
  assert(patt[16].period==periods[lastPitch] && patt[16].sample==5);
  /* Native audio preview is attempted only AFTER unlocking the mixer. */
  assert(previewSample==4);
  /* Selecting another sample and pressing A overwrites OLD note+sample. */
  editor.songPlaying=false;liveEditEnabled=false;
  cursor.pos=0;state.currRow=5; editor.currSample=7;
  patt[20].period=periods[1];patt[20].sample=3;
  int previewBefore=previewCalls;
  insert();
  assert(patt[20].period==periods[lastPitch] && patt[20].sample==8);
  assert(previewCalls==previewBefore+1 && previewSample==7 && previewPitch==lastPitch);
  /* SAMPLE field A must overwrite and audition, not retain previous sample. */
  editor.currSample=10;cursor.pos=CURSOR_SAMPLE1;
  insert();assert(patt[20].sample==11);
  assert(previewSample==10 && previewCalls==previewBefore+2);
  /* Precision mouse: tiny deflection moves by fractions, not huge steps. */
  assert(precisionMouseStep(4200)==0);
  assert(precisionMouseStep(8000)>0);
  assert(precisionMouseStep(32767)>precisionMouseStep(8000));
  assert(precisionMouseStep(-8000)==-precisionMouseStep(8000));
  /* No hidden edits are allowed while playing but LIVE EDIT is off. */
  editor.songPlaying=true;liveEditEnabled=false;
  state.currRow=6;cursor.pos=0;patt[24].period=periods[2];patt[24].sample=3;
  insert();assert(patt[24].period==periods[2] && patt[24].sample==3);
  /* SELECT again returns to PLAY without restarting or stopping. */
  editor.songPlaying=true;liveEditEnabled=true;cursor.pos=0;
  buttonEvent(PPT_BTN_SELECT,true);releaseButton(PPT_BTN_SELECT);
  assert(editor.currMode==MODE_PLAY && !liveEditEnabled && editor.songPlaying);
  assert(playCalls==started && stopCalls==stopped);
  /* SELECT+START is a chord, not a live-edit toggle. */
  modStop(); editor.currMode=MODE_IDLE;
  state.currPos=5;state.currRow=16;
  buttonEvent(PPT_BTN_SELECT,true);buttonEvent(PPT_BTN_START,true);
  releaseButton(PPT_BTN_START);releaseButton(PPT_BTN_SELECT);
  assert(playCalls==started+1 && playPos==5 && playRow==16);
  assert(editor.currMode==MODE_PLAY && !liveEditEnabled);
  /* SELECT held while pressing A doesn't flip playback mode. */
  buttonEvent(PPT_BTN_SELECT,true);buttonEvent(PPT_BTN_A,true);
  releaseButton(PPT_BTN_A);releaseButton(PPT_BTN_SELECT);
  assert(editor.currMode==MODE_PLAY && !liveEditEnabled);
  /* START stops after live edit and doesn't leave record mode active. */
  buttonEvent(PPT_BTN_SELECT,true);releaseButton(PPT_BTN_SELECT);
  assert(liveEditEnabled);
  buttonEvent(PPT_BTN_START,true);releaseButton(PPT_BTN_START);
  assert(!editor.songPlaying && editor.currMode==MODE_IDLE && !liveEditEnabled);
  /* L1+M stays ESC, L1 can't accidentally increment or decrement pattern. */
  state.currPattern=7;
  buttonEvent(PPT_BTN_L1,true);buttonEvent(PPT_BTN_MENU,true);
  assert(lastEvent.key.keysym.scancode==SDL_SCANCODE_ESCAPE);
  releaseButton(PPT_BTN_MENU);releaseButton(PPT_BTN_L1);
  assert(state.currPattern==7);
  puts("PPT v0.3.1: live edit safe, sample overwrite, preview, precise mouse, chords OK");
  return 0;
}
