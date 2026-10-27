#pragma once
#include <stdbool.h>
void ppt_bpm_draw(void);
void ppt_bpm_begin(bool target);
void ppt_bpm_commit(bool apply);
bool ppt_bpm_editing(void);
void ppt_bpm_direction(int d);
void ppt_bpm_next(void);
void ppt_bpm_previous(void);
bool ppt_bpm_apply(void);
