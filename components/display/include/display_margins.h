#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/*
 * User-adjustable panel margins: pixels of physical panel hidden
 * under an opaque bezel/enclosure cover on each edge (e.g. a
 * recessed cutout whose cover overlaps the glass). Zero on every
 * board by default; adjustable via the editor's F1 -> Settings menu
 * and persisted in NVS.
 *
 * display_margins_init() loads the persisted values (0 if none are
 * stored yet) into a FROZEN, session-lifetime copy. main.cpp calls it
 * once, very early in app_main() -- right after nvs_flash_init() and
 * before display_init() / draftling_lvgl_port_init() -- so every
 * consumer of the panel's logical width/height (the LVGL display
 * resolution, the editor's SCR_W/SCR_H, the touchscreen's
 * logical_width/height, and, on backends that offset writes into a
 * larger physical framebuffer, the backend itself) sees the same
 * value for the rest of the session.
 *
 * display_margin_left/right/top/bottom() always return that frozen
 * copy -- NOT whatever was most recently saved via
 * display_margins_set() -- because the LVGL display resolution is
 * fixed for the life of the session (re-deriving it live would mean
 * tearing down and rebuilding the entire LVGL display and widget
 * tree). If these returned the live NVS value, SCR_W/SCR_H would
 * start disagreeing with LVGL's actual (unchanged) canvas size the
 * moment a setting was saved, before the restart that is supposed to
 * apply it.
 */
void display_margins_init(void);
int  display_margin_left(void);
int  display_margin_right(void);
int  display_margin_top(void);
int  display_margin_bottom(void);

/* Persist new margin values to NVS for the *next* boot's
 * display_margins_init() to pick up. Does NOT change what
 * display_margin_*() returns this session -- see the file comment
 * above; the Settings UI must track the pending value itself
 * (display_margins_get_pending()) if it wants to display or cycle it
 * before a restart. */
void display_margins_set(int left, int right, int top, int bottom);

/* Read back the raw NVS-persisted margin values (which may differ
 * from the frozen display_margin_*() values above if changed this
 * session -- they take effect on the next restart). Any output
 * pointer may be NULL. Used by the Settings UI to seed its own
 * pending-value state when the settings screen is built. */
void display_margins_get_pending(int *left, int *right, int *top, int *bottom);

#ifdef __cplusplus
}
#endif
