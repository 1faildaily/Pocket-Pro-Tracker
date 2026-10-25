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
textEdit_t textEdit;
static int textCommitted, textMovedRight, textMovedLeft;
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
static char lastStatusMessage[64];static int effectStatusMessages;
void displayMsg(const char *msg){snprintf(lastStatusMessage,sizeof(lastStatusMessage),"%s",msg);effectStatusMessages++;}

void updateWindowTitle(bool modified){state.modified=modified;titleUpdates++;}
void updateCurrSample(void){sampleUIUpdates++;}
void sampleUpButton(void){if(editor.currSample<30)editor.currSample++;updateCurrSample();}
void sampleDownButton(void){if(editor.currSample>0)editor.currSample--;updateCurrSample();}
static int pointerModeLast;
void pointerSetMode(uint8_t mode,bool carry){assert((mode==POINTER_MODE_PLAY || mode==POINTER_MODE_IDLE || mode==POINTER_MODE_EDIT) && carry==DO_CARRY);pointerModeLast=mode;pointerModeCalls++;}
void statusAllRight(void){statusCalls++;}
void editTextPrevChar(void){if(textEdit.textPtr>textEdit.textStartPtr)textEdit.textPtr--;
 textMovedLeft++;}
void editTextNextChar(void){if(textEdit.textPtr<textEdit.textEndPtr && *textEdit.textPtr)
 {textEdit.textPtr++;textMovedRight++;}}
void leaveTextEditMode(int update){assert(update==EDIT_TEXT_UPDATE);ui.editTextFlag=false;
 state.modified=true;textCommitted++;}
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
  /* Ghost is invisible in STOP, ordinary PLAY or different pattern. */
  ppt_set_playhead(3,27);
  assert(ppt_ghost_row(3)==-1);
  atomic_store(&liveEditEnabled,true);editor.songPlaying=true;
  assert(ppt_ghost_row(3)==27);
  assert(ppt_ghost_row(4)==-1);
  ppt_set_playhead(4,63);
  assert(ppt_ghost_row(4)==63);
  assert(ppt_ghost_row(3)==-1);
  ppt_set_playhead(255,0);
  assert(ppt_ghost_row(4)==-1);
  atomic_store(&liveEditEnabled,false);editor.songPlaying=false;
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
  edit(0);assert(patt[12].param==1);
  edit(1);assert(patt[12].param==0);
  /* v0.3.9 effect limits: C40 max in hex, B7F max, D63 decimal-Bcd. */
  assert(pptEffectClamp(0xC,0xFF)==0x40);
  assert(pptEffectClamp(0xB,0xFF)==0x7F);
  assert(pptEffectClamp(0xD,0x3A)==0x39);
  assert(pptEffectClamp(0xD,0x6F)==0x63);
  assert(pptEffectClamp(0xE,0xE4)==0xE4);
  assert(pptEffectClamp(0xE,0xE8)==0xE8);
  assert(pptEffectClamp(0xE,0x48)==0x47);
  assert(pptEffectClamp(0xE,0x78)==0x77);
  assert(pptEffectClamp(0xE,0x3F)==0x31);
  assert(pptEffectClamp(0xE,0x8F)==0x8F);
  assert(pptEffectClamp(0xE,0x0F)==0x0F);
  assert(pptEffectStep(0xD,0x59,CURSOR_PARAM2,3)==0x60);
  assert(pptEffectStep(0xD,0x63,CURSOR_PARAM2,3)==0x63);
  assert(pptEffectStep(0xD,0x00,CURSOR_PARAM2,2)==0x00);
  assert(pptEffectStep(0xC,0x3F,CURSOR_PARAM2,3)==0x40);
  assert(pptEffectStep(0xC,0x40,CURSOR_PARAM2,3)==0x40);
  assert(pptEffectStep(0xE,0x30,CURSOR_PARAM2,3)==0x31);
  assert(pptEffectStep(0xE,0x31,CURSOR_PARAM2,3)==0x31);
  assert(pptEffectStep(0xE,0x43,CURSOR_PARAM2,3)==0x44);
  assert(!strcmp(pptEffectName(0xC,0),"C SET VOLUME"));
  assert(!strcmp(pptEffectName(0xE,0x42),"E4 VIBRATO WAVE"));
  /* A on FX/PARAM uses the active effect's range rather than old cached
     parameter from another effect. */
  cursor.pos=CURSOR_CMD;state.currRow=8;
  lastCmd=0xC; lastParam=0xFF; insert();
  assert(patt[32].command==0xC && patt[32].param==0);
  cursor.pos=CURSOR_PARAM2;lastParam=0xFF;insert();assert(patt[32].param==0x40);
  edit(3);assert(patt[32].param==0x40);
  cursor.pos=CURSOR_CMD;lastCmd=0xD;insert();
  cursor.pos=CURSOR_PARAM2;lastParam=0x7F;insert();
  assert(patt[32].command==0xD && patt[32].param==0x63);
  /* Effect STATUS updates when focus is in FX, without clobbering song
     title editing and other ProTracker dialogs. */
  cursor.pos=CURSOR_CMD;lastEffectCursor=-1;updateEffectStatus();
  assert(!strcmp(lastStatusMessage,"D PATTERN BREAK"));
  int beforeEffectStatus=effectStatusMessages;
  updateEffectStatus();assert(effectStatusMessages==beforeEffectStatus);
  cursor.pos=CURSOR_NOTE;updateEffectStatus();
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
  /* v0.3.5: all mouse speeds 70% of v0.3.4, without changing curve/deadzone.
     Check both analog (fixed point) and R1+D-pad/askBox digital mouse. */
  assert(MOUSE_SPEED_NUM==7 && MOUSE_SPEED_DEN==10);
  int slowFrac=0, slowTotal=0;
  for(int i=0;i<10;i++)slowTotal+=scaledMousePixels(6,&slowFrac,1);
  assert(slowTotal==42 && slowFrac==0); /* formerly 60px */
  for(int i=0;i<10;i++)slowTotal+=scaledMousePixels(-6,&slowFrac,1);
  assert(slowTotal==0 && slowFrac==0);
  slowFrac=0;slowTotal=0;
  for(int i=0;i<1024;i++)slowTotal+=scaledMousePixels(precisionMouseStep(32767),&slowFrac,MOUSE_ANALOG_DEN);
  assert(slowTotal==(precisionMouseStep(32767)*7)/10);
  slowFrac=0;slowTotal=0;
  for(int i=0;i<1024;i++)slowTotal+=scaledMousePixels(precisionMouseStep(8000),&slowFrac,MOUSE_ANALOG_DEN);
  assert(slowTotal==(precisionMouseStep(8000)*7)/10);
  /* Full joystick deflection is slower too; mouse curve remains untouched. */
  assert(precisionMouseStep(32767)==2663);

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
  /* MENU+START plays SONG from CURRENT POSITION, row 00. */
  state.currRow=38;state.currPos=7;
  buttonEvent(PPT_BTN_MENU,true);buttonEvent(PPT_BTN_START,true);
  releaseButton(PPT_BTN_MENU);releaseButton(PPT_BTN_START);
  assert(editor.songPlaying && editor.playMode==PLAY_MODE_NORMAL);
  assert(playPatt==DONT_SET_PATTERN && playPos==7 && playRow==0);
  buttonEvent(PPT_BTN_START,true);releaseButton(PPT_BTN_START);assert(!editor.songPlaying);
  /* Reversed press order START -> MENU also starts SONG. */
  buttonEvent(PPT_BTN_START,true);buttonEvent(PPT_BTN_MENU,true);
  releaseButton(PPT_BTN_MENU);releaseButton(PPT_BTN_START);
  assert(editor.songPlaying && editor.playMode==PLAY_MODE_NORMAL && playPos==7);
  buttonEvent(PPT_BTN_START,true);releaseButton(PPT_BTN_START);assert(!editor.songPlaying);
    /* L1+M stays ESC, L1 can't accidentally increment or decrement pattern. */
  state.currPattern=7;
  buttonEvent(PPT_BTN_L1,true);buttonEvent(PPT_BTN_MENU,true);
  assert(lastEvent.key.keysym.scancode==SDL_SCANCODE_ESCAPE);
  releaseButton(PPT_BTN_MENU);releaseButton(PPT_BTN_L1);
  assert(state.currPattern==7);
  /* Native SONGNAME string editing: X enters by clicking songname in PT2;
     the carousel only takes over once the native ui.editTextFlag is set. */
  editor.songPlaying=false;
  ui.editTextFlag=true;ui.askBoxShown=false;
  textEdit.type=TEXT_EDIT_STRING;textEdit.object=PTB_SONGNAME;
  textEdit.textStartPtr=state.header.name;
  textEdit.textEndPtr=state.header.name+19;
  textEdit.textPtr=textEdit.textStartPtr;
  memset(state.header.name,0,sizeof(state.header.name));
  directionAction(0);assert(strcmp(state.header.name,"a")==0);
  directionAction(0);assert(strcmp(state.header.name,"b")==0);
  directionAction(1);assert(strcmp(state.header.name,"a")==0);
  buttonEvent(PPT_BTN_X,true);releaseButton(PPT_BTN_X);
  assert(textEdit.textPtr==state.header.name+1 && !mouseDownForwarded[PPT_BTN_X]);
  directionAction(1);assert(state.header.name[1]=='_');
  directionAction(0);assert(state.header.name[1]==' ');
  directionAction(0);assert(state.header.name[1]=='a');
  /* LEFT/RIGHT are native text navigation. */
  directionAction(2);assert(textEdit.textPtr==state.header.name);
  directionAction(3);assert(textEdit.textPtr==state.header.name+1);
  buttonEvent(PPT_BTN_Y,true);releaseButton(PPT_BTN_Y);
  assert(!ui.editTextFlag && textCommitted==1);
  assert(strcmp(state.header.name,"aa")==0 && state.modified);
  /* v0.3.7: B deletes the preceding character in SONGNAME and keeps
     the rest of the title. It must not exit edit mode or activate
     the ordinary B clipboard action while editing a title. */
  ui.editTextFlag=true; textEdit.object=PTB_SONGNAME;
  textEdit.textStartPtr=state.header.name;
  textEdit.textEndPtr=state.header.name+19;
  strcpy(state.header.name,"abcdef");
  textEdit.textPtr=state.header.name+3; /* abc|def -> ab|def */
  clip.kind=PPT_CLIP_NONE;
  buttonEvent(PPT_BTN_B,true);releaseButton(PPT_BTN_B);
  assert(strcmp(state.header.name,"abdef")==0);
  assert(textEdit.textPtr==state.header.name+2 && ui.editTextFlag);
  assert(clip.kind==PPT_CLIP_NONE && ui.updateSongName);
  buttonEvent(PPT_BTN_B,true);releaseButton(PPT_BTN_B);
  assert(strcmp(state.header.name,"adef")==0);
  assert(textEdit.textPtr==state.header.name+1);
  buttonEvent(PPT_BTN_B,true);releaseButton(PPT_BTN_B);
  assert(strcmp(state.header.name,"def")==0);
  assert(textEdit.textPtr==state.header.name);
  /* Backspace at the start is a harmless no-op. */
  buttonEvent(PPT_BTN_B,true);releaseButton(PPT_BTN_B);
  assert(strcmp(state.header.name,"def")==0 && textEdit.textPtr==state.header.name);
  /* The last character is removed when caret is at the end. */
  textEdit.textPtr=state.header.name+3;
  buttonEvent(PPT_BTN_B,true);releaseButton(PPT_BTN_B);
  assert(strcmp(state.header.name,"de")==0 && textEdit.textPtr==state.header.name+2);
  /* Native editor may point beyond text end; delete last real character. */
  textEdit.textPtr=state.header.name+6;
  buttonEvent(PPT_BTN_B,true);releaseButton(PPT_BTN_B);
  assert(strcmp(state.header.name,"d")==0 && textEdit.textPtr==state.header.name+1);
  /* Protect the MOD title's 20-char boundary and its NUL terminator. */
  memset(state.header.name,'x',20);state.header.name[20]='\0';
  textEdit.textPtr=state.header.name+19;
  buttonEvent(PPT_BTN_B,true);releaseButton(PPT_BTN_B);
  assert(strlen(state.header.name)==19 && state.header.name[19]=='\0');
  assert(textEdit.textPtr==state.header.name+18);
  buttonEvent(PPT_BTN_Y,true);releaseButton(PPT_BTN_Y);
  assert(!ui.editTextFlag && textCommitted==2 && state.modified);
  /* NEW rectangular selection: SELECT+B starts, D-pad expands,
     next B snapshots the block, SELECT+A pastes it elsewhere. */
  ui.editTextFlag=false; state.currPattern=0; state.currRow=8;
  editor.songPlaying=false; cursor.pos=0; cursor.channel=0;
  clip.kind=PPT_CLIP_NONE;
  patt[8*4].period=periods[3];patt[9*4].period=periods[6];
  buttonEvent(PPT_BTN_SELECT,true);buttonEvent(PPT_BTN_B,true);
  releaseButton(PPT_BTN_B);releaseButton(PPT_BTN_SELECT);
  assert(selecting && ppt_selection_contains(0,8,0,0));
  assert(!ppt_selection_contains(0,8,0,1));
  directionAction(1);
  assert(state.currRow==9 && ppt_selection_contains(0,9,0,0));
  buttonEvent(PPT_BTN_B,true);releaseButton(PPT_BTN_B);
  assert(!selecting && clip.kind==PPT_CLIP_BLOCK && clip.rowLo==8 && clip.rowHi==9);
  assert(clip.colLo==0 && clip.colHi==0);
  state.currRow=12; patt[12*4].period=0;patt[13*4].period=0;
  buttonEvent(PPT_BTN_SELECT,true);buttonEvent(PPT_BTN_A,true);
  releaseButton(PPT_BTN_A);releaseButton(PPT_BTN_SELECT);
  assert(patt[12*4].period==periods[3] && patt[13*4].period==periods[6]);
  /* Extend selection in both axes across channels and paste to a new channel. */
  state.currRow=8; cursor.pos=5;cursor.channel=0;
  buttonEvent(PPT_BTN_SELECT,true);buttonEvent(PPT_BTN_B,true);
  releaseButton(PPT_BTN_B);releaseButton(PPT_BTN_SELECT);
  directionAction(3);directionAction(3);directionAction(1);
  assert(cursor.pos==7 && ppt_selection_contains(0,9,1,1));
  buttonEvent(PPT_BTN_B,true);releaseButton(PPT_BTN_B);
  assert(clip.kind==PPT_CLIP_BLOCK && clip.colLo==5 && clip.colHi==7);
  /* Channel copy: copying one channel must preserve the other three. */
  state.currRow=10;cursor.pos=6;cursor.channel=1;
  patt[1].period=periods[5];patt[2].period=periods[2];
  buttonEvent(PPT_BTN_SELECT,true);buttonEvent(PPT_BTN_B,true);
  buttonEvent(PPT_BTN_L1,true);
  assert(clip.kind==PPT_CLIP_CHANNEL && clip.sourceChannel==1 && !selecting);
  releaseButton(PPT_BTN_L1);releaseButton(PPT_BTN_B);releaseButton(PPT_BTN_SELECT);
  assert(state.currPattern==0);
  cursor.pos=12;cursor.channel=2;
  buttonEvent(PPT_BTN_SELECT,true);buttonEvent(PPT_BTN_A,true);
  releaseButton(PPT_BTN_A);releaseButton(PPT_BTN_SELECT);
  assert(patt[2].period==periods[5] && patt[1].period==periods[5]);
  /* Pattern copy: snapshot all rows/channels, then overwrite entire target. */
  patt[0].period=periods[9];
  buttonEvent(PPT_BTN_SELECT,true);buttonEvent(PPT_BTN_B,true);
  buttonEvent(PPT_BTN_R1,true);
  releaseButton(PPT_BTN_R1);releaseButton(PPT_BTN_B);releaseButton(PPT_BTN_SELECT);
  assert(clip.kind==PPT_CLIP_PATTERN && !selecting);
  patt[0].period=0;
  buttonEvent(PPT_BTN_SELECT,true);buttonEvent(PPT_BTN_A,true);
  releaseButton(PPT_BTN_A);releaseButton(PPT_BTN_SELECT);
  assert(patt[0].period==periods[9]);
  /* Single-field sample nibble selection must not overwrite its neighbor. */
  state.currRow=20;cursor.pos=1;cursor.channel=0;
  patt[20*4].sample=0xAB;
  buttonEvent(PPT_BTN_SELECT,true);buttonEvent(PPT_BTN_B,true);
  releaseButton(PPT_BTN_B);releaseButton(PPT_BTN_SELECT);
  assert(selecting && ppt_selection_contains(0,20,0,1) &&
         !ppt_selection_contains(0,20,0,2));
  buttonEvent(PPT_BTN_B,true);releaseButton(PPT_BTN_B);
  state.currRow=21;cursor.pos=1;
  patt[21*4].sample=0x34;
  buttonEvent(PPT_BTN_SELECT,true);buttonEvent(PPT_BTN_A,true);
  releaseButton(PPT_BTN_A);releaseButton(PPT_BTN_SELECT);
  assert(patt[21*4].sample==0xA4);
  /* Copy from channel 0 by pressing L1 BEFORE B, and do not switch pattern. */
  state.currPattern=0;cursor.pos=0;cursor.channel=0;
  buttonEvent(PPT_BTN_SELECT,true);buttonEvent(PPT_BTN_L1,true);
  buttonEvent(PPT_BTN_B,true);releaseButton(PPT_BTN_B);
  releaseButton(PPT_BTN_L1);releaseButton(PPT_BTN_SELECT);
  assert(clip.kind==PPT_CLIP_CHANNEL && state.currPattern==0);
  /* SELECT+B+B may be confirmed while SELECT is still down. */
  state.currRow=22;cursor.pos=0;
  buttonEvent(PPT_BTN_SELECT,true);buttonEvent(PPT_BTN_B,true);
  releaseButton(PPT_BTN_B);
  assert(selecting);
  buttonEvent(PPT_BTN_B,true);releaseButton(PPT_BTN_B);
  releaseButton(PPT_BTN_SELECT);
  assert(!selecting && clip.kind==PPT_CLIP_BLOCK && clip.rowLo==22);
  /* Clipboard also works during LIVE EDIT without stopping or moving playhead. */
  editor.songPlaying=true;atomic_store(&liveEditEnabled,true);
  int lockBefore=lockCalls;
  state.currRow=24;cursor.pos=0;cursor.channel=0;
  patt[24*4].period=periods[2];
  buttonEvent(PPT_BTN_SELECT,true);buttonEvent(PPT_BTN_B,true);
  releaseButton(PPT_BTN_B);releaseButton(PPT_BTN_SELECT);
  directionAction(1);
  buttonEvent(PPT_BTN_B,true);releaseButton(PPT_BTN_B);
  state.currRow=28;patt[28*4].period=0;
  buttonEvent(PPT_BTN_SELECT,true);buttonEvent(PPT_BTN_A,true);
  releaseButton(PPT_BTN_A);releaseButton(PPT_BTN_SELECT);
  assert(editor.songPlaying && ppt_live_edit_active());
  assert(patt[28*4].period==periods[2] && lockCalls>lockBefore && lockCalls==unlockCalls);
  editor.songPlaying=false;atomic_store(&liveEditEnabled,false);
  /* B alone is no longer a single-field copy action. */
  clip.kind=PPT_CLIP_NONE;
  buttonEvent(PPT_BTN_B,true);releaseButton(PPT_BTN_B);
  assert(clip.kind==PPT_CLIP_NONE);
  /* Outside SONGNAME, X/Y remain mouse buttons. Even if edit activates
     between mouse-down and mouse-up, the original release must be sent. */
  state.modified=false;ui.editTextFlag=false;video.window=(SDL_Window*)1;
  buttonEvent(PPT_BTN_X,true);
  assert(mouseDownForwarded[PPT_BTN_X] && lastEvent.type==SDL_MOUSEBUTTONDOWN);
  ui.editTextFlag=true;
  buttonEvent(PPT_BTN_X,true); /* ignored as held */
  releaseButton(PPT_BTN_X);
  assert(!mouseDownForwarded[PPT_BTN_X] && lastEvent.type==SDL_MOUSEBUTTONUP);
  ui.editTextFlag=false;
  buttonEvent(PPT_BTN_Y,true);assert(mouseDownForwarded[PPT_BTN_Y]);
  releaseButton(PPT_BTN_Y);assert(!mouseDownForwarded[PPT_BTN_Y]);
  /* 20-character limit, memory-safe continuation and native exit. */
  memset(state.header.name,'x',20);state.header.name[20]='\0';
  ui.editTextFlag=true;textEdit.object=PTB_SONGNAME;
  textEdit.textPtr=state.header.name+19;
  directionAction(0);assert(state.header.name[20]=='\0');
  int oldMoves=textMovedRight;
  buttonEvent(PPT_BTN_X,true);releaseButton(PPT_BTN_X);
  assert(textEdit.textPtr==state.header.name+19 && textMovedRight==oldMoves);
  buttonEvent(PPT_BTN_Y,true);releaseButton(PPT_BTN_Y);
  assert(!ui.editTextFlag && textCommitted==3);
  /* Sample names and numeric text editing are unaffected. */
  ui.editTextFlag=true;textEdit.object=PTB_SONGNAME+1;
  assert(!isSongNameEdit());
  ui.editTextFlag=false;

  /* POSITION SELECT belongs to the PT2 POS editor, not pattern/field edit. */
  state.header.songLength=16;
  for(int i=0;i<16;i++)state.header.patternTable[i]=i+10;
  state.currPos=8;state.currPattern=18;cursor.pos=0;
  editor.songPlaying=false;editor.currMode=MODE_IDLE;
  ui.posEdScreenShown=true;ui.editTextFlag=false;
  posLoopOwner=&state;clearPosLoop();
  const int previousPatt=(int)state.currPattern;
  directionAction(1);assert(state.currPos==9 && state.currPattern==19);
  directionAction(0);assert(state.currPos==8 && (int)state.currPattern==previousPatt);
  assert(state.header.patternTable[8]==18 && state.header.patternTable[9]==19);
  assert(editor.currPatternDisp==&state.header.patternTable[8]);
  /* One B starts a rectangular inclusive POS range at the current order. */
  buttonEvent(PPT_BTN_SELECT,true);buttonEvent(PPT_BTN_B,true);
  releaseButton(PPT_BTN_B);releaseButton(PPT_BTN_SELECT);
  assert(posMarking && posMarkStart==8 && ppt_loop_entry_style(8)==1);
  directionAction(1);directionAction(1);directionAction(1);
  assert(state.currPos==11 && posMarking);
  assert(ppt_loop_entry_style(10)==1 && ppt_loop_entry_style(12)==0);
  buttonEvent(PPT_BTN_B,true);releaseButton(PPT_BTN_B);
  assert(!posMarking && ppt_loop_entry_style(8)==2 && ppt_loop_entry_style(11)==2);
  assert(ppt_loop_redirect(9,16)==9);
  assert(ppt_loop_redirect(12,16)==8); /* after last selected POS */
  assert(ppt_loop_redirect(0,16)==8); /* Bxx outside selected range */
  assert(ppt_loop_redirect(16,16)==8); /* end of song */
  /* MENU+START launches loop from first marked order, row 00. */
  int playsBefore=playCalls;
  buttonEvent(PPT_BTN_MENU,true);buttonEvent(PPT_BTN_START,true);
  releaseButton(PPT_BTN_START);releaseButton(PPT_BTN_MENU);
  assert(playCalls==playsBefore+1 && playPos==8 && playRow==0 && editor.songPlaying);
  buttonEvent(PPT_BTN_START,true);releaseButton(PPT_BTN_START);
  assert(!editor.songPlaying);
  /* SELECT+Y fully clears the loop AND the gray highlight.
     Repeating CLEAR does not restore the old range. */
  buttonEvent(PPT_BTN_SELECT,true);buttonEvent(PPT_BTN_Y,true);
  releaseButton(PPT_BTN_Y);releaseButton(PPT_BTN_SELECT);
  assert(ppt_loop_entry_style(9)==0 && ppt_loop_redirect(12,16)==12);
  assert(atomic_load(&positionLoop)==0 && !posMarking);
  buttonEvent(PPT_BTN_SELECT,true);buttonEvent(PPT_BTN_Y,true);
  releaseButton(PPT_BTN_Y);releaseButton(PPT_BTN_SELECT);
  assert(ppt_loop_entry_style(9)==0 && atomic_load(&positionLoop)==0);
  /* Menu song play without a loop starts at currently selected POS. */
  buttonEvent(PPT_BTN_MENU,true);buttonEvent(PPT_BTN_START,true);
  releaseButton(PPT_BTN_START);releaseButton(PPT_BTN_MENU);
  assert(playPos==11 && editor.songPlaying);
  buttonEvent(PPT_BTN_START,true);releaseButton(PPT_BTN_START);
  /* POSITION view must follow native playback, while marking a range
     uses a separate endpoint (and must not seek the playing song). */
  state.currPos=8;state.currPattern=18;
  editor.songPlaying=true;atomic_store(&posBrowseActive,true);
  assert(ppt_position_browse_active());
  buttonEvent(PPT_BTN_SELECT,true);buttonEvent(PPT_BTN_B,true);
  releaseButton(PPT_BTN_B);releaseButton(PPT_BTN_SELECT);
  assert(posMarking && posMarkStart==8 && posMarkEnd==8);
  int beforeDirection=(int)state.currPos;
  directionAction(1);directionAction(1);directionAction(1);
  assert(posMarkEnd==11 && (int)state.currPos==beforeDirection && editor.songPlaying);
  /* Mimic PT2's UI display following 2 more audio rows/positions. */
  state.currPos=10;
  assert(ppt_loop_entry_style(8)==1 && ppt_loop_entry_style(11)==1);
  assert(ppt_loop_entry_style(12)==0);
  buttonEvent(PPT_BTN_B,true);releaseButton(PPT_BTN_B);
  assert(!posMarking && ppt_loop_entry_style(11)==2);
  assert(ppt_loop_redirect(12,16)==8); /* 08..11 despite playback POS 10 */
  buttonEvent(PPT_BTN_SELECT,true);buttonEvent(PPT_BTN_Y,true);
  releaseButton(PPT_BTN_Y);releaseButton(PPT_BTN_SELECT);
  assert(ppt_loop_entry_style(11)==0 && ppt_loop_redirect(12,16)==12);
  editor.songPlaying=false;atomic_store(&posBrowseActive,false);
  ui.posEdScreenShown=false;
  /* Clearing a loaded module invalidates the in-session selection. */
  clearPosLoop();assert(ppt_loop_entry_style(9)==0 && ppt_loop_redirect(12,16)==12);
  assert(state.header.patternTable[8]==18 && state.header.patternTable[9]==19);

  puts("PPT v0.3.10: POSITION playback follow, independent mark and full clear OK");
  return 0;
}
