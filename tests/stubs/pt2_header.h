#pragma once
#define MOD_ROWS 64
#define MAX_PATTERNS 100
#define PAULA_VOICES 4
#define CURSOR_NOTE 0
#define CURSOR_SAMPLE1 1
#define CURSOR_SAMPLE2 2
#define CURSOR_CMD 3
#define CURSOR_PARAM1 4
#define CURSOR_PARAM2 5
#define SDL_BUTTON_LEFT 1
#define SDL_BUTTON_RIGHT 3
#define SDL_PRESSED 1
#define SDL_RELEASED 0
#define SDL_MOUSEBUTTONDOWN 0x401
#define SDL_MOUSEBUTTONUP 0x402
#define SDL_KEYDOWN 0x300
#define SDLK_RETURN 13
#define SDLK_ESCAPE 27
typedef int SDL_Keycode;
typedef struct SDL_Window SDL_Window;
typedef struct {unsigned int type; struct {unsigned char button,state;} button; struct {unsigned char state; struct {SDL_Keycode sym;} keysym;} key;} SDL_Event;
unsigned int SDL_GetTicks(void);
int SDL_PushEvent(SDL_Event*);
unsigned int SDL_GetMouseState(int*,int*);
void SDL_WarpMouseInWindow(SDL_Window*,int,int);
