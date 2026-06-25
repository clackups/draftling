#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

void draftling_lvgl_port_init(int width, int height, int rotate_deg);
bool draftling_lvgl_port_lock(int timeout_ms);
void draftling_lvgl_port_unlock(void);

/* Runtime 180-degree display flip. The effective rotation is the
 * build-time base rotation plus an optional 180 degrees; a 180 flip
 * does not change the reported resolution, so it can be toggled live
 * without rebuilding the LVGL widget tree. */
void draftling_lvgl_port_set_flip180(bool flip);
bool draftling_lvgl_port_get_flip180(void);

#ifdef __cplusplus
}
#endif
