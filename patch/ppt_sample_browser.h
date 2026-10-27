#pragma once
#include <stdint.h>
#include <stdbool.h>
void ppt_browser_update(void);
void ppt_browser_preview(void);
void ppt_browser_draw(void);
void ppt_browser_mix(int16_t *stream,int frames);
void ppt_browser_close(void);
bool ppt_browser_import(const char *path,const char *name);
bool ppt_browser_import_dialog(void);
int ppt_browser_target_length(int pitch);
void ppt_set_import_note(int pitch);

bool ppt_browser_sample_mode(void);
