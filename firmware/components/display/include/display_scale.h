#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/*
 * User-selectable "2x scale" setting (F1 -> Settings), for visually
 * impaired users: off (the default on every board) or on, which makes
 * LVGL render into a canvas half the panel's width and height and
 * draws every rendered pixel as a 2x2 block on the panel
 * (lvgl_port.cpp's flush_cb). Everything on screen -- text, bars,
 * dialogs -- becomes twice as large, with a quarter of the content.
 *
 * Like display_flip.h, this is FROZEN for the life of the session: the
 * LVGL display resolution is fixed once draftling_lvgl_port_init()
 * runs, so a change only takes effect after a restart. main.cpp calls
 * display_scale_init() once, alongside display_flip_init() and before
 * draftling_lvgl_port_init(), so every consumer (the LVGL canvas size,
 * the touchscreen's logical frame, the editor's SCR_W / SCR_H) sees one
 * consistent value for the rest of the session.
 */
void display_scale_init(void);

/* Frozen session value: 2 when the 2x scale is on, else 1. */
int display_scale_factor(void);

/* Persist a new value to NVS for the *next* boot's display_scale_init()
 * to pick up. Does NOT change what display_scale_factor() returns this
 * session. */
void display_scale_set_2x(bool on);

/* Raw NVS-persisted value, which may differ from the frozen session
 * value if it was changed this session. Used by the Settings UI. */
bool display_scale_get_pending_2x(void);

#ifdef __cplusplus
}
#endif
