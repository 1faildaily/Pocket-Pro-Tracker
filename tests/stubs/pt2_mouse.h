#pragma once
#include <stdint.h>
void pointerSetMode(uint8_t mode,bool carry);
void sampleUpButton(void);
void sampleDownButton(void);

#define PTB_SONGNAME 1
#define PTB_SAMPLENAME 2

#define PTB_PE_PATTNAME 1234
