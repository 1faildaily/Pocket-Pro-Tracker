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
void updateCursorPos(void){}
void setSpritePos(int s,int x,int y){(void)s;(void)x;(void)y;}
unsigned int SDL_GetTicks(void){return 0;}
int SDL_PushEvent(SDL_Event *event){(void)event;return 1;}
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
  /* Analog sticks with positive range 0..255 must normalize correctly. */
  axis[0]=(axis_t){.min=0,.max=255,.value=255,.valid=true,.seen=true};
  assert(axisAmount(0)>30000);
  axis[0].value=127;
  assert(abs(axisAmount(0))<1000);
  puts("PPT v0.2.1 input: Dpad HAT, mouse fallback, axes and core tests passed");
  return 0;
}
