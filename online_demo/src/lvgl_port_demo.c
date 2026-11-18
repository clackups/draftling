/*
 * Online demo: LVGL display and touch port.
 *
 * LVGL renders the same logical screen as on the device (480x320 minus
 * the screen margins, turned for portrait, halved for 2x scale) into a
 * frame buffer here; after each pass of the LVGL loop the changed
 * rectangle goes to the page's canvas (demo_host_present()). The page
 * turns, scales and dims the canvas itself, so no software rotation is
 * needed. Mouse and touch input on the canvas arrive through
 * demo_pointer() in LVGL coordinates and drive a pointer input device,
 * as the FT6336U touch controller does on the board.
 */

#include <stdlib.h>
#include <string.h>

#include <emscripten.h>

#include "demo_host.h"
#include "display_flip.h"
#include "lvgl.h"
#include "lvgl_port.h"

static lv_display_t *s_disp;
static uint16_t *s_fb;
static int s_w, s_h, s_scale = 1;
static int s_dx1, s_dy1, s_dx2 = -1, s_dy2 = -1;   /* dirty box */

static int s_ptr_x, s_ptr_y;
static bool s_ptr_down;

static uint32_t tick_cb(void)
{
    return (uint32_t)emscripten_get_now();
}

static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px)
{
    int w = lv_area_get_width(area);
    const uint16_t *src = (const uint16_t *)px;
    for (int y = area->y1; y <= area->y2; y++) {
        memcpy(&s_fb[y * s_w + area->x1], src, (size_t)w * 2);
        src += w;
    }
    if (s_dx2 < 0) {
        s_dx1 = area->x1; s_dy1 = area->y1; s_dx2 = area->x2; s_dy2 = area->y2;
    } else {
        if (area->x1 < s_dx1) s_dx1 = area->x1;
        if (area->y1 < s_dy1) s_dy1 = area->y1;
        if (area->x2 > s_dx2) s_dx2 = area->x2;
        if (area->y2 > s_dy2) s_dy2 = area->y2;
    }
    lv_display_flush_ready(disp);
}

static void pointer_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    data->point.x = s_ptr_x;
    data->point.y = s_ptr_y;
    data->state = s_ptr_down ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

EMSCRIPTEN_KEEPALIVE void demo_pointer(int x, int y, int pressed)
{
    s_ptr_x = x < 0 ? 0 : (x >= s_w ? s_w - 1 : x);
    s_ptr_y = y < 0 ? 0 : (y >= s_h ? s_h - 1 : y);
    s_ptr_down = pressed != 0;
}

void draftling_lvgl_port_init(int width, int height, int rotate_deg, int scale)
{
    lv_init();
    lv_tick_set_cb(tick_cb);

    s_scale = scale > 1 ? scale : 1;
    s_w = width / s_scale;
    s_h = height / s_scale;
    if (rotate_deg == 90 || rotate_deg == 270) {
        int t = s_w; s_w = s_h; s_h = t;
    }
    s_fb = (uint16_t *)calloc((size_t)s_w * s_h, 2);

    s_disp = lv_display_create(s_w, s_h);
    lv_display_set_color_format(s_disp, LV_COLOR_FORMAT_RGB565);
    size_t buf_bytes = (size_t)s_w * (s_h / 4 + 1) * 2;
    void *b1 = malloc(buf_bytes);
    void *b2 = malloc(buf_bytes);
    lv_display_set_buffers(s_disp, b1, b2, buf_bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(s_disp, flush_cb);

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, pointer_read_cb);

    demo_host_set_screen(s_w, s_h, s_scale, display_flip_is_upside_down());
}

/* After every LVGL pass: hand the changed rectangle to the page. */
void demo_lvgl_port_present(void)
{
    if (s_dx2 < 0) return;
    demo_host_present(s_fb, s_w, s_dx1, s_dy1, s_dx2 - s_dx1 + 1, s_dy2 - s_dy1 + 1);
    s_dx2 = s_dy2 = -1;
}

/* Everything runs on the browser's main thread: nothing to lock. */
bool draftling_lvgl_port_lock(int timeout_ms)
{
    (void)timeout_ms;
    return true;
}

void draftling_lvgl_port_unlock(void) { }

void draftling_lvgl_port_set_flip180(bool flip) { (void)flip; }

void draftling_lvgl_port_clear_buffers(void) { }

void draftling_lvgl_port_map_to_physical(int x, int y, int w, int h,
                                         int *px, int *py, int *pw, int *ph)
{
    *px = x * s_scale;
    *py = y * s_scale;
    *pw = w * s_scale;
    *ph = h * s_scale;
}
