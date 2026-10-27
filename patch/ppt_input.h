#include <stdint.h>
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

/* POSITION SELECT: separate editor selection and SONG-order playback loop. */
bool ppt_position_browse_active(void);
int ppt_loop_redirect(int proposed, int length);
int ppt_loop_entry_style(int entry);

/* Fullscreen view is a transient display overlay; never changes audio. */
bool ppt_full_pattern_active(void);
void ppt_views_begin(void);
void ppt_views_end(void);
void ppt_disk_move(int direction);
void ppt_disk_activate(void);
int ppt_disk_selected_row(void);

void ppt_disk_draw_full(void);

void ppt_mod_restore_dialog(const uint32_t *frame);

void ppt_position_draw_full(void);
int ppt_position_cursor(void);
void ppt_position_set_cursor(int pos);

int ppt_position_first(void);

bool ppt_position_pattern_active(void);
