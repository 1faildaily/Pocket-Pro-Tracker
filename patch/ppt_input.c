/*
 * Pocket Pro Tracker controller patch v0.3.31 / PT2 clone 1.92
 * Add-on module, does not replace the original renderer, audio engine or file format.
 * BSD-3-Clause, 2026. Upstream 8bitbubsy BSD-3-Clause credits preserved.
 * Raw key mapping confirmed on one RG35XX Pro with muOS (controls.json).
 * DEVICE TEST REQUIRED: the analog axes, display backend, audio and editing semantics
 * still have to be validated on real hardware.
 */
#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
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
#include "pt2_audio.h"
#include "pt2_helpers.h"
#include "pt2_textout.h"
#include "pt2_textedit.h"
#include "pt2_sampler.h"
#include "pt2_diskop.h"
#include "pt2_posed.h"
#include "ppt_input.h"
#include "ppt_effects.h"
#include "ppt_sample_browser.h"

#define DEVICES 16
#define INITIAL_DELAY 280
#define REPEAT_DELAY 85
#define MOUSE_STEP 6
#define R1_DPAD_MULTIPLIER 2
#define RIGHT_STICK_TRIGGER 30000
#define RIGHT_STICK_CROSS_MAX 15000
#define RIGHT_STICK_REARM 13000
#define MOUSE_DEADZONE 4200
/* Universal speed multiplier for ALL virtual-mouse input paths.
   7/10 = exactly 30% slower than v0.3.4 at any analog deflection
   and in R1+D-pad / askBox D-pad navigation. Keep fractional pixels
   so slower movement remains smooth and never gets rounded to zero. */
#define MOUSE_SPEED_NUM 7
#define MOUSE_SPEED_DEN 10
#define MOUSE_ANALOG_DEN 1024
#define MOUSE_FINE_LINEAR 205
#define MOUSE_FINE_CURVE 2458
#define SCREEN_MOUSE_W 320
#define SCREEN_MOUSE_H 255
#define C_COUNT 13

enum {PPT_BTN_SELECT, PPT_BTN_START, PPT_BTN_L1, PPT_BTN_L2, PPT_BTN_R1, PPT_BTN_R2, PPT_BTN_A, PPT_BTN_B, PPT_BTN_X, PPT_BTN_Y, PPT_BTN_MENU, PPT_BTN_R3, PPT_BTN_L3};
static const int rawMap[C_COUNT] = {310, 311, 308, 314, 309, 315, 304, 305, 307, 306, 354, BTN_THUMBR, BTN_THUMBL};
static int fds[DEVICES];
static bool held[C_COUNT], dir[4], dirRepeat[4];
static uint32_t nextAt[4];
static uint32_t dirDownAt[4]; /* absolute key-down timestamp for SAMPLER RANGE acceleration */
static int lastPitch=24; /* C-3, original PT2 uses C-1..B-3 */
void ppt_set_import_note(int pitch){
  if(pitch>=0 && pitch<36){lastPitch=pitch;editor.currPlayNote=(uint8_t)pitch;}
}
static uint8_t lastCmd=0, lastParam=0;
/* L1/R1 act on release, so a chord never switches patterns unintentionally. */
static bool shoulderUsed[2];
/* SELECT alone switches between PLAY and native RECORD-style live editing.
   The transport keeps running, and SELECT+START must NOT toggle this mode. */
/* Audio callback and main SDL thread share this flag. Keep it atomic so
   the replayer can skip ONLY the UI position sync while remaining realtime. */
static atomic_bool liveEditEnabled;
/* PT2 POS editor is a separate view. Keep its selection detached from
   the replayer in SONG playback too, even when LIVE EDIT is disabled. */
static atomic_bool posBrowseActive;
bool ppt_position_browse_active(void){
  return atomic_load_explicit(&posBrowseActive,memory_order_acquire);
}

static bool selectChordUsed;
bool ppt_live_edit_active(void){
  return atomic_load_explicit(&liveEditEnabled,memory_order_acquire);
}
/* PLAYHEAD snapshot from PT2's real audio tick. Packing PATTERN+ROW in one
   atomic prevents mismatched row/pattern pairs on the SDL display thread.
   IMPORTANT: the cursor (currRow / currPattern) is NOT updated here. */
static atomic_int playheadSnapshot = ATOMIC_VAR_INIT(-1);
void ppt_set_playhead(int pattern, int row){
  if(pattern>=0 && pattern<MAX_PATTERNS && row>=0 && row<64)
    atomic_store_explicit(&playheadSnapshot,(pattern<<8)|row,memory_order_release);
  else
    atomic_store_explicit(&playheadSnapshot,-1,memory_order_release);
}
int ppt_ghost_row(int displayPattern){
  if(!ppt_live_edit_active() || !editor.songPlaying)return -1;
  const int snapshot=atomic_load_explicit(&playheadSnapshot,memory_order_acquire);
  if(snapshot<0 || (snapshot>>8)!=displayPattern)return -1;
  return snapshot&255;
}
static FILE *inputLog;
static void logLiveEdit(const char *event){
  if (inputLog){fprintf(inputLog,"PPT 0.3.31 LIVE_EDIT %s playing=%d currMode=%d\n",event,(int)editor.songPlaying,(int)editor.currMode);fflush(inputLog);}
}
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
static int mouseDpadFracX, mouseDpadFracY;
static bool rightStickArmed=true;
static int logDirections, logAxes, logButtons, logUnmapped;
static int analogByDevice[DEVICES][TRACKED_AXES];
/* The MENU/M button may use a non-standard evdev code on muOS.
   If the firmware consumes the key before evdev we cannot receive it. */
static bool menuRawCode(unsigned code){
  return code==312 || code==354 || code==KEY_MENU;
}
static int buttonFromRaw(unsigned code){
  for(int j=0;j<C_COUNT;j++)if(code==(unsigned)rawMap[j])return j;
  if(code==313)return PPT_BTN_L3; /* muOS alternate stick-click code */
  if(code==BTN_MODE)return PPT_BTN_R3;
  if(menuRawCode(code))return PPT_BTN_MENU;
  return -1;
}
static void diagnostic(const char *kind,int code,int value){
  if(!inputLog)return;
  int *counter=&logButtons;
  if(kind[0]=='D')counter=&logDirections;
  if(kind[0]=='A')counter=&logAxes;
  if((*counter)++>=24)return;
  fprintf(inputLog,"PPT 0.3.31 %s code=%d value=%d\n",kind,code,value);
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
/* v0.3.1 precision mouse: low-amplitude input moves in sub-pixel steps,
   high deflection accelerates smoothly; no rounding on each frame. */
static int precisionMouseStep(int normalized){
  int mag=abs(normalized);
  if(mag<=MOUSE_DEADZONE)return 0;
  const int full=32767-MOUSE_DEADZONE;
  const int distance=clamp(mag-MOUSE_DEADZONE,0,full);
  const int64_t linear=(int64_t)distance*MOUSE_FINE_LINEAR/full;
  const int64_t curve=(int64_t)distance*distance*MOUSE_FINE_CURVE/((int64_t)full*full);
  int result=(int)(linear+curve);
  return normalized<0?-result:result;
}
/* Apply the same 70% speed factor to analog and D-pad mouse movement.
   sourceDen=1024 for analog fixed-point steps, 1 for D-pad pixel steps.
   The accumulator preserves subpixel motion and exact average speed. */
static int scaledMousePixels(int step, int *fraction, int sourceDen){
  const int denom=sourceDen*MOUSE_SPEED_DEN;
  const int total=step*MOUSE_SPEED_NUM+*fraction;
  const int pixels=total/denom;
  *fraction=total-pixels*denom;
  return pixels;
}
static void updateAnalogMouse(void){
  /* Left stick confirmed on this unit: ABS_Z=2 horizontal, ABS_RX=3 vertical.
     Pixel fractions accumulate until a full pixel can be moved; slow nudges
     therefore work even on a 320x255 virtual screen. */
  const int x=precisionMouseStep(axisAmount(ABS_Z));
  const int y=precisionMouseStep(axisAmount(ABS_RX));
  if(!x)mouseFracX=0;
  if(!y)mouseFracY=0;
  const int dx=scaledMousePixels(x,&mouseFracX,MOUSE_ANALOG_DEN);
  const int dy=scaledMousePixels(y,&mouseFracY,MOUSE_ANALOG_DEN);
  moveVirtualMouse(dx,dy);
}


/* PT2 native carousel ONLY in MOD NAME and SAMPLE NAME.
   All other string fields (POSITION, DISK OP, MIX) and number fields stay
   on the original v0.3.10 controller path, without any carousel override.
   The original PT2 text editor owns the buffer and its commit semantics. */
static const char nameChars[]=" abcdefghijklmnopqrstuvwxyz0123456789-_";
/* Right analog: physical ABS_RY(4) horizontal, ABS_RZ(5) vertical.
   Change view once at >91% travel. Release near center to re-arm. */
enum {PPT_VIEW_POSITION, PPT_VIEW_PATTERN, PPT_VIEW_DISK, PPT_VIEW_SAMPLER};
static bool fullPattern;
static int fullPatternReturnView=PPT_VIEW_PATTERN;
bool ppt_full_pattern_active(void){
  return fullPattern && song && !ui.askBoxShown && !ui.editTextFlag &&
    !ui.diskOpScreenShown && !ui.samplerScreenShown && !ui.posEdScreenShown &&
    !ui.editOpScreenShown && !ui.aboutScreenShown;
}
static void rightStickOpenView(int target)
{
  if(!song || ui.editTextFlag || ui.askBoxShown)return;
  fullPattern=false;
  if(ui.samplerScreenShown && target!=PPT_VIEW_SAMPLER)exitFromSam();
  ui.posEdScreenShown=false;
  ui.diskOpScreenShown=false;
  ui.editOpScreenShown=false;
  ui.aboutScreenShown=false;
  if(target==PPT_VIEW_POSITION)ui.posEdScreenShown=true;
  else if(target==PPT_VIEW_DISK)ui.diskOpScreenShown=true;
  displayMainScreen();
  if(target==PPT_VIEW_SAMPLER && !ui.samplerScreenShown)samplerScreen();
  if(inputLog){
    fprintf(inputLog,"PPT 0.3.31 VIEW %s playing=%d\n",
      target==PPT_VIEW_POSITION?"POSITION":target==PPT_VIEW_PATTERN?"PATTERN":
      target==PPT_VIEW_DISK?"DISKOP":"SAMPLER",(int)editor.songPlaying);
    fflush(inputLog);
  }
}
static void toggleFullPattern(void){
  if(!song || ui.askBoxShown || ui.editTextFlag || ui.changingSmpResample)return;
  if(fullPattern){rightStickOpenView(fullPatternReturnView);return;}
  fullPatternReturnView=ui.samplerScreenShown?PPT_VIEW_SAMPLER:
    ui.diskOpScreenShown?PPT_VIEW_DISK:ui.posEdScreenShown?PPT_VIEW_POSITION:PPT_VIEW_PATTERN;
  rightStickOpenView(PPT_VIEW_PATTERN);
  fullPattern=true;
}
static void fullPatternClick(void){
  /* Same 320x255 coordinates used by the independent fullscreen renderer. */
  if(mouse.x<32 || mouse.x>=320 || mouse.y<12 || mouse.y>=243)return;
  const int first=clamp((int)song->currRow-15,0,32);
  const int current=(int)song->currRow-first;
  int index=0,y=12;
  for(;index<31;index++){
    const int height=index==current?14:7;
    if(mouse.y<y+height)break;
    y+=height;
  }
  const int row=first+index;
  const int channel=clamp((mouse.x-32)/72,0,3);
  const int digit=clamp(((mouse.x-32)%72)/8,0,7);
  song->currRow=row;
  cursor.channel=channel;
  cursor.pos=channel*6+(digit<3?0:digit-2);
  updateCursorPos();ui.updatePatternData=true;
}
static void updateRightStickViews(void)
{
  if(!axis[ABS_RY].seen || !axis[ABS_RZ].seen)return;
  const int x=axisAmount(ABS_RY), y=axisAmount(ABS_RZ);
  const int ax=x<0?-x:x, ay=y<0?-y:y;
  if(ax<=RIGHT_STICK_REARM && ay<=RIGHT_STICK_REARM){rightStickArmed=true;return;}
  if(!rightStickArmed || !song || ui.askBoxShown || ui.editTextFlag)return;
  int target=-1;
  if(ax>=RIGHT_STICK_TRIGGER && ay<=RIGHT_STICK_CROSS_MAX)
    target=x<0?PPT_VIEW_POSITION:PPT_VIEW_PATTERN;
  else if(ay>=RIGHT_STICK_TRIGGER && ax<=RIGHT_STICK_CROSS_MAX)
    target=y<0?PPT_VIEW_DISK:PPT_VIEW_SAMPLER;
  if(target>=0){rightStickArmed=false;rightStickOpenView(target);}
}
static bool mouseDownForwarded[C_COUNT];
typedef struct { char *ptr; size_t maxChars; bool sample; } nameEdit_t;
static bool currentNameEdit(nameEdit_t *out){
  if(!song || !ui.editTextFlag || textEdit.type!=TEXT_EDIT_STRING || editor.mixFlag)
    return false;
  char *name=NULL;
  size_t cap=0;
  bool isSample=false;
  if(textEdit.object==PTB_SONGNAME){
    name=song->header.name;cap=20;
  }else if(textEdit.object==PTB_SAMPLENAME &&
           editor.currSample>=0 && editor.currSample<MOD_SAMPLES){
    name=song->samples[editor.currSample].text;cap=22;isSample=true;
  }else return false;
  if(textEdit.textStartPtr!=name || !textEdit.textPtr || !textEdit.textEndPtr ||
     textEdit.textPtr<name || textEdit.textPtr>=name+cap ||
     textEdit.textEndPtr<name || textEdit.textEndPtr>=name+cap)
    return false;
  if(out){out->ptr=name;out->maxChars=cap;out->sample=isSample;}
  return true;
}
static void nameRedraw(const nameEdit_t *n){
  if(n->sample)ui.updateCurrSampleName=true;
  else ui.updateSongName=true;
}
/* Overwrite the selected character. Always keep the native terminator. */
static void nameCycle(int step){
  nameEdit_t n;
  if(!currentNameEdit(&n))return;
  char *p=textEdit.textPtr;
  int idx=0;
  const char old=*p;
  const int count=(int)(sizeof(nameChars)-1);
  for(int i=0;i<count;i++){
    if(nameChars[i]==old ||
      ((old>='A' && old<='Z') && nameChars[i]==old-'A'+'a')){
      idx=i;break;
    }
  }
  idx=(idx+step+count)%count;
  *p=nameChars[idx];
  if(old=='\0' && p<n.ptr+n.maxChars-1)p[1]='\0';
  n.ptr[n.maxChars]='\0';
  nameRedraw(&n);
}
/* B deletes the previous character, shifting the suffix left.
   Cursor beyond current string end is first brought back to the end. */
static void nameBackspace(void){
  nameEdit_t n;
  if(!currentNameEdit(&n))return;
  const size_t len=strnlen(n.ptr,n.maxChars);
  while(textEdit.textPtr>n.ptr+len && textEdit.textPtr>n.ptr)
    editTextPrevChar();
  if(textEdit.textPtr<=n.ptr)return;
  const size_t offset=(size_t)(textEdit.textPtr-n.ptr)-1;
  if(offset<len)
    memmove(n.ptr+offset,n.ptr+offset+1,len-offset); /* including NUL */
  editTextPrevChar();
  n.ptr[n.maxChars]='\0';
  nameRedraw(&n);
}
static void nameAdvance(void){
  nameEdit_t n;
  if(!currentNameEdit(&n))return;
  if(*textEdit.textPtr=='\0'){
    *textEdit.textPtr=' ';
    if(textEdit.textPtr<n.ptr+n.maxChars-1)textEdit.textPtr[1]='\0';
    n.ptr[n.maxChars]='\0';
    nameRedraw(&n);
  }
  editTextNextChar();
}
static void nameFinish(void){
  nameEdit_t n;
  if(!currentNameEdit(&n))return;
  leaveTextEditMode(EDIT_TEXT_UPDATE); /* native PT2 commit */
  nameRedraw(&n);
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
/* A recorded pattern can be read on the audio thread. Lock while updating
   note/sample/effect cells to avoid tearing during live playback. */
static bool beginPatternWrite(void){
  bool doLock=editor.songPlaying && !audio.locked;
  if(doLock)lockAudio();
  return doLock;
}
static void endPatternWrite(bool locked){if(locked)unlockAudio();}
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
/* In STOP/ordinary PLAY, keep upstream PT2 ALT+left/right behavior.
   In LIVE EDIT, NEVER use incPatt/decPatt: those also change the audio
   replayer's internal modPattern. Instead switch only the editor's pattern. */
static void changePattern(int step){
  if(!song || ui.askBoxShown || ui.editTextFlag ||
     ui.diskOpScreenShown || ui.samplerScreenShown)return;
  if(ppt_live_edit_active() && editor.songPlaying){
    const bool locked=beginPatternWrite();
    song->currPattern=(uint16_t)wrap((int)song->currPattern+step,MAX_PATTERNS);
    ui.updateCurrPattText=true;
    ui.updatePatternData=true;
    endPatternWrite(locked);
  }else if(step<0)decPatt();
  else incPatt();
}
/* Use PT2's own sample change functions, which also update the sample info. */
static void changeActiveSample(int step){
  if(!song || ui.editTextFlag)return;
  if(ui.askBoxShown && !ppt_browser_import_dialog())return;
  if(ui.diskOpScreenShown && !ppt_browser_sample_mode())return;
  if(step<0) sampleDownButton();
  else sampleUpButton();
}
static void previewNote(const note_t *n){
  if(!n || !n->period)return;
  const int sample=(n->sample>=1 && n->sample<=31)?n->sample-1:clamp(editor.currSample,0,30);
  pptPreviewInsertedNote((int8_t)pitchOf(n->period),(int8_t)sample);
}
/* Effect name follows current FX/parameter field. Poll position/value once
   after input events, also covering changes by an attached USB keyboard. */
static int lastEffectCursor=-1, lastEffectPattern=-1, lastEffectRow=-1;
static int lastEffectCommand=-1, lastEffectSub=-1;
static void updateEffectStatus(void){
  if(!song || ui.askBoxShown || ui.diskOpScreenShown || ui.samplerScreenShown ||
     ui.editTextFlag || ui.posEdScreenShown || cursor.pos%6<CURSOR_CMD){
    lastEffectCursor=-1;return;
  }
  const note_t *n=currentNote();
  if(!n)return;
  const int sub=n->command==0xE?(n->param>>4):-1;
  if(lastEffectCursor==cursor.pos && lastEffectPattern==(int)song->currPattern &&
     lastEffectRow==(int)song->currRow && lastEffectCommand==(int)n->command &&
     lastEffectSub==sub)return;
  lastEffectCursor=cursor.pos;lastEffectPattern=(int)song->currPattern;
  lastEffectRow=(int)song->currRow;lastEffectCommand=n->command;lastEffectSub=sub;
  displayMsg(pptEffectName(n->command,n->param));
}
/* PT2's *native* EDITSKIP is editor.editMoveAdd (0..9), normally
   changed from a keyboard by CTRL+0..9. On the handheld, physical
   SELECT acts as SHIFT:
      SELECT+LEFT/RIGHT adjusts EDITSKIP -1/+1 and reports STATUS;
      SELECT+UP/DOWN navigates by EDITSKIP rows.
   EDITSKIP=0 disables automatic note-entry advance in upstream PT2.
   For explicit cursor travel a minimum manual step of 1 is used, so
   SHIFT+UP/DOWN never gets stuck at EDITSKIP=0. */
static void adjustEditSkip(int change){
  if(!song)return;
  const int next=clamp((int)editor.editMoveAdd+change,0,16);
  editor.editMoveAdd=(uint8_t)next;
  ui.updateTrackerFlags=true;
  char msg[32];
  snprintf(msg,sizeof(msg),"EDITSKIP = %d",next);
  displayMsg(msg);
  /* Prevent the FX tooltip poll from immediately hiding the new status
     when the cursor is sitting on an effect command/parameter. */
  const note_t *n=currentNote();
  if(n){
    lastEffectCursor=cursor.pos;
    lastEffectPattern=(int)song->currPattern;
    lastEffectRow=(int)song->currRow;
    lastEffectCommand=(int)n->command;
    lastEffectSub=n->command==0xE?(n->param>>4):-1;
  }
}
static void navigateEditSkip(int direction){
  if(!song)return;
  const int step=editor.editMoveAdd>0 ? editor.editMoveAdd : 1;
  song->currRow=wrap((int)song->currRow+(direction<0?-step:step),64);
  ui.updatePatternData=true;
  updateCursorPos();
}
static void edit(int d){
  if(!song || ui.diskOpScreenShown || ui.samplerScreenShown || ui.editTextFlag || ui.askBoxShown)return;
  if(editor.songPlaying && !liveEditEnabled)return;
  note_t *n=currentNote();if(!n)return;
  /* LSDJ convention: UP means HIGHER, DOWN means LOWER. */
  const int delta=(d==2)?-1:(d==3)?1:(d==0)?12:-12;
  const bool locked=beginPatternWrite();
  bool audition=false, sampleUIChanged=false;
  switch(cursor.pos%6){
    case CURSOR_NOTE: {
      int next=clamp(pitchOf(n->period)+delta,0,35);
      lastPitch=next;n->period=periods[next];
      /* Pitch remembered, sample number never cached by the adapter. */
      audition=true;
    }break;
    case CURSOR_SAMPLE1:
    case CURSOR_SAMPLE2: {
      int step=(d<2)?16:1;
      int sign=(d==0 || d==3)?1:-1;
      const int selected=clamp(editor.currSample+1,1,31);
      const int num=clamp((n->sample?n->sample:selected)+sign*step,1,31);
      n->sample=(uint8_t)num;
      editor.currSample=(int8_t)(num-1);
      sampleUIChanged=true;
      audition=n->period!=0;
    }break;
    case CURSOR_CMD:
      lastCmd=(uint8_t)wrap((int)n->command+((d<2)?(d==0?4:-4):(d==2?-1:1)),16);
      n->command=lastCmd;
      n->param=pptEffectClamp(n->command,n->param);
      lastParam=n->param;
      break;
    case CURSOR_PARAM1:
    case CURSOR_PARAM2:
      /* Unlike NOTE, FX digits obey effect-specific ranges: D is BCD 00..63,
         C is HEX 00..40; Ex has limits determined by its high nibble. */
      lastParam=pptEffectStep(n->command,n->param,cursor.pos%6,d);
      n->param=lastParam;break;
  }
  endPatternWrite(locked);
  if(sampleUIChanged)updateCurrSample();
  changed();
  if(audition)previewNote(n);
}
extern void pptRecordInsertedNote(int8_t pitch,int8_t sampleIndex);
static bool aAdvancePending;
static int aAdvanceRow,aAdvancePattern;
static module_t *aAdvanceOwner;
static void finishAEntry(void){
  if(aAdvancePending && song==aAdvanceOwner && song &&
     !ui.askBoxShown && !ui.editTextFlag && !ui.diskOpScreenShown && !ui.samplerScreenShown &&
     (int)song->currPattern==aAdvancePattern && (int)song->currRow==aAdvanceRow &&
     editor.currMode!=MODE_RECORD && (!editor.songPlaying || ppt_live_edit_active())){
    song->currRow=wrap(aAdvanceRow+clamp(editor.editMoveAdd,0,16),64);
    ui.updatePatternData=true;updateCursorPos();
  }
  aAdvancePending=false;
}
static void insert(void){
  if(!song || ui.diskOpScreenShown || ui.samplerScreenShown || ui.editTextFlag || ui.askBoxShown)return;
  if(editor.songPlaying && editor.currMode==MODE_RECORD){
    aAdvancePending=false;
    pptRecordInsertedNote((int8_t)lastPitch,editor.currSample);
    return;
  }
  if(editor.songPlaying && !liveEditEnabled)return;
  note_t *n=currentNote();if(!n)return;
  const bool locked=beginPatternWrite();
  bool audition=false;
  const uint8_t selectedSample=(uint8_t)(clamp(editor.currSample,0,30)+1);
  switch(cursor.pos%6){
    case CURSOR_NOTE:
      /* A always replaces NOTE and SAMPLE with current pitch and currently
         selected sample, even if the destination already has a sample. */
      n->period=periods[lastPitch];
      n->sample=selectedSample;
      audition=true;
      if(held[PPT_BTN_A]){
        aAdvancePending=true;aAdvanceRow=song->currRow;
        aAdvancePattern=song->currPattern;aAdvanceOwner=song;
      }
      break;
    case CURSOR_SAMPLE1:
    case CURSOR_SAMPLE2:
      /* Editing SAMPLE by A also overwrites an existing sample number.
         Preserve the note pitch and audition using the NEW sample. */
      n->sample=selectedSample;
      audition=n->period!=0;
      break;
    case CURSOR_CMD:
      n->command=lastCmd;
      n->param=pptEffectClamp(n->command,n->param);
      lastParam=n->param;break;
    case CURSOR_PARAM1:
    case CURSOR_PARAM2:
      n->param=pptEffectClamp(n->command,lastParam);
      lastParam=n->param;break;
  }
  endPatternWrite(locked);
  changed();
  /* Never call upstream Paula preview while holding our own audio lock:
     the native preview acquires lockAudio() on its own. */
  if(audition)previewNote(n);
}

static void erase(void){
  if(editor.songPlaying && !liveEditEnabled)return;
  note_t *n=currentNote();if(!n)return;
  const bool locked=beginPatternWrite();
  switch(cursor.pos%6){
    case CURSOR_NOTE:n->period=0;n->sample=0;break;
    case CURSOR_SAMPLE1:
    case CURSOR_SAMPLE2:n->sample=0;break;
    case CURSOR_CMD:n->command=0;break;
    case CURSOR_PARAM1:
    case CURSOR_PARAM2:n->param=0;break;
  }
  endPatternWrite(locked);
  changed();
}
/* PPT v0.3.7: one selection-aware clipboard with three modes:
   rectangular FIELD block, entire CHANNEL, entire PATTERN.
   Coordinates use row 0..63 and linear field column 0..23 (4 channels x
   NOTE, SMP-high, SMP-low, FX, PARAM-high, PARAM-low). This is not the
   upstream PT2 keyboard clipboard; it is an independent controller clipboard
   so field-accurate selection and live-edit pasting work consistently. */
enum {PPT_CLIP_NONE, PPT_CLIP_BLOCK, PPT_CLIP_CHANNEL, PPT_CLIP_PATTERN};
static struct {
  int kind, rowLo, rowHi, colLo, colHi, sourceChannel;
  note_t cells[64][PAULA_VOICES];
} clip;
static bool selecting;
static int markRow, markCol, markPattern;

/* PPT 0.3.31 POSITION LOOP. Packed into one atomic word because the audio
   callback cannot safely lock or read half-updated UI selection state.
   bits 0-7=first, 8-15=last, 16=range valid, 17=enabled. */
enum { POS_LOOP_VALID=1<<16, POS_LOOP_ON=1<<17 };
static atomic_int positionLoop = ATOMIC_VAR_INIT(0);
static bool posMarking;
static int posMarkStart, posMarkEnd;
static module_t *posLoopOwner;
static void posLoopRefresh(void){ui.updatePosEd=true;}
static bool posEditorActive(void){
  return song && ui.posEdScreenShown && !ui.editTextFlag &&
    !ui.askBoxShown && !ui.diskOpScreenShown;
}
static int posLoopFirst(int packed){return packed&255;}
static int posLoopLast(int packed){return (packed>>8)&255;}
/* Called on Paula's audio thread. Only redirects when advancing out of the
   inclusive range; Bxx/Dxx jumps inside the loop retain their normal effect.
   Caller resets ROW to 00 when the returned position differs. */
int ppt_loop_redirect(int proposed, int length){
  const int info=atomic_load_explicit(&positionLoop,memory_order_acquire);
  if((info&(POS_LOOP_VALID|POS_LOOP_ON))!=(POS_LOOP_VALID|POS_LOOP_ON) ||
     length<=0)return proposed;
  const int lo=posLoopFirst(info), hi=posLoopLast(info);
  if(lo<0 || hi>=length || lo>hi)return proposed;
  return proposed<lo || proposed>hi ? lo : proposed;
}
/* 1 for a temporary mark, 2 for a confirmed, active loop.
   SELECT+Y clears both: no inactive gray region remains. */
int ppt_loop_entry_style(int pos){
  if(posMarking && song && ui.posEdScreenShown){
    const int a=posMarkStart, b=posMarkEnd;
    return pos>=(a<b?a:b) && pos<=(a>b?a:b) ? 1 : 0;
  }
  const int info=atomic_load_explicit(&positionLoop,memory_order_acquire);
  return (info&POS_LOOP_VALID) && pos>=posLoopFirst(info) &&
         pos<=posLoopLast(info) ? ((info&POS_LOOP_ON)?2:1) : 0;
}
static void clearPosLoop(void){
  atomic_store_explicit(&positionLoop,0,memory_order_release);
  posMarking=false;posLoopRefresh();
}
static void beginPosMark(void){
  if(!posEditorActive())return;
  posMarking=true;posMarkStart=posMarkEnd=clamp((int)song->currPos,0,127);
  posLoopRefresh();
  if(inputLog){fprintf(inputLog,"PPT 0.3.31 POS_LOOP MARK %d\n",posMarkStart);fflush(inputLog);}
}
static void confirmPosMark(void){
  if(!posEditorActive() || !posMarking)return;
  /* Selection endpoint is independent of the continuously advancing
     PLAYHEAD. Without this, confirming a loop during SONG playback could
     accidentally select the current playback position instead of the user's
     selected endpoint. */
  const int here=posMarkEnd;
  const int lo=here<posMarkStart?here:posMarkStart;
  const int hi=here>posMarkStart?here:posMarkStart;
  atomic_store_explicit(&positionLoop,lo|(hi<<8)|POS_LOOP_VALID|POS_LOOP_ON,memory_order_release);
  posMarking=false;posLoopRefresh();
  if(inputLog){fprintf(inputLog,"PPT 0.3.31 POS_LOOP RANGE %d..%d ON\n",lo,hi);fflush(inputLog);}
}
/* SELECT+Y is a full CLEAR, not an enable/disable toggle. The former
   toggle left a gray stored range behind and prevented intuitive remarking. */
static void userClearPosLoop(void){
  if(!posEditorActive())return;
  clearPosLoop();
  if(inputLog){fprintf(inputLog,"PPT 0.3.31 POS_LOOP CLEARED\n");fflush(inputLog);}
}
/* Move a selected loop endpoint without moving the engine's playback
   position. While STOPped, browse the normal PT2 list so the endpoint is
   visible. While PLAYing, leave the native POSITION view following the song. */
static void browsePos(int direction);
static void movePosMark(int direction){
  if(!posEditorActive() || !posMarking)return;
  const int len=clamp((int)song->header.songLength,1,128);
  const int next=clamp(posMarkEnd+direction,0,len-1);
  if(next==posMarkEnd)return;
  posMarkEnd=next;
  if(!editor.songPlaying)browsePos(direction);
  posLoopRefresh();
}
/* Change the DISPLAYED song position without calling modSetPos(), which
   also changes modPos and could cause an audible jump during playback. */
static void browsePos(int direction){
  if(!posEditorActive())return;
  const int length=clamp((int)song->header.songLength,1,128);
  const int old=clamp((int)song->currPos,0,length-1);
  const int now=clamp(old+direction,0,length-1);
  if(now==old)return;
  song->currPos=(uint16_t)now;
  const int pat=clamp((int)song->header.patternTable[now],0,MAX_PATTERNS-1);
  song->currPattern=(uint16_t)pat;
  editor.currPatternDisp=&song->header.patternTable[now];
  editor.currPosEdPattDisp=&song->header.patternTable[now];
  ui.updateSongPos=true;
  ui.updateSongPattern=true;
  ui.updateCurrPattText=true;
  ui.updatePatternData=true;
  posLoopRefresh();
}

static bool selectionAllowed(void){
  return song && !ui.askBoxShown && !ui.editTextFlag &&
         !ui.diskOpScreenShown && !ui.samplerScreenShown &&
         (!editor.songPlaying || ppt_live_edit_active());
}
/* Rendering queries only the active selection, NOT the saved clipboard.
   Playback ghost stays independent and is rendered underneath any selected
   cells (selection gray takes precedence on overlap). */
bool ppt_selection_contains(int patt,int row,int channel,int field){
  if(!selecting || !song || patt!=markPattern || row<0 || row>=64 ||
     channel<0 || channel>=PAULA_VOICES || field<0 || field>=6)return false;
  const int col=channel*6+field;
  const int loRow=markRow<song->currRow?markRow:song->currRow;
  const int hiRow=markRow>song->currRow?markRow:song->currRow;
  const int loCol=markCol<cursor.pos?markCol:cursor.pos;
  const int hiCol=markCol>cursor.pos?markCol:cursor.pos;
  return row>=loRow && row<=hiRow && col>=loCol && col<=hiCol;
}
static void cancelSelection(void){
  if(selecting){selecting=false;ui.updatePatternData=true;}
}
static void beginSelection(void){
  if(!selectionAllowed())return;
  selecting=true;
  markRow=clamp((int)song->currRow,0,63);
  markCol=clamp(cursor.pos,0,23);
  markPattern=song->currPattern;
  ui.updatePatternData=true;
}
static void stashClip(int kind){
  if(!selectionAllowed())return;
  const int pat=song->currPattern;
  note_t *source=song->patterns[pat];
  if(!source)return;
  clip.kind=kind;
  clip.sourceChannel=clamp((int)cursor.channel,0,3);
  if(kind==PPT_CLIP_BLOCK){
    if(!selecting || markPattern!=pat)return;
    clip.rowLo=markRow<song->currRow?markRow:song->currRow;
    clip.rowHi=markRow>song->currRow?markRow:song->currRow;
    clip.colLo=markCol<cursor.pos?markCol:cursor.pos;
    clip.colHi=markCol>cursor.pos?markCol:cursor.pos;
  }else{
    clip.rowLo=0;clip.rowHi=63;
    clip.colLo=kind==PPT_CLIP_CHANNEL?clip.sourceChannel*6:0;
    clip.colHi=kind==PPT_CLIP_CHANNEL?clip.colLo+5:23;
  }
  const bool locked=beginPatternWrite();
  memcpy(clip.cells,source,sizeof(clip.cells));
  endPatternWrite(locked);
  cancelSelection();
  if(inputLog){fprintf(inputLog,"PPT 0.3.31 CLIP kind=%d rows=%d..%d columns=%d..%d\n",
    clip.kind,clip.rowLo,clip.rowHi,clip.colLo,clip.colHi);fflush(inputLog);}
}
static void copyFieldValue(note_t *dst,const note_t *src,int srcField,int dstField){
  /* Field-to-field paste preserves the meaning of the SOURCE column.
     A NOTE pasted onto NOTE replaces only pitch; a single SAMPLE/PARAM
     nibble preserves the destination's other nibble. */
  if(srcField!=dstField)return;
  switch(dstField){
    case CURSOR_NOTE: dst->period=src->period;break;
    case CURSOR_SAMPLE1:dst->sample=(dst->sample&0x0F)|(src->sample&0xF0);break;
    case CURSOR_SAMPLE2:dst->sample=(dst->sample&0xF0)|(src->sample&0x0F);break;
    case CURSOR_CMD:dst->command=src->command;break;
    case CURSOR_PARAM1:dst->param=(dst->param&0x0F)|(src->param&0xF0);break;
    case CURSOR_PARAM2:dst->param=(dst->param&0xF0)|(src->param&0x0F);break;
  }
  if(dstField==CURSOR_CMD || dstField==CURSOR_PARAM1 || dstField==CURSOR_PARAM2)
    dst->param=pptEffectClamp(dst->command,dst->param);
}
/* Delete only the selected field rectangle; preserve the saved clipboard. */
static void eraseSelection(void){
  if(!selecting || !selectionAllowed() || (int)song->currPattern!=markPattern)return;
  note_t *dest=song->patterns[markPattern];
  if(!dest)return;
  const bool locked=beginPatternWrite();
  for(int row=0;row<64;row++)for(int col=0;col<24;col++){
    if(!ppt_selection_contains(markPattern,row,col/6,col%6))continue;
    note_t *n=&dest[row*PAULA_VOICES+col/6];
    switch(col%6){
      case CURSOR_NOTE:n->period=0;break;
      case CURSOR_SAMPLE1:n->sample&=0x0F;break;
      case CURSOR_SAMPLE2:n->sample&=0xF0;break;
      case CURSOR_CMD:n->command=0;break;
      case CURSOR_PARAM1:n->param&=0x0F;break;
      case CURSOR_PARAM2:n->param&=0xF0;break;
    }
  }
  endPatternWrite(locked);
  cancelSelection();changed();
}
static void pasteClip(void){
  if(!selectionAllowed() || clip.kind==PPT_CLIP_NONE)return;
  note_t *dest=song->patterns[song->currPattern];
  if(!dest)return;
  /* Whole PATTERN is unconditional 64x4 overwrite, just like ALT+F5.
     Whole CHANNEL starts at row 0 in CURRENT channel, like SHIFT+F5.
     Selected BLOCK uses current row/field as its top-left anchor. */
  const int startRow=clip.kind==PPT_CLIP_BLOCK?(int)song->currRow:0;
  const int startCol=clip.kind==PPT_CLIP_BLOCK?cursor.pos:
                     clip.kind==PPT_CLIP_CHANNEL?cursor.channel*6:0;
  const int rowCount=clip.rowHi-clip.rowLo+1;
  const int colCount=clip.colHi-clip.colLo+1;
  const bool locked=beginPatternWrite();
  for(int r=0;r<rowCount && startRow+r<64;r++){
    for(int c=0;c<colCount && startCol+c<24;c++){
      const int sourceCol=clip.colLo+c, targetCol=startCol+c;
      const int sourceRow=clip.rowLo+r;
      const note_t *src=&clip.cells[sourceRow][sourceCol/6];
      note_t *dst=&dest[(startRow+r)*PAULA_VOICES+targetCol/6];
      /* For rectangular field pasting, map note/sample/FX positions by
         RELATIVE field index (e.g. NOTE=>NOTE when copying a channel).
         For a one-column block pasted to another type of field, we don't
         silently reinterpret notes as effect parameters. */
      copyFieldValue(dst,src,sourceCol%6,targetCol%6);
    }
  }
  endPatternWrite(locked);
  changed();
  if(inputLog){fprintf(inputLog,"PPT 0.3.31 PASTE kind=%d to patt=%d row=%d col=%d\n",
   clip.kind,(int)song->currPattern,startRow,startCol);fflush(inputLog);}
}
/* Transport: START plays pattern from 00; SELECT+START plays the SAME
   pattern from the selected ROW; MENU+START plays SONG from selected POSITION.
   Every START chord stops playback if the replayer is currently running. */
/* LIVE EDIT: do not change PT2's currMode or audio clock. The patched
   replayer's updateUIPositions() skips copying audio row/pattern/order to
   the display while live edit is enabled. Audio continues on song->row.
   D-pad can move the separate editor view (song->currRow). */
static void toggleLiveEdit(void){
  if(editor.currMode==MODE_RECORD)return;
  if(!song || !editor.songPlaying || ui.askBoxShown || ui.editTextFlag ||
     ui.diskOpScreenShown || ui.samplerScreenShown)return;
  /* IMPORTANT: v0.3.0 changed editor.currMode from PLAY to RECORD in-place.
     PT2's RECORD mode has additional state dependencies, so this transition
     was unsafe and could terminate the process. Keep PT2's native transport
     mode untouched. Pad writes are enabled separately by liveEditEnabled. */
  /* Prevent an audio tick from advancing the UI midway through toggling. */
  const bool locked=beginPatternWrite();
  const bool next=!ppt_live_edit_active();
  atomic_store_explicit(&liveEditEnabled,next,memory_order_release);
  ui.updatePatternData=true;
  endPatternWrite(locked);
  logLiveEdit(next ? "ON_FROZEN_CURSOR" : "OFF_FOLLOW_PLAYHEAD");
}

enum { PPT_PLAY_PATTERN, PPT_PLAY_PATTERN_ROW, PPT_PLAY_SONG };
static void startPlayback(int mode){
  if(!song || ui.askBoxShown || ui.editTextFlag)return;
  if(editor.songPlaying){
    atomic_store(&liveEditEnabled,false);
    modStop();
    editor.currMode=MODE_IDLE;
    pointerSetMode(POINTER_MODE_IDLE,DO_CARRY);
    statusAllRight();
    return;
  }
  if(mode==PPT_PLAY_SONG){
    /* SONG begins at the currently displayed POSITION in the ORDER TABLE.
       PT2 resolves the matching pattern from song->header.patternTable[pos].
       Row 00, then subsequent order positions proceed as usual. */
    editor.playMode=PLAY_MODE_NORMAL;
    const int info=atomic_load_explicit(&positionLoop,memory_order_acquire);
    int startPos=(int)song->currPos;
    if((info&(POS_LOOP_ON|POS_LOOP_VALID))==(POS_LOOP_ON|POS_LOOP_VALID) &&
       posLoopLast(info)<song->header.songLength)
      startPos=posLoopFirst(info);
    modPlay(DONT_SET_PATTERN,(int16_t)startPos,0);
  }else{
    editor.playMode=PLAY_MODE_PATTERN;
    const int8_t row=mode==PPT_PLAY_PATTERN_ROW?(int8_t)song->currRow:0;
    modPlay(song->currPattern,DONT_SET_ORDER,row);
  }
  editor.currMode=MODE_PLAY;
  atomic_store(&liveEditEnabled,false);
  pointerSetMode(POINTER_MODE_PLAY,DO_CARRY);
  statusAllRight();
}
/* Dispatch the real Escape key to PT2's existing keyboard handler. In the
   main view this opens REALLY QUIT ?, from subviews it navigates back.
   Never force editor.programRunning=false and never bypass the confirmation. */
static bool exitChordSent, startPending, startWithShift, startWithMenu;
static void requestEscape(void){
  if(fullPattern){rightStickOpenView(fullPatternReturnView);exitChordSent=true;return;}
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
/* v0.3.16 SAMPLER pad controls. PT2 keeps ownership of audio,
   sample allocations, waveform drawing, and copy confirmation boxes. */
static void pptSamplerCopyToFreeSlot(void){
  if(!song || editor.sampleZero || editor.currSample<0 || editor.currSample>=MOD_SAMPLES)return;
  if(editor.markStartOfs<0 || editor.markEndOfs-editor.markStartOfs<2 ||
     editor.markEndOfs>song->samples[editor.currSample].length){
    displayErrorMsg("NO RANGE SELECTED");return;
  }
  /* Native samplerSamCopy SHIFT branch creates a new sample in the first
     zero-length slot, checks overflow, and prompts through askBox. Do NOT
     intercept its modal loop. The real keyboard modifier is restored. */
  const bool previousShift=keyb.shiftPressed;
  keyb.shiftPressed=true;
  samplerSamCopy();
  keyb.shiftPressed=previousShift;
}
/* PPT v0.3.31: one sample byte for a tap, progressively faster holds.
   Repeat delay is unchanged; the entire acceleration curve resets on key-up.
   Each step uses the same direction/boundary, no abrupt jumps on a tap. */
static int samplerRangeStep(int d){
  if(d!=2 && d!=3)return 1;
  if(!dir[d])return 1;
  const uint32_t elapsed=SDL_GetTicks()-dirDownAt[d];
  if(elapsed>=2300)return 100;
  if(elapsed>=1800)return 50;
  if(elapsed>=1350)return 10;
  if(elapsed>=900)return 5;
  if(elapsed>=450)return 2;
  return 1;
}
/* Move EITHER RANGE boundary independently; never move the other boundary.
   SELECT+LEFT/RIGHT controls the left boundary, MENU+LEFT/RIGHT the right.
   Keep at least two sample bytes between them to preserve a valid slice for
   PT2's sampler copy and its MOD-format LOOP ON operation. */
static void pptSamplerMoveRange(bool editStart,int direction,int step){
  if(!song || editor.sampleZero || editor.currSample<0 || editor.currSample>=MOD_SAMPLES)return;
  const int32_t length=song->samples[editor.currSample].length;
  int32_t start=editor.markStartOfs;
  int32_t end=editor.markEndOfs;
  if(start<0 || end-start<2 || end>length || length<2){
    displayErrorMsg("NO RANGE SELECTED");return;
  }
  const int32_t amount=step>0?step:1;
  const int32_t delta=direction<0?-amount:amount;
  if(editStart)start=clamp(start+delta,0,end-2);
  else end=clamp(end+delta,start+2,length);
  editor.markStartOfs=start;
  editor.markEndOfs=end;
  editor.samplePos=editStart?start:end;
  displaySample();
  updateSamplePos();
}
/* RESAMPLING NOTE carousel: the original PT2 sampler accepts 36 chromatic
   notes (C-1 to B-3). We leave samplerResample() untouched: moving in the
   carousel only selects its destination pitch, never edits sample data. */
static bool resampleCarouselActive;
static uint8_t resampleNoteBefore;
static void resampleCarouselBegin(void){
  if(!resampleCarouselActive){
    resampleCarouselActive=true;
    resampleNoteBefore=editor.resampleNote;
  }
}
static void resampleCarouselMove(int d){
  if(!ui.samplerScreenShown || !ui.changingSmpResample)return;
  resampleCarouselBegin();
  const int offset=d==0?1:d==1?-1:d==2?-12:12;
  editor.resampleNote=(uint8_t)clamp((int)editor.resampleNote+offset,0,35);
  ui.updateResampleNote=true;
}
static void resampleCarouselFinish(bool save){
  if(!ui.changingSmpResample)return;
  resampleCarouselBegin();
  if(!save)editor.resampleNote=resampleNoteBefore;
  ui.changingSmpResample=false;
  ui.updateResampleNote=true;
  resampleCarouselActive=false;
  setPrevStatusMessage();
  pointerSetPreviousMode();
}
static void toggleRecord(void){
  if(!song || ui.askBoxShown || ui.editTextFlag || ui.changingSmpResample)return;
  aAdvancePending=false;
  atomic_store(&liveEditEnabled,false);
  if(editor.songPlaying && editor.currMode==MODE_RECORD){
    modStop();editor.currMode=MODE_IDLE;
    pointerSetMode(POINTER_MODE_IDLE,DO_CARRY);statusAllRight();return;
  }
  if(ui.diskOpScreenShown || ui.samplerScreenShown || ui.posEdScreenShown)
    rightStickOpenView(PPT_VIEW_PATTERN);
  /* Identical initialization to native right SHIFT, not an in-place mode flip. */
  editor.playMode=PLAY_MODE_PATTERN;
  modPlay(song->currPattern,DONT_SET_ORDER,DONT_SET_ROW);
  editor.currMode=MODE_RECORD;
  pointerSetMode(POINTER_MODE_EDIT,DO_CARRY);statusAllRight();
}
static void buttonEvent(int b,bool down){
  if(b<0||b>=C_COUNT)return;
  if(held[b]==down)return;
  held[b]=down;
  if(!down)return;
  if(b!=PPT_BTN_A)aAdvancePending=false;
  /* MOD/SAMPLE NAME editing has priority over the normal X/Y mouse buttons.
     A mouse-down which originally entered the field is tracked and will
     still get a matching mouse-up when the key is released. */
  if(currentNameEdit(NULL)){
    if(b==PPT_BTN_X){nameAdvance();return;}
    if(b==PPT_BTN_Y){nameFinish();return;}
    if(b==PPT_BTN_B){nameBackspace();return;}
  }
  /* Native RESAMPLE NOTE selection is a modal note-entry prompt, not a
     text edit. X confirms, Y exits keeping the selected note, B cancels. */
  if(ui.samplerScreenShown && ui.changingSmpResample){
    if(b==PPT_BTN_X || b==PPT_BTN_Y){resampleCarouselFinish(true);return;}
    if(b==PPT_BTN_B){resampleCarouselFinish(false);return;}
    return;
  }
  if(b!=PPT_BTN_SELECT && held[PPT_BTN_SELECT])selectChordUsed=true;
  if(b==PPT_BTN_SELECT){selectChordUsed=false;return;}
  if(ppt_browser_sample_mode() && (b==PPT_BTN_L2 || b==PPT_BTN_R2)){
    if(!ui.editTextFlag)changeActiveSample(b==PPT_BTN_L2?-1:1);
    return; /* never fall through to legacy left/right mouse clicks */
  }
  if(ui.askBoxShown){
    if(b==PPT_BTN_A){modalKey(SDLK_RETURN);return;}
    if(b==PPT_BTN_B){modalKey(SDLK_ESCAPE);return;}
    /* Keep left/right clicks (including legacy L2/R2) in native modals. */
    if(b!=PPT_BTN_R2 && b!=PPT_BTN_L2 && b!=PPT_BTN_X && b!=PPT_BTN_Y)return;
  }
  if(ppt_browser_sample_mode() && !ui.editTextFlag && b==PPT_BTN_B){
    ppt_browser_preview();return;
  }
  if(b==PPT_BTN_L3){toggleRecord();return;}
  if(b==PPT_BTN_R3){toggleFullPattern();return;}
  if(ui.diskOpScreenShown && !ui.editTextFlag && b==PPT_BTN_A){
    ppt_disk_activate();return;
  }
  if(ppt_full_pattern_active() && b==PPT_BTN_X){fullPatternClick();return;}
  if(ppt_full_pattern_active() && b==PPT_BTN_Y && !selecting)return;
  /* Physical M/MENU (raw 312 confirmed on RG35XX Pro) + L1:
     native ESC / REALLY QUIT confirmation. Keep L1+START as fallback on
     firmware that swallows MENU at system level. */
  if((b==PPT_BTN_MENU && held[PPT_BTN_L1]) ||
     (b==PPT_BTN_L1 && held[PPT_BTN_MENU])){
    shoulderUsed[0]=true;
    requestEscape();
    return;
  }
  if(b==PPT_BTN_MENU){
    /* Also allow START -> MENU rather than requiring a specific key order. */
    if(held[PPT_BTN_START] && startPending)startWithMenu=true;
    return;
  }
  if(b==PPT_BTN_START){
    /* Start on RELEASE. This allows a near-simultaneous L1+START chord
       to take precedence even if START was physically pressed first. */
    if(held[PPT_BTN_L1]){startPending=false;shoulderUsed[0]=true;requestEscape();}
    else {startPending=true;startWithShift=held[PPT_BTN_SELECT];
          startWithMenu=held[PPT_BTN_MENU];}
    return;
  }
  /* Sampler buttons override pattern clipboard and A-note insertion.
     SELECT+A auditions the marked range; A plays the full waveform.
     B calls upstream copy-to-first-empty-slot (with confirmation). */
  if(ui.samplerScreenShown && !ui.editTextFlag){
    if(b==PPT_BTN_A){
      if(held[PPT_BTN_SELECT])samplerPlayRange();
      else samplerPlayWaveform();
      return;
    }
    if(b==PPT_BTN_B){pptSamplerCopyToFreeSlot();return;}
  }
  /* POSITION editor has its own rectangular SONG order selection.
     SELECT+B begins, and the next B confirms even if SELECT was released. */
  if(posEditorActive()){
    if(b==PPT_BTN_B){
      if(posMarking)confirmPosMark();
      else if(held[PPT_BTN_SELECT])beginPosMark();
      return;
    }
    if(b==PPT_BTN_Y && held[PPT_BTN_SELECT]){userClearPosLoop();return;}
    /* Prevent channel/pattern copy chords from escaping POSITION editor. */
    if((b==PPT_BTN_L1 || b==PPT_BTN_R1) && held[PPT_BTN_SELECT]){
      shoulderUsed[b==PPT_BTN_R1]=true;return;
    }
  }
  if(b==PPT_BTN_Y && selecting){eraseSelection();return;}
  /* Shortcut SELECT+B+L1/R1: native-style CHANNEL/PATTERN copy.
     This must win over pattern switching and over R1+D-pad mouse mode. */
  if((b==PPT_BTN_L1 || b==PPT_BTN_R1) &&
     held[PPT_BTN_SELECT] && held[PPT_BTN_B]){
    shoulderUsed[b==PPT_BTN_R1]=true;
    stashClip(b==PPT_BTN_L1?PPT_CLIP_CHANNEL:PPT_CLIP_PATTERN);
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
    if(held[PPT_BTN_SELECT])pasteClip();
    else if(held[PPT_BTN_B])erase(); /* legacy B+A clears selected field */
    else insert();
  }else if(b==PPT_BTN_B){
    if(held[PPT_BTN_SELECT]){
      if(selecting){stashClip(PPT_CLIP_BLOCK);}
      else if(held[PPT_BTN_R1]){shoulderUsed[1]=true;stashClip(PPT_CLIP_PATTERN);}
      else if(held[PPT_BTN_L1]){shoulderUsed[0]=true;stashClip(PPT_CLIP_CHANNEL);}
      else beginSelection(); /* first SELECT+B starts gray selection */
    }else if(selecting){stashClip(PPT_CLIP_BLOCK);} /* next B confirms */
    else if(held[PPT_BTN_A])erase();
    /* No separate single-field B clipboard. SELECT+B can mark one field. */
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
    mouseDownForwarded[b]=true;
  }
}
static void releaseButton(int b){
  if(b<0||b>=C_COUNT || !held[b])return;
  /* SELECT alone toggles live editing, but SELECT+A/B/START never does. */
  const bool doLive=(b==PPT_BTN_SELECT && !selectChordUsed &&
      editor.songPlaying && !selecting && !ui.askBoxShown && !ui.editTextFlag && !ui.changingSmpResample);
  /* Execute START when released so L1+START cannot accidentally PLAY. */
  const bool doStart=(b==PPT_BTN_START && startPending && !exitChordSent && !held[PPT_BTN_L1] && !ui.askBoxShown);
  /* MENU wins if both MENU and SELECT are held. */
  const int transportMode=(startWithMenu || held[PPT_BTN_MENU])?PPT_PLAY_SONG:
                          startWithShift?PPT_PLAY_PATTERN_ROW:PPT_PLAY_PATTERN;
  const bool doPattern=(b==PPT_BTN_L1 || b==PPT_BTN_R1) &&
      !shoulderUsed[b==PPT_BTN_R1] && !held[PPT_BTN_START] &&
      !held[PPT_BTN_MENU] && !held[PPT_BTN_SELECT] && !selecting &&
      !ui.posEdScreenShown && !ui.askBoxShown;
  held[b]=false;
  if(b==PPT_BTN_A)finishAEntry();
  if(b==PPT_BTN_SELECT){selectChordUsed=false;if(doLive)toggleLiveEdit();}
  if(b==PPT_BTN_START){startPending=false;startWithShift=false;
    startWithMenu=false;exitChordSent=false;}
  if(b==PPT_BTN_L1 || b==PPT_BTN_MENU)exitChordSent=false;
  if(doStart)startPlayback(transportMode);
  if(doPattern)changePattern(b==PPT_BTN_L1?-1:1);
  if(b==PPT_BTN_L1 || b==PPT_BTN_R1)shoulderUsed[b==PPT_BTN_R1]=false;
  if(video.window && mouseDownForwarded[b]){
    mouseDownForwarded[b]=false;
    SDL_Event evt;memset(&evt,0,sizeof(evt));
    evt.type=SDL_MOUSEBUTTONUP;
    evt.button.button=(b==PPT_BTN_X || b==PPT_BTN_R2)?SDL_BUTTON_LEFT:SDL_BUTTON_RIGHT;
    evt.button.state=SDL_RELEASED;SDL_PushEvent(&evt);
  }
}
static void directionAction(int d){
  if(held[PPT_BTN_A])aAdvancePending=false;
  if(ui.askBoxShown && ppt_browser_import_dialog()){
    const SDL_Keycode keys[4]={SDLK_UP,SDLK_DOWN,SDLK_LEFT,SDLK_RIGHT};
    modalKey(keys[d]);
    return;
  }
  if(ui.samplerScreenShown && ui.changingSmpResample){resampleCarouselMove(d);return;}
  if(currentNameEdit(NULL)){
    if(d==0)nameCycle(+1);       /* UP -> next char */
    else if(d==1)nameCycle(-1);  /* DOWN -> previous char */
    else if(d==2)editTextPrevChar(); /* LEFT -> previous position */
    else editTextNextChar();         /* RIGHT -> next position */
    return;
  }
  if(held[PPT_BTN_SELECT])selectChordUsed=true;
  if(ui.diskOpScreenShown && !ui.editTextFlag && !ui.askBoxShown && !held[PPT_BTN_R1]){
    if(d==0 || d==1)ppt_disk_move(d==0?-1:1);
    return;
  }
  if(ui.samplerScreenShown && !ui.editTextFlag){
    /* Both edges are adjustable in either direction, always by the same
       accelerated step (1/2/5/10/50/100 sample bytes):
       SELECT+LEFT/RIGHT = left/start edge; MENU+LEFT/RIGHT = right/end edge.
       MENU wins if both modifiers are held. These shortcuts apply only to
       SAMPLER, so SELECT+D-pad still controls EDITSKIP in PATTERN EDIT. */
    if((held[PPT_BTN_MENU] || held[PPT_BTN_SELECT]) && (d==2 || d==3))
      pptSamplerMoveRange(!held[PPT_BTN_MENU],d==2?-1:1,samplerRangeStep(d));
    return;
  }
  if(posEditorActive()){
    /* In POSITION EDIT the native PT2 playback view follows the song.
       An active loop selection has its own endpoint; D-pad cannot accidentally
       seek or stop audio while its gray range is being chosen. */
    if(d==0){if(posMarking)movePosMark(-1);else browsePos(-1);}
    else if(d==1){if(posMarking)movePosMark(+1);else browsePos(+1);}
    return;
  }
  if(selecting){
    /* Selection uses independent navigation; a held R1 is reserved for the
       SELECT+B+R1 whole-pattern chord, never virtual-mouse movement here. */
    if(song && markPattern==(int)song->currPattern){
      if(d==0)song->currRow=clamp((int)song->currRow-1,0,63);
      else if(d==1)song->currRow=clamp((int)song->currRow+1,0,63);
      else cursor.pos=clamp(cursor.pos+(d==2?-1:1),0,23);
      cursor.channel=cursor.pos/6;
      ui.updatePatternData=true;updateCursorPos();
    }
    return;
  }
  /* PATTERN EDIT only: SELECT+UP/DOWN jumps by native EDITSKIP;
     SELECT+LEFT/RIGHT adjusts the same native edit step and reports
     PT2's original status message. Separate sampler/POS/block handling
     above retains priority. R1+D-pad remains a mouse accelerator. */
  if(held[PPT_BTN_SELECT] && !held[PPT_BTN_R1] &&
     !ui.askBoxShown && !ui.diskOpScreenShown &&
     !ui.editTextFlag && !ui.editOpScreenShown && !ui.aboutScreenShown){
    if(d==0 || d==1)navigateEditSkip(d==0?-1:1);
    else adjustEditSkip(d==2?-1:1);
    return;
  }
  /* During askBox(), the D-pad controls the pointer, not the pattern.
     The askBox owns a separate event loop but still calls ppt_input_update(). */
  if(held[PPT_BTN_R1])shoulderUsed[1]=true;
  if(ui.askBoxShown || held[PPT_BTN_R1]){
    /* R1+D-pad is twice the old mouse speed; normal analog remains unchanged. */
    const int step=MOUSE_STEP*(held[PPT_BTN_R1]?R1_DPAD_MULTIPLIER:1);
    const int baseX=(d==2?-step:(d==3?step:0));
    const int baseY=(d==0?-step:(d==1?step:0));
    const int dx=scaledMousePixels(baseX,&mouseDpadFracX,1);
    const int dy=scaledMousePixels(baseY,&mouseDpadFracY,1);
    moveVirtualMouse(dx,dy);
  }else if(held[PPT_BTN_A])edit(d);
  else navigate(d);
}
static void directionEvent(int d,bool down){
  if(d<0||d>3)return;
  if(!down){dir[d]=false;dirRepeat[d]=false;return;}
  if(dir[d])return;
  dir[d]=true;dirRepeat[d]=false;
  dirDownAt[d]=SDL_GetTicks();
  nextAt[d]=dirDownAt[d]+INITIAL_DELAY;
  directionAction(d);
}
/* Hardware D-pad auto-repeat: 1-byte tap; step acceleration by hold duration. */
static void repeatDirections(uint32_t now){
  for(int d=0;d<4;d++){
    if(dir[d] && (int32_t)(now-nextAt[d])>=0){
      directionAction(d);
      nextAt[d]=now+REPEAT_DELAY;dirRepeat[d]=true;
    }
  }
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
  fullPattern=false;aAdvancePending=false;
  posLoopOwner=song;
  clearPosLoop();
  atomic_store(&posBrowseActive,false);
  memset(held,0,sizeof(held));memset(dir,0,sizeof(dir));
  memset(dirDownAt,0,sizeof(dirDownAt));memset(dirRepeat,0,sizeof(dirRepeat));
  memset(axis,0,sizeof(axis));
  rightStickArmed=true;
  const char *logPath=getenv("PPT_DIAG_LOG");
  if(logPath && logPath[0])inputLog=fopen(logPath,"a");
  if(inputLog){fprintf(inputLog,"PPT 0.3.31 input init\n");fflush(inputLog);}
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
        axis[a].value=info.value;axis[a].valid=true;axis[a].seen=true;
        if(inputLog){fprintf(inputLog,"axis %u min=%d max=%d center=%d\n",ids[a],info.minimum,info.maximum,info.value);fflush(inputLog);}
      }
    }
  }
  active=true;
}
void ppt_input_close(void){
  for(int i=0;i<DEVICES;i++){if(fds[i]>=0){close(fds[i]);fds[i]=-1;}}
  active=false;
  ppt_browser_close();
  if(inputLog){fclose(inputLog);inputLog=NULL;}
}
void ppt_input_update(void){
  if(!active)return;
  if(!ui.changingSmpResample)resampleCarouselActive=false;
  if(song!=posLoopOwner){
    posLoopOwner=song;
    clearPosLoop(); /* never carry a song range into another loaded MOD */
  }
  atomic_store_explicit(&posBrowseActive,(bool)(song && ui.posEdScreenShown && editor.songPlaying),memory_order_release);
  if(!ui.posEdScreenShown && posMarking){posMarking=false;posLoopRefresh();}
  if(!editor.songPlaying && ppt_live_edit_active()){
    atomic_store(&liveEditEnabled,false);logLiveEdit("AUTO_OFF");}
  struct input_event ev;
  for(int i=0;i<DEVICES;i++){
    if(fds[i]<0)continue;
    int limit=256;
    while(limit-- && read(fds[i],&ev,sizeof(ev))==(ssize_t)sizeof(ev)){
      if(ev.type==EV_KEY){
        bool down=ev.value!=0;
        const int b=buttonFromRaw(ev.code);
        if(b>=0){diagnostic("BUTTON",(int)ev.code,ev.value);if(down)buttonEvent(b,true);else releaseButton(b);continue;}
        int d=-1;
        if(ev.code==KEY_UP || ev.code==BTN_DPAD_UP)d=0;
        else if(ev.code==KEY_DOWN || ev.code==BTN_DPAD_DOWN)d=1;
        else if(ev.code==KEY_LEFT || ev.code==BTN_DPAD_LEFT)d=2;
        else if(ev.code==KEY_RIGHT || ev.code==BTN_DPAD_RIGHT)d=3;
        if(d>=0){diagnostic("DPAD-KEY",(int)ev.code,ev.value);directionEvent(d,down);}
        else if(inputLog && logUnmapped++<60){
          fprintf(inputLog,"PPT 0.3.31 OTHER-KEY event%d code=%u value=%d L1=%d\n",
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
              fprintf(inputLog,"PPT 0.3.31 ANALOG event%d code=%d value=%d\n",i,(int)ev.code,(int)ev.value);
              fflush(inputLog);
            }}
        }
      }
    }
  }
  ppt_browser_update();
  const uint32_t now=SDL_GetTicks();
  repeatDirections(now);
  updateRightStickViews();
  updateAnalogMouse();
  applyVirtualMouse();
  updateEffectStatus();
}
