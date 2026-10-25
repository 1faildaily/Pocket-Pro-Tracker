#include "pt2_visuals.h"
static int32_t periodToNote(int32_t v) { return v; }
void drawPatternNormal(void) {
  if (row < 0 || row >= MOD_ROWS) { }
  else if (i == MIDDLE_ROW) {
    for (int32_t j = 0; j < PAULA_VOICES; j++, note++) {
      textOutBigBg(x, y, noteNames[periodToNote(note->period)], video.palette[PAL_GENTXT], video.palette[PAL_GENBKG]);
      charOutBigBg(x, y, smpChar, video.palette[PAL_GENTXT], video.palette[PAL_GENBKG]);
      printOneHexBigBg(x, y, note->sample & 0x0F, video.palette[PAL_GENTXT], video.palette[PAL_GENBKG]);
      printOneHexBigBg(x, y, note->command, video.palette[PAL_GENTXT], video.palette[PAL_GENBKG]);
      printTwoHexBigBg(x, y, note->param, video.palette[PAL_GENTXT], video.palette[PAL_GENBKG]);
    }
  }
  else // non-middle rows
  {
    printTwoDecimalsBg(8, y, row, video.palette[PAL_PATTXT], video.palette[PAL_BACKGRD]);
    for (int32_t j = 0; j < PAULA_VOICES; j++, note++) {
      textOutBg(x, y, noteNames[periodToNote(note->period)], video.palette[PAL_PATTXT], video.palette[PAL_BACKGRD]);
      charOutBg(x, y, smpChar, video.palette[PAL_PATTXT], video.palette[PAL_BACKGRD]);
      printOneHexBg(x, y, note->sample & 0x0F, video.palette[PAL_PATTXT], video.palette[PAL_BACKGRD]);
      printOneHexBg(x, y, note->command, video.palette[PAL_PATTXT], video.palette[PAL_BACKGRD]);
      printTwoHexBg(x, y, note->param, video.palette[PAL_PATTXT], video.palette[PAL_BACKGRD]);
    }
  }
}
void drawPatternDotted(void) {
  if (row < 0 || row >= MOD_ROWS) { }
  else if (i == MIDDLE_ROW) {
    for (int32_t j = 0; j < PAULA_VOICES; j++, note++) {
      textOutBigBg(x, y, noteNames[periodToNote(note->period)], video.palette[PAL_GENTXT], video.palette[PAL_GENBKG]);
      textOutBigBg(x, y, emptyDottedSample, video.palette[PAL_GENTXT], video.palette[PAL_GENBKG]);
      charOutBigBg(x, y, smpChar, video.palette[PAL_GENTXT], video.palette[PAL_GENBKG]);
      printOneHexBigBg(x, y, note->sample & 0x0F, video.palette[PAL_GENTXT], video.palette[PAL_GENBKG]);
      textOutBigBg(x, y, emptyDottedEffect, video.palette[PAL_GENTXT], video.palette[PAL_GENBKG]);
      printOneHexBigBg(x, y, note->command, video.palette[PAL_GENTXT], video.palette[PAL_GENBKG]);
      printTwoHexBigBg(x, y, note->param, video.palette[PAL_GENTXT], video.palette[PAL_GENBKG]);
    }
  }
  else // non-middle rows
  {
    printTwoDecimalsBg(8, y, row, video.palette[PAL_PATTXT], video.palette[PAL_BACKGRD]);
    for (int32_t j = 0; j < PAULA_VOICES; j++, note++) {
      textOutBg(x, y, noteNames[periodToNote(note->period)], video.palette[PAL_PATTXT], video.palette[PAL_BACKGRD]);
      textOutBg(x, y, emptyDottedSample, video.palette[PAL_PATTXT], video.palette[PAL_BACKGRD]);
      charOutBg(x, y, smpChar, video.palette[PAL_PATTXT], video.palette[PAL_BACKGRD]);
      printOneHexBg(x, y, note->sample & 0x0F, video.palette[PAL_PATTXT], video.palette[PAL_BACKGRD]);
      textOutBg(x, y, emptyDottedEffect, video.palette[PAL_PATTXT], video.palette[PAL_BACKGRD]);
      printOneHexBg(x, y, note->command, video.palette[PAL_PATTXT], video.palette[PAL_BACKGRD]);
      printTwoHexBg(x, y, note->param, video.palette[PAL_PATTXT], video.palette[PAL_BACKGRD]);
    }
  }
}
