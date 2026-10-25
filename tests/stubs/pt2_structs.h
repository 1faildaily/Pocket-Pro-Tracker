#pragma once
#include "pt2_header.h"
#include <stdint.h>
#include <stdbool.h>
typedef struct {uint8_t param,sample,command;uint16_t period;} note_t;
typedef struct {unsigned int currPattern, currPos; int currRow; bool modified; note_t *patterns[MAX_PATTERNS];} module_t;
typedef struct {int8_t currSample;bool programRunning, songPlaying; int playMode,currMode;} editor_t;
typedef struct {bool updatePatternData, updateCurrSampleNum,editTextFlag,askBoxShown,diskOpScreenShown,samplerScreenShown;} ui_t;
typedef struct {int pos,channel;} cursor_t;
typedef struct {SDL_Window*window;}video_t;
typedef struct {int x,y,rawX,rawY,absX,absY;} mouse_t;
extern mouse_t mouse;
extern module_t*song;extern editor_t editor;extern ui_t ui;extern cursor_t cursor;extern video_t video;
