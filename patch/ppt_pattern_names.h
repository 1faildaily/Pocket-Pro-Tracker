#pragma once
#include <stdbool.h>
char *ppt_pattern_name(int pattern);
bool ppt_pattern_names_save(const char *modulePath);
void ppt_pattern_names_prepare(const char *modulePath);
bool ppt_pattern_names_load(void);
