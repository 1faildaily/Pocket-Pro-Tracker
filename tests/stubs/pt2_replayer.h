#pragma once
#include <stdint.h>
void modPlay(int16_t pattern,int16_t pos,int8_t row);
void modStop(void);
void setPattern(int16_t pattern);

void incPatt(void);
void decPatt(void);
void modSetPos(int16_t pos,int16_t row);
