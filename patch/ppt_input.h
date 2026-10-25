#include <stdbool.h>
#pragma once
// Pocket Pro Tracker input adaptor for 8bitbubsy/pt2-clone v1.92
void ppt_input_init(void);
void ppt_input_update(void);
void ppt_input_close(void);

/* True while pad editor holds ROW/PATTERN independently of playback. */
bool ppt_live_edit_active(void);

/* Published from PT2's audio-tick code (pattern index, actual played ROW). */
void ppt_set_playhead(int pattern, int row);
/* Returns playhead ROW 0..63 only if the edited pattern is playing and
   LIVE EDIT is active; -1 otherwise. Safe to call on the GUI thread. */
int ppt_ghost_row(int displayPattern);

/* True only while SELECT+B rectangular selection is displayed. */
bool ppt_selection_contains(int pattern, int row, int channel, int field);
