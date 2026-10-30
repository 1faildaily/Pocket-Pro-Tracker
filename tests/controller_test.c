#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "pt2_header.h"
#include "pt2_structs.h"
module_t state,*song=&state;
editor_t editor;
keyb_t keyb;
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
static int recordCalls;
void pptRecordInsertedNote(int8_t p,int8_t sample){(void)p;(void)sample;assert(editor.currMode==MODE_RECORD && editor.songPlaying);recordCalls++;}
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
static int previousPointerModeCalls, previousStatusCalls;
void pointerSetPreviousMode(void){previousPointerModeCalls++;}
void setPrevStatusMessage(void){previousStatusCalls++;}
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
static int viewRenders, samplerOpens, samplerExits, diskRenders;
void renderDiskOpScreen(void){diskRenders++;}
void displayMainScreen(void){viewRenders++;if(ui.diskOpScreenShown)renderDiskOpScreen();}
void samplerScreen(void){samplerOpens++;ui.samplerScreenShown=true;}
void exitFromSam(void){samplerExits++;ui.samplerScreenShown=false;}

static int samplerWholeAuditions, samplerRangeAuditions, samplerNewSlots;
static int samplerRangeRenders, samplerPosUpdates, samplerErrs;
static int samplerShiftWasHeld;
void samplerPlayWaveform(void){samplerWholeAuditions++;}
void samplerPlayRange(void){samplerRangeAuditions++;}
void samplerSamCopy(void){samplerNewSlots++;samplerShiftWasHeld=keyb.shiftPressed;}
void displaySample(void){samplerRangeRenders++;}
void updateSamplePos(void){samplerPosUpdates++;}
void displayErrorMsg(const char *s){(void)s;samplerErrs++;}
static int browserPreviews;
bool ppt_browser_sample_mode(void){return ui.diskOpScreenShown;}
void ppt_browser_update(void){}
void ppt_browser_close(void){}
void ppt_browser_preview(void){browserPreviews++;}
static bool browserDialog;
bool ppt_browser_import_dialog(void){return browserDialog;}
static int diskMoves,diskActivations;
void ppt_disk_move(int d){diskMoves+=d;}
void ppt_disk_activate(void){diskActivations++;}
static uint32_t fakeTicks;
unsigned int SDL_GetTicks(void){return fakeTicks;}
static SDL_Event lastEvent; static int eventsPushed;
int SDL_PushEvent(SDL_Event *event){lastEvent=*event;eventsPushed++;return 1;}
unsigned int SDL_GetMouseState(int*x,int*y){*x=0;*y=0;return 0;}
void SDL_WarpMouseInWindow(SDL_Window*w,int x,int y){(void)w;(void)x;(void)y;}
static char qaNames[MAX_PATTERNS][16];
char *ppt_pattern_name(int pattern){return pattern>=0 && pattern<MAX_PATTERNS?qaNames[pattern]:NULL;}
void renderMuteButtons(void){}
#include "../patch/ppt_input.c"
int main(void){
  static note_t patt[64*4];
  memset(&state,0,sizeof(state));
  state.patterns[0]=patt;
  cursor.pos=0;
  editor.currSample=0;
  assert(buttonFromRaw(BTN_THUMBR)==PPT_BTN_R3);
  assert(buttonFromRaw(BTN_MODE)==PPT_BTN_R3);
  assert(buttonFromRaw(312)==PPT_BTN_MENU);
  assert(buttonFromRaw(313)==PPT_BTN_L3);
  assert(buttonFromRaw(BTN_THUMBL)==PPT_BTN_L3);
  assert(buttonFromRaw(KEY_MENU)==PPT_BTN_MENU);
  /* R3 toggles; MENU never switches. A activates disk entry; X is a mouse click. */
  ui.diskOpScreenShown=true;
  buttonEvent(PPT_BTN_MENU,true);releaseButton(PPT_BTN_MENU);
  assert(!ppt_full_pattern_active() && ui.diskOpScreenShown);
  buttonEvent(PPT_BTN_R3,true);assert(ppt_full_pattern_active());
  buttonEvent(PPT_BTN_R3,true);assert(ppt_full_pattern_active()); /* key repeat */
  releaseButton(PPT_BTN_R3);
  buttonEvent(PPT_BTN_R3,true);releaseButton(PPT_BTN_R3);
  assert(!ppt_full_pattern_active() && ui.diskOpScreenShown);
  int eventCount=eventsPushed;
  directionAction(1);assert(diskMoves==1);
  buttonEvent(PPT_BTN_A,true);releaseButton(PPT_BTN_A);
  assert(diskActivations==1 && eventsPushed==eventCount);
  buttonEvent(PPT_BTN_B,true);releaseButton(PPT_BTN_B);assert(browserPreviews==1);
  int slotEvents=eventsPushed;
  editor.songPlaying=true;
  buttonEvent(PPT_BTN_R2,true);releaseButton(PPT_BTN_R2);assert(editor.currSample==1);
  ui.askBoxShown=true;browserDialog=true;
  buttonEvent(PPT_BTN_R2,true);releaseButton(PPT_BTN_R2);assert(editor.currSample==2);
  buttonEvent(PPT_BTN_L2,true);releaseButton(PPT_BTN_L2);assert(editor.currSample==1);
  assert(eventsPushed==slotEvents && editor.songPlaying);
  directionAction(0);assert(lastEvent.key.keysym.sym==SDLK_UP);
  directionAction(1);assert(lastEvent.key.keysym.sym==SDLK_DOWN);
  directionAction(2);assert(lastEvent.key.keysym.sym==SDLK_LEFT);
  directionAction(3);assert(lastEvent.key.keysym.sym==SDLK_RIGHT);
  ui.askBoxShown=false;browserDialog=false;editor.songPlaying=false;editor.currSample=0;

  eventCount=eventsPushed;video.window=(SDL_Window*)1;
  buttonEvent(PPT_BTN_X,true);assert(mouseDownForwarded[PPT_BTN_X]);
  releaseButton(PPT_BTN_X);assert(eventsPushed==eventCount+2);
  ui.diskOpScreenShown=false;
  buttonEvent(PPT_BTN_MENU,true);directionAction(3);releaseButton(PPT_BTN_MENU);
  assert(!ppt_full_pattern_active());
  buttonEvent(PPT_BTN_MENU,true);buttonEvent(PPT_BTN_START,true);
  releaseButton(PPT_BTN_START);releaseButton(PPT_BTN_MENU);
  assert(!ppt_full_pattern_active());editor.songPlaying=false;
  buttonEvent(PPT_BTN_R3,true);releaseButton(PPT_BTN_R3);
  assert(ppt_full_pattern_active());
  mouse.x=64;mouse.y=40;fullPatternClick();assert(song->currRow==3 && cursor.pos==2);
  buttonEvent(PPT_BTN_R3,true);releaseButton(PPT_BTN_R3);
  assert(!ppt_full_pattern_active());song->currRow=0;cursor.pos=0;cursor.channel=0;
  viewRenders=samplerOpens=samplerExits=diskRenders=0;
  playCalls=stopCalls=pointerModeCalls=statusCalls=0;
  /* EDITSKIP affects plain A, but never A+direction or A+B chords. */
  editor.editMoveAdd=4;song->currRow=0;cursor.pos=0;
  buttonEvent(PPT_BTN_A,true);assert(song->currRow==0);releaseButton(PPT_BTN_A);
  assert(song->currRow==4 && patt[0].period!=0);
  buttonEvent(PPT_BTN_A,true);releaseButton(PPT_BTN_A);assert(song->currRow==8 && patt[16].period!=0);
  buttonEvent(PPT_BTN_A,true);directionAction(3);releaseButton(PPT_BTN_A);assert(song->currRow==8);
  buttonEvent(PPT_BTN_A,true);buttonEvent(PPT_BTN_B,true);releaseButton(PPT_BTN_B);releaseButton(PPT_BTN_A);
  assert(song->currRow==8);
  editor.editMoveAdd=16;song->currRow=60;
  buttonEvent(PPT_BTN_A,true);releaseButton(PPT_BTN_A);assert(song->currRow==12);
  buttonEvent(PPT_BTN_L3,true);releaseButton(PPT_BTN_L3);
  assert(editor.currMode==MODE_RECORD && editor.songPlaying && playRow==DONT_SET_ROW);
  int recordRow=song->currRow;
  buttonEvent(PPT_BTN_A,true);releaseButton(PPT_BTN_A);assert(recordCalls==1 && song->currRow==recordRow);
  buttonEvent(PPT_BTN_L3,true);releaseButton(PPT_BTN_L3);assert(!editor.songPlaying);
  editor.editMoveAdd=0;song->currRow=0;cursor.pos=0;
  memset(patt,0,sizeof(patt));previewCalls=recordCalls=0;lastPitch=24;
  /* v0.3.24: note erase includes sample, FX preserved; rectangle is exact. */
  patt[0]=(note_t){.period=428,.sample=17,.command=0xA,.param=0xBC};
  erase();assert(patt[0].period==0 && patt[0].sample==0);
  assert(patt[0].command==0xA && patt[0].param==0xBC);
  patt[0]=(note_t){.period=428,.sample=17,.command=0xA,.param=0xBC};
  patt[4]=patt[0];patt[8]=patt[0];
  clip.kind=PPT_CLIP_PATTERN;clip.cells[0][0].period=123;
  beginSelection();song->currRow=1;cursor.pos=2;
  int prevEvents=eventsPushed;
  buttonEvent(PPT_BTN_Y,true);releaseButton(PPT_BTN_Y);
  assert(!selecting && eventsPushed==prevEvents);
  assert(patt[0].period==0 && patt[0].sample==0 && patt[0].command==0xA && patt[0].param==0xBC);
  assert(patt[4].period==0 && patt[4].sample==0 && patt[4].param==0xBC);
  assert(patt[8].period==428 && patt[8].sample==17);
  assert(clip.kind==PPT_CLIP_PATTERN && clip.cells[0][0].period==123);
  /* Reverse direction and single-nibble deletion must preserve neighbors. */
  song->currRow=1;cursor.pos=5;beginSelection();song->currRow=0;
  eraseSelection();assert(patt[0].param==0xB0 && patt[4].param==0xB0);
  assert(patt[8].param==0xBC);
  memset(patt,0,sizeof(patt));memset(&clip,0,sizeof(clip));
  song->currRow=0;cursor.pos=0;state.modified=false;
  /* v0.3.17: SHIFT (physical SELECT) + D-pad pattern EDITSKIP.
     Native PT2 stores the speed at editor.editMoveAdd = 0..9. */
  assert(editor.editMoveAdd==0);
  ui.samplerScreenShown=false; ui.posEdScreenShown=false;
  ui.diskOpScreenShown=false; ui.editOpScreenShown=false;
  ui.aboutScreenShown=false; ui.askBoxShown=false;
  ui.editTextFlag=false;
  state.currRow=10; cursor.pos=0;
  int initialPlayCount=playCalls;
  buttonEvent(PPT_BTN_SELECT,true);
  directionAction(3);assert(editor.editMoveAdd==1);
  assert(ui.updateTrackerFlags && !strcmp(lastStatusMessage,"EDITSKIP = 1"));
  directionAction(3);directionAction(3);directionAction(3);
  assert(editor.editMoveAdd==4 && !strcmp(lastStatusMessage,"EDITSKIP = 4"));
  directionAction(1);assert(state.currRow==14);
  directionAction(0);assert(state.currRow==10);
  directionAction(2);assert(editor.editMoveAdd==3);
  assert(!strcmp(lastStatusMessage,"EDITSKIP = 3"));
  state.currRow=63;directionAction(1);assert(state.currRow==2);
  state.currRow=1;directionAction(0);assert(state.currRow==62);
  for(int i=0;i<20;i++)directionAction(3);
  assert(editor.editMoveAdd==16);
  directionAction(3);assert(editor.editMoveAdd==16);
  for(int i=0;i<20;i++)directionAction(2);
  assert(editor.editMoveAdd==0);
  state.currRow=5;directionAction(1);assert(state.currRow==6);
  assert(playCalls==initialPlayCount);
  releaseButton(PPT_BTN_SELECT);
  assert(!ppt_live_edit_active());
  /* Sample RANGE has separate boundaries. In SAMPLER:
     SHIFT/SELECT+LEFT or RIGHT moves START; MENU+LEFT or RIGHT moves END.
     Neither shortcut changes PATTERN EDITSKIP. */
  editor.editMoveAdd=4;
  editor.currSample=0;state.samples[0].length=200;
  ui.samplerScreenShown=true;
  editor.markStartOfs=40;editor.markEndOfs=100;
  buttonEvent(PPT_BTN_SELECT,true);
  directionAction(2);assert(editor.markStartOfs==39 && editor.markEndOfs==100);
  directionAction(3);assert(editor.markStartOfs==40 && editor.markEndOfs==100);
  assert(editor.editMoveAdd==4);
  releaseButton(PPT_BTN_SELECT);
  buttonEvent(PPT_BTN_MENU,true);
  directionAction(2);assert(editor.markStartOfs==40 && editor.markEndOfs==99);
  directionAction(3);assert(editor.markStartOfs==40 && editor.markEndOfs==100);
  assert(editor.editMoveAdd==4);
  releaseButton(PPT_BTN_MENU);
  ui.samplerScreenShown=false;
  /* POSITION EDIT still browses positions, never touches EDITSKIP. */
  ui.posEdScreenShown=true; state.header.songLength=16;
  state.currPos=7;
  buttonEvent(PPT_BTN_SELECT,true);
  directionAction(1);
  assert(state.currPos==8 && editor.editMoveAdd==4);
  releaseButton(PPT_BTN_SELECT);
  ui.posEdScreenShown=false;state.currPos=0;
  /* Active pattern-copy mark extends by one row, NOT the edit skip. */
  state.currPattern=0;state.currRow=8;cursor.pos=0;cursor.channel=0;
  beginSelection();assert(selecting);
  buttonEvent(PPT_BTN_SELECT,true);
  directionAction(1);
  assert(state.currRow==9 && editor.editMoveAdd==4);
  releaseButton(PPT_BTN_SELECT);
  cancelSelection();state.currRow=0;
  /* During LIVE EDIT, changing the editor ROW must leave the playhead
     untouched while playback continues. */
  editor.songPlaying=true;atomic_store(&liveEditEnabled,true);
  int frozenPlaybackRow=state.row;
  state.currRow=8;
  buttonEvent(PPT_BTN_SELECT,true);
  directionAction(1);
  assert(state.currRow==12 && state.row==frozenPlaybackRow);
  assert(editor.songPlaying && ppt_live_edit_active());
  releaseButton(PPT_BTN_SELECT);
  /* Releasing SELECT after using D-pad must NOT toggle LIVE EDIT. */
  assert(ppt_live_edit_active());
  editor.songPlaying=false;atomic_store(&liveEditEnabled,false);
  state.currRow=0;
  editor.editMoveAdd=0; /* Remaining legacy tests intentionally edit in-place. */
  /* Reset test-only redraw counters after exercising view precedence. */
  samplerRangeRenders=0;samplerPosUpdates=0;
  /* v0.3.16 sampler key combinations do not paste/delete pattern cells. */
  ui.samplerScreenShown=true;
  editor.currSample=0;
  state.samples[0].length=200;
  editor.markStartOfs=40; editor.markEndOfs=100;
  buttonEvent(PPT_BTN_A,true);releaseButton(PPT_BTN_A);
  assert(samplerWholeAuditions==1 && samplerRangeAuditions==0);
  buttonEvent(PPT_BTN_SELECT,true);
  buttonEvent(PPT_BTN_A,true);releaseButton(PPT_BTN_A);
  assert(samplerRangeAuditions==1 && samplerWholeAuditions==1);
  /* Both RANGE edges move left and right by 1 per tap. */
  fakeTicks=0;
  directionEvent(2,true); assert(editor.markStartOfs==39 && editor.markEndOfs==100);
  directionEvent(2,false);
  directionEvent(3,true); assert(editor.markStartOfs==40 && editor.markEndOfs==100);
  directionEvent(3,false);
  releaseButton(PPT_BTN_SELECT);
  buttonEvent(PPT_BTN_MENU,true);
  directionEvent(2,true); assert(editor.markStartOfs==40 && editor.markEndOfs==99);
  directionEvent(2,false);
  directionEvent(3,true); assert(editor.markStartOfs==40 && editor.markEndOfs==100);
  directionEvent(3,false);
  assert(samplerRangeRenders==4 && samplerPosUpdates==4);
  /* MENU has priority over SELECT; only the END boundary moves. */
  buttonEvent(PPT_BTN_SELECT,true);
  directionAction(3);
  assert(editor.markStartOfs==40 && editor.markEndOfs==101);
  releaseButton(PPT_BTN_SELECT);
  /* Hold acceleration 1 -> 2 -> 5 -> 10 -> 50 -> 100.
     A fresh press always restarts at 1. */
  editor.markStartOfs=50;editor.markEndOfs=70;
  state.samples[0].length=1000;
  fakeTicks=10000;
  directionEvent(3,true);assert(editor.markStartOfs==50 && editor.markEndOfs==71);
  fakeTicks=10100;repeatDirections(fakeTicks);assert(editor.markEndOfs==71);
  fakeTicks=10281;repeatDirections(fakeTicks);assert(editor.markEndOfs==72);
  fakeTicks=10451;repeatDirections(fakeTicks);assert(editor.markEndOfs==74);
  fakeTicks=10901;repeatDirections(fakeTicks);assert(editor.markEndOfs==79);
  fakeTicks=11351;repeatDirections(fakeTicks);assert(editor.markEndOfs==89);
  fakeTicks=11801;repeatDirections(fakeTicks);assert(editor.markEndOfs==139);
  fakeTicks=12301;repeatDirections(fakeTicks);assert(editor.markEndOfs==239);
  directionEvent(3,false);
  fakeTicks=12400;directionEvent(3,true);
  assert(editor.markEndOfs==240);
  directionEvent(3,false);
  /* MENU+LEFT contracts END but cannot cross START+2. */
  editor.markStartOfs=90;editor.markEndOfs=120;
  fakeTicks=13000;directionEvent(2,true);assert(editor.markEndOfs==119);
  fakeTicks=15301;repeatDirections(fakeTicks);assert(editor.markEndOfs==92);
  directionEvent(2,false);
  releaseButton(PPT_BTN_MENU);
  /* Accelerated SELECT+RIGHT clamps at END-2 without moving END. */
  buttonEvent(PPT_BTN_SELECT,true);
  editor.markStartOfs=70;editor.markEndOfs=90;
  fakeTicks=16000;directionEvent(3,true);assert(editor.markStartOfs==71);
  fakeTicks=18301;repeatDirections(fakeTicks);assert(editor.markStartOfs==88);
  directionEvent(3,false);
  assert(editor.markEndOfs==90);
  /* SELECT+LEFT uses one-byte precision near zero. */
  editor.markStartOfs=1;editor.markEndOfs=90;
  directionAction(2);assert(editor.markStartOfs==0 && editor.markEndOfs==90);
  releaseButton(PPT_BTN_SELECT);
  /* Repeating direction without SELECT must not alter sample selection. */
  releaseButton(PPT_BTN_SELECT);
  int32_t before=editor.markEndOfs;
  fakeTicks=16000;directionEvent(3,true);fakeTicks=18000;repeatDirections(fakeTicks);
  assert(editor.markEndOfs==before);directionEvent(3,false);
  buttonEvent(PPT_BTN_SELECT,true);
  releaseButton(PPT_BTN_SELECT);
  buttonEvent(PPT_BTN_B,true);releaseButton(PPT_BTN_B);
  assert(samplerNewSlots==1 && samplerShiftWasHeld && !keyb.shiftPressed);
  editor.markStartOfs=-1;
  buttonEvent(PPT_BTN_B,true);releaseButton(PPT_BTN_B);
  assert(samplerNewSlots==1 && samplerErrs==1);
  state.samples[0].length=200;
  editor.markStartOfs=0;editor.markEndOfs=200;
  buttonEvent(PPT_BTN_SELECT,true);directionAction(2);
  directionAction(3);assert(editor.markStartOfs==1 && editor.markEndOfs==200);
  releaseButton(PPT_BTN_SELECT);
  buttonEvent(PPT_BTN_MENU,true);
  directionAction(3);assert(editor.markEndOfs==200);
  editor.markStartOfs=193;editor.markEndOfs=195;
  directionAction(3);assert(editor.markEndOfs==196);
  assert(editor.markStartOfs==193);
  directionAction(2);assert(editor.markEndOfs==195);
  releaseButton(PPT_BTN_MENU);
  /* Minimum 2-byte selection maintained at both boundaries. */
  buttonEvent(PPT_BTN_SELECT,true);
  directionAction(3);assert(editor.markStartOfs==193 && editor.markEndOfs==195);
  releaseButton(PPT_BTN_SELECT);
  /* Invalid/empty selection must not alter offsets. */
  editor.markStartOfs=-1;editor.markEndOfs=195;
  buttonEvent(PPT_BTN_MENU,true);
  int errsBefore=samplerErrs;
  directionAction(2);assert(samplerErrs==errsBefore+1 && editor.markEndOfs==195);
  releaseButton(PPT_BTN_MENU);
  /* RESAMPLING NOTE: native sampler selection 0..35 (C-1..B-3).
     G/D = semitone, L/R = octave. No sample audio is modified here. */
  ui.changingSmpResample=true; editor.resampleNote=24;
  int previewsBefore=samplerWholeAuditions;
  directionAction(0);assert(editor.resampleNote==25 && ui.updateResampleNote);
  directionAction(3);assert(editor.resampleNote==35);
  directionAction(0);assert(editor.resampleNote==35); /* hard max */
  directionAction(2);assert(editor.resampleNote==23);
  directionAction(1);assert(editor.resampleNote==22);
  buttonEvent(PPT_BTN_X,true);releaseButton(PPT_BTN_X);
  assert(!ui.changingSmpResample && editor.resampleNote==22);
  assert(previousPointerModeCalls==1 && previousStatusCalls==1);
  assert(samplerWholeAuditions==previewsBefore);
  ui.changingSmpResample=true;
  directionAction(1);assert(editor.resampleNote==21);
  buttonEvent(PPT_BTN_B,true);releaseButton(PPT_BTN_B);
  assert(!ui.changingSmpResample && editor.resampleNote==22); /* cancel */
  ui.changingSmpResample=true;editor.resampleNote=0;
  directionAction(1);assert(editor.resampleNote==0); /* hard min */
  buttonEvent(PPT_BTN_Y,true);releaseButton(PPT_BTN_Y);
  assert(!ui.changingSmpResample && editor.resampleNote==0);
  assert(previousPointerModeCalls==3 && previousStatusCalls==3);
  assert(samplerNewSlots==1); /* RESAMPLE NOTE B never copies a RANGE */
  ui.samplerScreenShown=false;
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
  /* v0.3.15: mouse speed doubles only when R1+D-pad is held. */
  assert(R1_DPAD_MULTIPLIER==2);
  int f0=0,f1=0, m0=0,m1=0;
  for(int i=0;i<10;i++){
    m0+=scaledMousePixels(MOUSE_STEP,&f0,1);
    m1+=scaledMousePixels(MOUSE_STEP*R1_DPAD_MULTIPLIER,&f1,1);
  }
  assert(m0==42 && m1==84);
  /* Right analog has independent axes, directional thresholds and recenter latch. */
  axis[ABS_RY]=(axis_t){.min=-4096,.max=4096,.value=0,.valid=true,.seen=true};
  axis[ABS_RZ]=(axis_t){.min=-4096,.max=4096,.value=0,.valid=true,.seen=true};
  ui.posEdScreenShown=ui.diskOpScreenShown=ui.samplerScreenShown=false;
  ui.editOpScreenShown=ui.askBoxShown=ui.editTextFlag=false;
  rightStickArmed=true;editor.songPlaying=true;
  int oldPlay=playCalls, oldStop=stopCalls;
  axis[ABS_RY].value=-4096;updateRightStickViews();
  assert(ui.posEdScreenShown && viewRenders==1);
  axis[ABS_RY].value=4096;updateRightStickViews();
  assert(ui.posEdScreenShown && viewRenders==1); /* no repeat until center */
  axis[ABS_RY].value=0;updateRightStickViews();
  axis[ABS_RY].value=4096;updateRightStickViews();
  assert(!ui.posEdScreenShown && viewRenders==2);
  axis[ABS_RY].value=0;updateRightStickViews();
  axis[ABS_RZ].value=-4096;updateRightStickViews();
  assert(ui.diskOpScreenShown && diskRenders==1);
  axis[ABS_RZ].value=0;updateRightStickViews();
  axis[ABS_RZ].value=4096;updateRightStickViews();
  assert(ui.samplerScreenShown && !ui.diskOpScreenShown && samplerOpens==1);
  axis[ABS_RZ].value=0;updateRightStickViews();
  axis[ABS_RY].value=-4096;axis[ABS_RZ].value=4096;
  int oldView=viewRenders;updateRightStickViews();
  assert(viewRenders==oldView); /* diagonal must not accidentally switch */
  axis[ABS_RY].value=axis[ABS_RZ].value=0;updateRightStickViews();
  axis[ABS_RY].value=4096;updateRightStickViews();
  assert(!ui.samplerScreenShown && samplerExits==1);
  axis[ABS_RY].value=0;updateRightStickViews();
  ui.editTextFlag=true;axis[ABS_RY].value=-4096;updateRightStickViews();
  assert(!ui.posEdScreenShown && viewRenders==oldView+1);
  ui.editTextFlag=false;axis[ABS_RY].value=0;updateRightStickViews();
  assert(playCalls==oldPlay && stopCalls==oldStop && editor.songPlaying);
  editor.songPlaying=false;
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
  fullPattern=false;ui.diskOpScreenShown=false;
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
  /* v0.3.12: SAMPLE NAME has the same carousel with a 22-cell limit.
     MOD, SAMPLE names only. Numeric/POS/DISK OP must never be intercepted. */
  ui.editTextFlag=true;textEdit.object=PTB_SAMPLENAME;
  editor.currSample=3;
  char *sampleName=state.samples[3].text;
  memset(sampleName,0,23);
  textEdit.textStartPtr=sampleName;
  textEdit.textEndPtr=sampleName+21;
  textEdit.textPtr=sampleName;
  assert(currentNameEdit(NULL));
  directionAction(0);assert(!strcmp(sampleName,"a") && ui.updateCurrSampleName);
  buttonEvent(PPT_BTN_X,true);releaseButton(PPT_BTN_X);
  assert(textEdit.textPtr==sampleName+1);
  directionAction(0);assert(!strcmp(sampleName,"aa"));
  buttonEvent(PPT_BTN_Y,true);releaseButton(PPT_BTN_Y);
  assert(!ui.editTextFlag && textCommitted==4 && !strcmp(sampleName,"aa"));
  /* Removing a character from the middle must shift suffix, not just clear it. */
  ui.editTextFlag=true;textEdit.object=PTB_SAMPLENAME;
  strcpy(sampleName,"tracker");textEdit.textPtr=sampleName+4;
  buttonEvent(PPT_BTN_B,true);releaseButton(PPT_BTN_B);
  assert(!strcmp(sampleName,"traker") && textEdit.textPtr==sampleName+3);
  /* 22-char name is guaranteed to remain terminated. */
  memset(sampleName,'x',22);sampleName[22]='\0';
  textEdit.textPtr=sampleName+21;
  directionAction(0);assert(sampleName[22]=='\0');
  buttonEvent(PPT_BTN_X,true);releaseButton(PPT_BTN_X);
  assert(textEdit.textPtr==sampleName+21);
  buttonEvent(PPT_BTN_Y,true);releaseButton(PPT_BTN_Y);
  assert(!ui.editTextFlag && textCommitted==5);
  /* Out-of-scope text fields retain original PT2 behavior, including
     numeric and DISK OP input. */
  ui.editTextFlag=true;textEdit.object=PTB_SAMPLENAME+1;
  textEdit.textStartPtr=sampleName;textEdit.textPtr=sampleName;
  assert(!currentNameEdit(NULL));
  textEdit.object=PTB_SONGNAME;textEdit.type=TEXT_EDIT_STRING;
  textEdit.textStartPtr=sampleName; /* wrong storage: must not activate */
  assert(!currentNameEdit(NULL));
  textEdit.type=99;textEdit.object=PTB_SAMPLENAME;
  assert(!currentNameEdit(NULL));
  ui.editTextFlag=false;

  memset(editor.muted,0,sizeof(editor.muted));
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
  editor.songPlaying=true;editor.playMode=PLAY_MODE_NORMAL;atomic_store(&posBrowseActive,true);
  assert(ppt_position_browse_active());
  buttonEvent(PPT_BTN_SELECT,true);buttonEvent(PPT_BTN_B,true);
  releaseButton(PPT_BTN_B);releaseButton(PPT_BTN_SELECT);
  assert(posMarking && posMarkStart==8 && posMarkEnd==8);
  int beforeDirection=(int)state.currPos;
  directionAction(1);directionAction(1);directionAction(1);
  assert(posMarkEnd==11 && (int)state.currPos==beforeDirection+3 && editor.songPlaying && ppt_position_detached());
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
  editor.songPlaying=false;atomic_store(&posDetached,false);atomic_store(&posBrowseActive,false);
  ui.posEdScreenShown=false;
  /* Clearing a loaded module invalidates the in-session selection. */
  clearPosLoop();assert(ppt_loop_entry_style(9)==0 && ppt_loop_redirect(12,16)==12);
  assert(state.header.patternTable[8]==18 && state.header.patternTable[9]==19);

  puts("PPT v0.3.12: PT2 v0.3.10 playback + MOD/SAMPLE name carousel OK");
  /* POSITION range clipboard survives loop clear and inserts before destination. */
  memset(held,0,sizeof(held));editor.songPlaying=false;atomic_store(&posDetached,false);memset(editor.muted,0,sizeof(editor.muted));
  ui.posEdScreenShown=true;ui.diskOpScreenShown=false;ui.samplerScreenShown=false;
  ui.askBoxShown=false;ui.editTextFlag=false;song->header.songLength=5;
  for(int i=0;i<5;i++)song->header.patternTable[i]=(uint16_t)(10+i);
  song->currPos=1;beginPosMark();movePosMark(1);confirmPosMark();
  assert(posClipCount==2 && posClip[0]==11 && posClip[1]==12);
  buttonEvent(PPT_BTN_Y,true);releaseButton(PPT_BTN_Y);song->currPos=3;
  buttonEvent(PPT_BTN_SELECT,true);buttonEvent(PPT_BTN_A,true);
  releaseButton(PPT_BTN_A);releaseButton(PPT_BTN_SELECT);
  const int expectedOrders[]={10,11,12,11,12,13,14};
  assert(song->header.songLength==7);
  for(int i=0;i<7;i++)assert(song->header.patternTable[i]==expectedOrders[i]);
  editor.songPlaying=true;pastePositionRange();assert(song->header.songLength==7);
  editor.songPlaying=false;song->header.songLength=127;pastePositionRange();
  assert(song->header.songLength==127);
  song->header.songLength=7;
  song->currPos=6;posAppend=false;directionAction(1);
  assert(ppt_position_cursor()==7 && song->currPos==6);
  pastePositionRange();assert(editor.currPosEdPattDisp==&song->header.patternTable[7]);assert(song->header.songLength==9 && song->header.patternTable[7]==11 && song->header.patternTable[8]==12);
  directionAction(3);assert(song->header.patternTable[7]==12);
  directionAction(2);assert(song->header.patternTable[7]==11);
  ppt_position_set_cursor(9);directionAction(3);
  assert(song->header.songLength==10 && song->header.patternTable[9]==1);
  posAppend=false;song->currPos=2;song->header.patternTable[2]=17;song->currPattern=3;
  editor.songPlaying=false;startPlayback(PPT_PLAY_PATTERN);
  assert(editor.playMode==PLAY_MODE_PATTERN && playPatt==17 && playRow==0 && song->currPos==2);
  assert(ppt_position_pattern_active());
  browsePos(1);assert(song->currPos==3);
  startPlayback(PPT_PLAY_PATTERN);assert(!editor.songPlaying && !ppt_position_pattern_active());
  clearPosLoop();startPlayback(PPT_PLAY_SONG);
  assert(editor.playMode==PLAY_MODE_NORMAL && playPos==3 && !ppt_position_pattern_active());
  startPlayback(PPT_PLAY_SONG);
  puts("POSITION clipboard: selected range, loop-clear retention, shifted insertion, playback/full guards OK");
  /* v0.3.46: POSITION actions must never edit hidden pattern data. */
  static note_t posPatterns[MAX_PATTERNS][256],beforePatterns[MAX_PATTERNS][256];
  for(int p=0;p<MAX_PATTERNS;p++)song->patterns[p]=posPatterns[p];
  memset(posPatterns,0,sizeof(posPatterns));memset(qaNames,0,sizeof(qaNames));
  memset(held,0,sizeof(held));memset(editor.muted,0,sizeof(editor.muted));
  clearPosLoop();posAppend=false;atomic_store(&posDetached,false);
  ui.posEdScreenShown=true;editor.songPlaying=false;editor.currMode=MODE_IDLE;
  song->header.songLength=4;memset(song->header.patternTable,0,sizeof(song->header.patternTable));
  for(int i=0;i<4;i++)song->header.patternTable[i]=(uint16_t)(10+i);
  ppt_position_set_cursor(1);posPatterns[11][0]=(note_t){.period=428,.sample=3,.command=9,.param=16};
  memcpy(beforePatterns,posPatterns,sizeof(posPatterns));int posPreviewsBefore=previewCalls;
  buttonEvent(PPT_BTN_A,true);releaseButton(PPT_BTN_A);
  insert();erase();edit(0); /* Defense even if a future caller bypasses dispatch. */
  buttonEvent(PPT_BTN_L3,true);releaseButton(PPT_BTN_L3);
  assert(!memcmp(beforePatterns,posPatterns,sizeof(posPatterns)) && previewCalls==posPreviewsBefore);
  assert(!selectionAllowed() && editor.currMode==MODE_IDLE && !aAdvancePending);
  /* Both orders of the A+B chord delete exactly one entry, never note data. */
  buttonEvent(PPT_BTN_B,true);buttonEvent(PPT_BTN_A,true);
  releaseButton(PPT_BTN_A);releaseButton(PPT_BTN_B);
  assert(song->header.songLength==3 && song->header.patternTable[1]==12 && song->header.patternTable[2]==13);
  assert(song->header.patternTable[3]==11); /* Dormant ID retains native MOD bytes. */
  assert(!memcmp(beforePatterns,posPatterns,sizeof(posPatterns)));
  ppt_position_set_cursor(2);buttonEvent(PPT_BTN_A,true);buttonEvent(PPT_BTN_B,true);
  releaseButton(PPT_BTN_B);releaseButton(PPT_BTN_A);
  assert(song->header.songLength==2 && song->currPos==1 && song->header.patternTable[2]==13);
  deletePosition();assert(song->header.songLength==1 && song->header.patternTable[1]==12);
  deletePosition();assert(song->header.songLength==1 && !strcmp(lastStatusMessage,"KEEP ONE POSITION"));
  ppt_position_set_cursor(1);deletePosition();assert(song->header.songLength==1);
  posAppend=false;editor.songPlaying=true;deletePosition();assert(song->header.songLength==1);
  editor.songPlaying=false;
  /* Mute is shared with native buttons; no shoulder changes sample or pattern. */
  const int shoulders[4]={PPT_BTN_L1,PPT_BTN_L2,PPT_BTN_R1,PPT_BTN_R2};
  int sampleBefore=editor.currSample,pattBefore=song->currPattern;
  for(int ch=0;ch<4;ch++){
    buttonEvent(PPT_BTN_SELECT,true);buttonEvent(shoulders[ch],true);
    releaseButton(PPT_BTN_SELECT);releaseButton(shoulders[ch]);
    assert(editor.muted[ch]);
    buttonEvent(PPT_BTN_SELECT,true);buttonEvent(shoulders[ch],true);
    releaseButton(shoulders[ch]);releaseButton(PPT_BTN_SELECT);
    assert(!editor.muted[ch]);
  }
  assert(editor.currSample==sampleBefore && song->currPattern==(unsigned)pattBefore);
  ui.posEdScreenShown=false;
  buttonEvent(PPT_BTN_SELECT,true);buttonEvent(PPT_BTN_L2,true);
  releaseButton(PPT_BTN_L2);releaseButton(PPT_BTN_SELECT);assert(editor.muted[1] && editor.currSample==sampleBefore);
  memset(editor.muted,0,sizeof(editor.muted));ui.posEdScreenShown=true;
  /* Four selected entries -> four NEW, independent IDs, even repeated sources. */
  memset(song->header.patternTable,0,sizeof(song->header.patternTable));song->header.songLength=5;
  const uint16_t sources[5]={9,12,9,17,18};memcpy(song->header.patternTable,sources,sizeof(sources));
  for(int p=9;p<=18;p++)for(int row=0;row<64;row++)for(int ch=0;ch<4;ch++)
    posPatterns[p][row*4+ch]=(note_t){.period=(uint16_t)(400+p),.sample=(uint8_t)(ch+1),.command=0xA,.param=(uint8_t)row};
  posPatterns[2][0].period=428;strcpy(qaNames[3],"RESERVED");strcpy(qaNames[9],"INTRO");
  ppt_position_set_cursor(0);beginPosMark();movePosMark(1);movePosMark(1);movePosMark(1);
  buttonEvent(PPT_BTN_MENU,true);buttonEvent(PPT_BTN_A,true);
  releaseButton(PPT_BTN_A);releaseButton(PPT_BTN_MENU);
  assert(!editor.muted[2] && posClipCount==4 && posClipNew && !posMarking);
  memcpy(beforePatterns,posPatterns,sizeof(posPatterns));
  ppt_position_set_cursor(1);pastePositionRange();assert(song->header.songLength==9);
  for(int i=0;i<4;i++){
    int dst=song->header.patternTable[1+i];assert(dst!=0 && dst!=2 && dst!=3);
    for(int j=0;j<5;j++)assert(dst!=sources[j]);
    for(int j=0;j<i;j++)assert(dst!=song->header.patternTable[1+j]);
    assert(!memcmp(posPatterns[dst],beforePatterns[sources[i]],sizeof(posPatterns[dst])));
  }
  assert(!strcmp(qaNames[song->header.patternTable[1]],"INTRO"));
  for(int i=0;i<5;i++)assert(!memcmp(posPatterns[sources[i]],beforePatterns[sources[i]],sizeof(posPatterns[0])));
  assert(song->header.patternTable[5]==12 && song->header.patternTable[8]==18);
  /* Ordinary copy with mute switches to new snapshots, with full effects omitted. */
  editor.muted[1]=editor.muted[3]=true;ppt_position_set_cursor(0);beginPosMark();confirmPosMark();
  assert(posClipCount==1 && posClipNew);
  memset(editor.muted,0,sizeof(editor.muted));ppt_position_set_cursor(song->header.songLength);pastePositionRange();
  int dst=song->header.patternTable[song->header.songLength-1];assert(dst!=9);
  for(int row=0;row<64;row++)for(int ch=0;ch<4;ch++){
    note_t *v=&posPatterns[dst][row*4+ch];
    if(ch==1 || ch==3)assert(!v->period && !v->sample && !v->command && !v->param);
    else assert(!memcmp(v,&posPatterns[9][row*4+ch],sizeof(*v)));
  }
  /* No free patterns / full order list: abort all writes, including names. */
  for(int p=0;p<MAX_PATTERNS;p++)posPatterns[p][0].period=428;
  memcpy(beforePatterns,posPatterns,sizeof(posPatterns));uint16_t beforeOrders[128];
  memcpy(beforeOrders,song->header.patternTable,sizeof(beforeOrders));int oldLen=song->header.songLength;
  ppt_position_set_cursor(1);pastePositionRange();assert(song->header.songLength==oldLen);
  assert(!memcmp(beforeOrders,song->header.patternTable,sizeof(beforeOrders)) && !memcmp(beforePatterns,posPatterns,sizeof(posPatterns)));
  assert(!strcmp(lastStatusMessage,"NO EMPTY PATTERNS"));
  song->header.songLength=128;pastePositionRange();assert(song->header.songLength==128 && !strcmp(lastStatusMessage,"POSITION FULL"));
  /* SELECT detaches POSITION only. Browsing neither seeks nor edits the player. */
  song->header.songLength=9;posAppend=false;clearPosLoop();ppt_position_set_cursor(2);
  editor.songPlaying=true;editor.playMode=PLAY_MODE_NORMAL;atomic_store(&liveEditEnabled,false);atomic_store(&posDetached,false);
  ppt_set_position_playhead(2);int oldStops=stopCalls,oldPlays=playCalls;
  buttonEvent(PPT_BTN_SELECT,true);releaseButton(PPT_BTN_SELECT);
  assert(ppt_position_detached() && !ppt_live_edit_active());
  directionAction(1);directionAction(1);ppt_set_position_playhead(3);
  assert(song->currPos==4 && ppt_position_playhead()==3 && playCalls==oldPlays && stopCalls==oldStops);
  startPlayback(PPT_PLAY_PATTERN);
  assert(editor.songPlaying && editor.playMode==PLAY_MODE_NORMAL && playPos==4 && !ppt_position_detached());
  startPlayback(PPT_PLAY_PATTERN);assert(!editor.songPlaying);
  /* Both orders of MENU+A prepare a copy and never paste or change mute. */
  ppt_position_set_cursor(0);beginPosMark();confirmPosMark();posClipNew=false;
  int unchangedLength=song->header.songLength;
  buttonEvent(PPT_BTN_A,true);buttonEvent(PPT_BTN_MENU,true);
  releaseButton(PPT_BTN_MENU);releaseButton(PPT_BTN_A);
  assert(posClipNew && song->header.songLength==unchangedLength && !editor.muted[2]);
  /* Returning to FOLLOW from APPEND must remove the virtual append cursor. */
  editor.songPlaying=true;editor.playMode=PLAY_MODE_NORMAL;atomic_store(&posDetached,true);
  ppt_position_set_cursor(song->header.songLength);assert(posAppend);
  buttonEvent(PPT_BTN_SELECT,true);releaseButton(PPT_BTN_SELECT);
  assert(!ppt_position_detached() && !posAppend && !ppt_live_edit_active());
  editor.songPlaying=false;
  puts("POSITION v0.3.46: isolated editing, deletion, mute chords, masked/new snapshots, rollback and detached launch OK");

  char patternName[16];memset(patternName,'a',15);patternName[15]=0;
  ui.editTextFlag=true;textEdit.type=TEXT_EDIT_STRING;textEdit.object=PTB_PE_PATTNAME;
  textEdit.textStartPtr=patternName;textEdit.textEndPtr=patternName+15;
  textEdit.textPtr=patternName+15;
  assert(currentNameEdit(NULL));nameCycle(1);
  assert(textEdit.textPtr==patternName+14 && patternName[15]==0);
  nameFinish();assert(!ui.editTextFlag);
  return 0;
}

bool ppt_bpm_editing(void){return false;}
void ppt_bpm_next(void){}
void ppt_bpm_previous(void){}
void ppt_bpm_direction(int d){(void)d;}

void ppt_browser_toggle_mode(void){}
void modSetPos(int16_t pos,int16_t row){if(pos>=0)song->currPos=(uint16_t)pos;if(row>=0)song->currRow=(uint16_t)row;}
