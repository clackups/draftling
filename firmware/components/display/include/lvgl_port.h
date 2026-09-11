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

/* Zero both LVGL draw buffers to drop stale rendered pixels after a
 * layout change (e.g. a base-font-size change). Needed on the
 * reflective-LCD FULL render-mode path where LVGL alternates two
 * screen-sized buffers and can flush a stale one, causing garbage.
 * Call with the LVGL lock held. */
void draftling_lvgl_port_clear_buffers(void);

/* Map a rectangle from LVGL's logical (post-rotation) coordinate space
 * into physical panel coordinates, applying the same transform
 * flush_cb() uses to place each rendered tile -- the current base
 * rotation, the "Display orientation" portrait setting (folded into
 * the base rotation at boot) and the runtime 180-degree flip all
 * together.
 *
 * display_set_partial_clip() takes a one-shot clip rectangle that is
 * intersected against the dirty bounding box display_push_rgb565()
 * already accumulated in *physical* coordinates (flush_cb maps every
 * pushed tile through this same transform first). A caller outside
 * lvgl_port.cpp that computes a clip rectangle from LVGL widget
 * positions -- which are logical, i.e. pre-rotation -- must map it
 * through here first, or the clip is nonsensical whenever a rotation
 * is in effect (portrait, or a non-zero base/flip rotation): it either
 * intersects to nothing, silently dropping every partial-refresh flush
 * so the panel stops updating until a full, unclipped refresh runs. */
void draftling_lvgl_port_map_to_physical(int x, int y, int w, int h,
                                         int *px, int *py, int *pw, int *ph);

#ifdef __cplusplus
}
#endif
