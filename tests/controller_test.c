#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "pt2_header.h"
#include "pt2_structs.h"
module_t state,*song=&state;
editor_t editor;
ui_t ui;
mouse_t mouse;
cursor_t cursor;
video_t video;
static int playCalls, playPatt, playPos, playRow;
static int pointerModeCalls, statusCalls;
void modPlay(int16_t patt,int16_t pos,int8_t row){playCalls++;playPatt=patt;playPos=pos;playRow=row;}
void pointerSetMode(uint8_t mode,bool carry){assert(mode==POINTER_MODE_PLAY && carry==DO_CARRY);pointerModeCalls++;}
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
  /* A puts last note + remembered sample in empty location. */
  insert();
  assert(patt[0].period==214 && patt[0].sample==1);
  edit(3); /* A+RIGHT pitch C#3 */
  assert(patt[0].period==202);
  state.currRow=1;insert();
  assert(patt[4].period==202 && patt[4].sample==1);
  cursor.pos=1;edit(3); /* sample 2 */
  assert(patt[4].sample==2);
  state.currRow=2;cursor.pos=0;insert();
  assert(patt[8].period==202 && patt[8].sample==2);
  /* Pasting NOTE only cannot overwrite another sample or its FX. */
  state.currRow=1;copyField();
  patt[8].sample=7;patt[8].command=15;patt[8].param=0x7d;
  state.currRow=2;pasteField();
  assert(patt[8].period==patt[4].period && patt[8].sample==7 && patt[8].command==15 && patt[8].param==0x7d);
  /* Effect parameter editing supports complete byte 00..FF and wrap. */
  cursor.pos=5;edit(3);assert(patt[8].param==0x7e);
  cursor.pos=3;edit(2);assert(patt[8].command==14);
  /* Correct physical button map from user calibration. */
  assert(rawMap[PPT_BTN_SELECT]==310);
  assert(rawMap[PPT_BTN_START]==311);
  assert(rawMap[PPT_BTN_L1]==308);
  assert(rawMap[PPT_BTN_L2]==314);
  assert(rawMap[PPT_BTN_R1]==309);
  assert(rawMap[PPT_BTN_R2]==315);
  assert(rawMap[PPT_BTN_A]==304 && rawMap[PPT_BTN_B]==305 && rawMap[PPT_BTN_X]==307 && rawMap[PPT_BTN_Y]==306);
  /* Actual evdev on muOS can expose the D-pad as ABS_HAT0Y. */
  state.currRow=7;
  processHatAxis(ABS_HAT0Y,1);
  assert(state.currRow==8 && dir[1]);
  processHatAxis(ABS_HAT0Y,0);
  assert(!dir[1]);
  processHatAxis(ABS_HAT0X,-1);
  assert(dir[2]);
  processHatAxis(ABS_HAT0X,0);
  assert(!dir[2]);
  /* R1+D-pad moves the virtual mouse, without altering the row. */
  held[PPT_BTN_R1]=true;
  int mx=mousePosX, row=state.currRow;
  directionEvent(3,true);
  assert(mousePosX>mx && state.currRow==row);
  directionEvent(3,false);
  held[PPT_BTN_R1]=false;
  video.window=(SDL_Window*)1;
  applyVirtualMouse();
  assert(mouse.x==mousePosX && mouse.y==mousePosY);
  /* Right analog on RG35XX Pro: raw axis 4 drives pointer X,
     raw axis 5 drives pointer Y. Raw axis 3 is LEFT Y: ignored. */
  mousePosX=160;mousePosY=127;
  axis[ABS_RX]=(axis_t){.min=-4096,.max=4096,.value=4096,.valid=true,.seen=true};
  updateAnalogMouse();assert(mousePosX==160 && mousePosY==127);
  axis[ABS_RY]=(axis_t){.min=-4096,.max=4096,.value=4096,.valid=true,.seen=true};
  updateAnalogMouse();assert(mousePosX>160 && mousePosY==127);
  int xBefore=mousePosX;
  axis[ABS_RY].value=0;
  axis[ABS_RZ]=(axis_t){.min=-4096,.max=4096,.value=-4096,.valid=true,.seen=true};
  updateAnalogMouse();assert(mousePosX==xBefore && mousePosY<127);
  axis[ABS_RZ].value=0;
  /* START=PLAY PATTERN; physical SELECT+START=PLAY SONG. */
  state.currPos=5;state.currPattern=3;
  buttonEvent(PPT_BTN_START,true);
  assert(playCalls==1 && playPatt==3 && playPos==DONT_SET_ORDER && playRow==DONT_SET_ROW);
  assert(editor.playMode==PLAY_MODE_PATTERN && editor.currMode==MODE_PLAY);
  buttonEvent(PPT_BTN_START,false);
  buttonEvent(PPT_BTN_SELECT,true);
  buttonEvent(PPT_BTN_START,true);
  assert(playCalls==2 && playPatt==DONT_SET_PATTERN && playPos==5 && playRow==DONT_SET_ROW);
  assert(editor.playMode==PLAY_MODE_NORMAL && pointerModeCalls==2 && statusCalls==2);
  buttonEvent(PPT_BTN_START,false);
  buttonEvent(PPT_BTN_SELECT,false);
  /* Modal must not trigger playback on START. */
  ui.askBoxShown=true;
  buttonEvent(PPT_BTN_START,true);
  assert(playCalls==2);
  buttonEvent(PPT_BTN_START,false);
  ui.askBoxShown=false;
  /* Analog sticks with positive range 0..255 must normalize correctly. */
  axis[0]=(axis_t){.min=0,.max=255,.value=255,.valid=true,.seen=true};
  assert(axisAmount(0)>30000);
  axis[0].value=127;
  assert(abs(axisAmount(0))<1000);
  /* A blocking PT2 askBox must not insert notes or scroll the song. */
  ui.askBoxShown=true;
  int beforeRow=state.currRow;
  uint16_t beforeNote=patt[beforeRow*4].period;
  int beforeMouseX=mousePosX;
  directionEvent(3,true);
  assert(state.currRow==beforeRow && mousePosX>beforeMouseX);
  directionEvent(3,false);
  buttonEvent(PPT_BTN_A,true);
  assert(lastEvent.type==SDL_KEYDOWN && lastEvent.key.keysym.sym==SDLK_RETURN);
  buttonEvent(PPT_BTN_A,false);
  buttonEvent(PPT_BTN_B,true);
  assert(lastEvent.type==SDL_KEYDOWN && lastEvent.key.keysym.sym==SDLK_ESCAPE);
  buttonEvent(PPT_BTN_B,false);
  assert(patt[beforeRow*4].period==beforeNote);
  ui.askBoxShown=false;
  puts("PPT v0.2.3: START pattern, SELECT+START song, right analog axis fix, modal tests passed");
  return 0;
}
