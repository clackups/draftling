#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/*
 * User-selectable display orientation: landscape (the default on every
 * board) or portrait (the whole UI rotated another 90 degrees on top of
 * the board's build-time base rotation).
 *
 * Like the screen margins (display_margins.h), this is FROZEN for the
 * life of the session: the LVGL display resolution and the entire
 * widget tree are built once at boot from this value, so a change only
 * takes effect after a restart. main.cpp calls
 * display_orientation_init() once, right after display_margins_init()
 * and before draftling_lvgl_port_init(), so every consumer -- the LVGL
 * rotation angle, the editor's SCR_W / SCR_H, the split-screen axis --
 * sees one consistent value for the rest of the session.
 *
 * display_orientation_is_portrait() always returns that frozen copy,
 * NOT whatever was most recently saved via
 * display_orientation_set_portrait(); the Settings UI tracks the
 * pending value itself (display_orientation_get_pending_portrait()).
 */
void display_orientation_init(void);

/* Frozen session value: true = portrait, false = landscape. */
bool display_orientation_is_portrait(void);

/* Persist a new orientation to NVS for the *next* boot's
 * display_orientation_init() to pick up. Does NOT change what
 * display_orientation_is_portrait() returns this session. */
void display_orientation_set_portrait(bool portrait);

/* Raw NVS-persisted value, which may differ from the frozen
 * display_orientation_is_portrait() value if it was changed this
 * session (it takes effect on the next restart). Used by the Settings
 * UI to seed its pending-value state. */
bool display_orientation_get_pending_portrait(void);

#ifdef __cplusplus
}
#endif
