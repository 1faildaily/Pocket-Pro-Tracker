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
  /* SELECT alone freezes UI edit position without stopping audio. */
  editor.currMode=MODE_PLAY;editor.songPlaying=true;
  int started=playCalls, stopped=stopCalls;
  buttonEvent(PPT_BTN_SELECT,true);releaseButton(PPT_BTN_SELECT);
  assert(editor.currMode==MODE_PLAY && ppt_live_edit_active());
  assert(editor.songPlaying && playCalls==started && stopCalls==stopped);
  /* LIVE EDIT keeps original MODE_PLAY and editor row independent of playhead. */
  state.row=27; state.currRow=4; state.currPattern=0;
  int unchangedRow=state.currRow; unsigned int beforePattern=state.currPattern;
  /* PT2 replayer sync is patched to skip when ppt_live_edit_active(). */
  assert(ppt_live_edit_active());
  buttonEvent(PPT_BTN_R1,true);releaseButton(PPT_BTN_R1);
  assert(state.currPattern==beforePattern+1);
  assert(state.row==27 && state.currRow==unchangedRow);
  buttonEvent(PPT_BTN_L1,true);releaseButton(PPT_BTN_L1);
  assert(state.currPattern==beforePattern);
  navigate(1);assert(state.currRow==unchangedRow+1 && state.row==27);

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
  editor.songPlaying=false;atomic_store(&liveEditEnabled,false);
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
  editor.songPlaying=true;atomic_store(&liveEditEnabled,false);
  state.currRow=6;cursor.pos=0;patt[24].period=periods[2];patt[24].sample=3;
  insert();assert(patt[24].period==periods[2] && patt[24].sample==3);
  /* SELECT again returns to PLAY without restarting or stopping. */
  editor.songPlaying=true;atomic_store(&liveEditEnabled,true);cursor.pos=0;
  buttonEvent(PPT_BTN_SELECT,true);releaseButton(PPT_BTN_SELECT);
  assert(editor.currMode==MODE_PLAY && !liveEditEnabled && editor.songPlaying);
  assert(playCalls==started && stopCalls==stopped);
  /* SELECT+START now plays PATTERN FROM ROW (not SONG). */
  modStop(); editor.currMode=MODE_IDLE;
  state.currPos=5;state.currRow=16;
  buttonEvent(PPT_BTN_SELECT,true);buttonEvent(PPT_BTN_START,true);
  releaseButton(PPT_BTN_START);releaseButton(PPT_BTN_SELECT);
  assert(playCalls==started+1 && playPatt==(int)state.currPattern &&
         playPos==DONT_SET_ORDER && playRow==16 && editor.playMode==PLAY_MODE_PATTERN);
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
  /* START alone plays PATTERN from ROW 00. */
  state.currPattern=2;state.currRow=29;
  buttonEvent(PPT_BTN_START,true);releaseButton(PPT_BTN_START);
  assert(playPatt==2 && playRow==0 && playPos==DONT_SET_ORDER);
  buttonEvent(PPT_BTN_START,true);releaseButton(PPT_BTN_START);assert(!editor.songPlaying);
  /* MENU+START plays SONG from order 0, row 0. */
  state.currRow=38;state.currPos=7;
  buttonEvent(PPT_BTN_MENU,true);buttonEvent(PPT_BTN_START,true);
  releaseButton(PPT_BTN_MENU);releaseButton(PPT_BTN_START);
  assert(editor.songPlaying && editor.playMode==PLAY_MODE_NORMAL);
  assert(playPatt==DONT_SET_PATTERN && playPos==0 && playRow==0);
  buttonEvent(PPT_BTN_START,true);releaseButton(PPT_BTN_START);assert(!editor.songPlaying);
  /* Reversed press order START -> MENU also starts SONG. */
  buttonEvent(PPT_BTN_START,true);buttonEvent(PPT_BTN_MENU,true);
  releaseButton(PPT_BTN_MENU);releaseButton(PPT_BTN_START);
  assert(editor.songPlaying && editor.playMode==PLAY_MODE_NORMAL && playPos==0);
  buttonEvent(PPT_BTN_START,true);releaseButton(PPT_BTN_START);assert(!editor.songPlaying);
    /* L1+M stays ESC, L1 can't accidentally increment or decrement pattern. */
  state.currPattern=7;
  buttonEvent(PPT_BTN_L1,true);buttonEvent(PPT_BTN_MENU,true);
  assert(lastEvent.key.keysym.scancode==SDL_SCANCODE_ESCAPE);
  releaseButton(PPT_BTN_MENU);releaseButton(PPT_BTN_L1);
  assert(state.currPattern==7);
  puts("PPT v0.3.2: frozen edit row, pattern switching, independent transport, menu+start OK");
  return 0;
}
