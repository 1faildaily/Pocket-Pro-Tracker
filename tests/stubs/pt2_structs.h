#pragma once
#include "pt2_header.h"
#include <stdint.h>
#include <stdbool.h>
typedef struct {uint8_t param,sample,command;uint16_t period;} note_t;
typedef struct {uint16_t patternTable[128];uint16_t songLength;char name[21];} header_t;
typedef struct {char text[23];int32_t length;} moduleSample_t;
typedef struct {unsigned int currPattern, currPos; int currRow; int row; bool modified; header_t header; moduleSample_t samples[31]; note_t *patterns[MAX_PATTERNS];} module_t;
typedef struct {int32_t markStartOfs,markEndOfs,samplePos;bool sampleZero;int8_t currSample;bool muted[4];bool programRunning, songPlaying, mixFlag; int playMode,currMode,recordMode;uint8_t editMoveAdd,resampleNote,currPlayNote;uint16_t *currPatternDisp,*currPosEdPattDisp;} editor_t;
typedef struct {int editOpScreen;bool updateSongLength,updateSongSize,updatePatternData, updateTrackerFlags, updateCurrSampleNum,updateSongPattern,updateSongName,updateCurrSampleName,updateCurrPattText,updateResampleNote,changingSmpResample,updateSongPos,updatePosEd,posEdScreenShown,editTextFlag,askBoxShown,diskOpScreenShown,samplerScreenShown,editOpScreenShown,aboutScreenShown;} ui_t;
typedef struct {int pos,channel;} cursor_t;
typedef struct {SDL_Window*window;}video_t;
typedef struct {int x,y,rawX,rawY,absX,absY;} mouse_t;
extern mouse_t mouse;
extern module_t*song;extern editor_t editor;extern ui_t ui;extern cursor_t cursor;extern video_t video;

typedef struct {bool locked;} audio_t;
extern audio_t audio;

typedef struct {int type,object;char *textPtr,*textStartPtr,*textEndPtr;} textEdit_t;
extern textEdit_t textEdit;

typedef struct {bool shiftPressed;} keyb_t;
extern keyb_t keyb;
