#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/*
 * User-selectable "Display upside down" setting: off (the default on
 * every board) or on, which adds another 180 degrees on top of
 * whatever rotation the build-time base rotation and the "Display
 * orientation" (landscape/portrait) setting already produce.
 *
 * Like display_orientation.h, this is FROZEN for the life of the
 * session: the LVGL rotation angle is baked into
 * DISPLAY_ROTATE_EFFECTIVE (main/app_config.h) and handed to
 * draftling_lvgl_port_init() once at boot, so a change only takes
 * effect after a restart. main.cpp calls display_flip_init() once,
 * alongside display_orientation_init() and before
 * draftling_lvgl_port_init(), so every consumer sees one consistent
 * value for the rest of the session.
 *
 * display_flip_is_upside_down() always returns that frozen copy, NOT
 * whatever was most recently saved via display_flip_set_upside_down();
 * the Settings UI tracks the pending value itself
 * (display_flip_get_pending_upside_down()).
 */
void display_flip_init(void);

/* Frozen session value: true = upside down, false = normal. */
bool display_flip_is_upside_down(void);

/* Persist a new value to NVS for the *next* boot's display_flip_init()
 * to pick up. Does NOT change what display_flip_is_upside_down()
 * returns this session. */
void display_flip_set_upside_down(bool upside_down);

/* Raw NVS-persisted value, which may differ from the frozen
 * display_flip_is_upside_down() value if it was changed this session
 * (it takes effect on the next restart). Used by the Settings UI to
 * seed its pending-value state. */
bool display_flip_get_pending_upside_down(void);

#ifdef __cplusplus
}
#endif
