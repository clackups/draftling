#include <cstdio>
#include <cstring>
#include "sdkconfig.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <esp_heap_caps.h>
#include "lvgl.h"

#include "lvgl_port.h"
#include "display.h"

static const char *TAG = "LvglPort";

#define LVGL_TICK_PERIOD_MS    5
#define LVGL_TASK_MAX_DELAY_MS 500
#define LVGL_TASK_MIN_DELAY_MS 50
#define BYTES_PER_PIXEL        2   /* LV_COLOR_FORMAT_RGB565 */

static SemaphoreHandle_t s_lvgl_mux = NULL;

/* User-requested display rotation (DRAFTLING_DISPLAY_ROTATE_ANGLE).
 * LVGL v9.2's lv_display_set_rotation only swaps the reported
 * horizontal / vertical resolution -- it does NOT rotate the
 * rendered buffer before handing it to the flush callback. We
 * therefore software-rotate each partial tile here, in flush_cb,
 * before pushing it to the display backend.
 *
 * s_phys_w / s_phys_h are the panel dimensions in the
 * pre-user-rotation (backend-native) orientation; flush_cb maps
 * the LVGL-coordinate tile back into this space.
 *
 * s_rotate_deg is the *effective* rotation used by flush_cb. It is
 * the build-time base rotation (s_base_rotate_deg, from
 * DRAFTLING_DISPLAY_ROTATE_ANGLE) optionally combined with a runtime
 * 180-degree flip toggled from the editor's Settings menu
 * (draftling_lvgl_port_set_flip180). A 180 flip never changes the
 * reported panel resolution, so it can be applied at runtime without
 * rebuilding the LVGL widget tree -- only a full repaint is needed. */
static int       s_base_rotate_deg = 0;
static bool      s_flip180         = false;
static int       s_rotate_deg = 0;
static lv_display_t *s_disp    = NULL;
static int       s_phys_w     = 0;
static int       s_phys_h     = 0;
static uint16_t *s_rot_buf    = NULL;
static size_t    s_rot_buf_px = 0;

/* Rotate an RGB565 tile (w x h) in src by 90 / 180 / 270 degrees CW
 * into dst. Caller guarantees dst has enough room for w*h pixels. */
static void rotate_tile_rgb565(const uint16_t *src, uint16_t *dst,
                               int w, int h, int rotate_deg)
{
    switch (rotate_deg) {
    case 90: {
        /* (i, j) -> (h - 1 - j, i); dest is h wide, w tall */
        int dst_w = h;
        for (int j = 0; j < h; j++) {
            int di = h - 1 - j;
            const uint16_t *srow = src + (size_t)j * w;
            for (int i = 0; i < w; i++) {
                dst[(size_t)i * dst_w + di] = srow[i];
            }
        }
        break;
    }
    case 180: {
        /* (i, j) -> (w - 1 - i, h - 1 - j) */
        for (int j = 0; j < h; j++) {
            const uint16_t *srow = src + (size_t)j * w;
            uint16_t *drow = dst + (size_t)(h - 1 - j) * w;
            for (int i = 0; i < w; i++) {
                drow[w - 1 - i] = srow[i];
            }
        }
        break;
    }
    case 270: {
        /* (i, j) -> (j, w - 1 - i); dest is h wide, w tall */
        int dst_w = h;
        for (int j = 0; j < h; j++) {
            const uint16_t *srow = src + (size_t)j * w;
            for (int i = 0; i < w; i++) {
                dst[(size_t)(w - 1 - i) * dst_w + j] = srow[i];
            }
        }
        break;
    }
    default:
        break;
    }
}

/* Map the LVGL (logical, post-rotation) tile area to physical panel
 * coordinates and dimensions. */
static void map_area_to_physical(const lv_area_t *area, int rotate_deg,
                                 int phys_w, int phys_h,
                                 int *px, int *py, int *pw, int *ph)
{
    int x1 = area->x1, y1 = area->y1, x2 = area->x2, y2 = area->y2;
    int w  = x2 - x1 + 1;
    int h  = y2 - y1 + 1;

    switch (rotate_deg) {
    case 90:
        *px = phys_w - 1 - y2;
        *py = x1;
        *pw = h;
        *ph = w;
        break;
    case 180:
        *px = phys_w - 1 - x2;
        *py = phys_h - 1 - y2;
        *pw = w;
        *ph = h;
        break;
    case 270:
        *px = y1;
        *py = phys_h - 1 - x2;
        *pw = h;
        *ph = w;
        break;
    default:
        *px = x1;
        *py = y1;
        *pw = w;
        *ph = h;
        break;
    }
}

static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *color_map)
{
    int w = area->x2 - area->x1 + 1;
    int h = area->y2 - area->y1 + 1;
    bool last = lv_display_flush_is_last(disp);

    /* Software-rotate the tile into the backend-native panel
     * orientation, then translate the area to physical coords. The
     * RLCD per-pixel fallback further down also needs rotated
     * coordinates, so we always do the mapping when a user rotation
     * is configured. */
    int phys_x = area->x1, phys_y = area->y1, phys_w = w, phys_h = h;
    uint8_t *push_buf = color_map;

    if (s_rotate_deg == 90 || s_rotate_deg == 180 || s_rotate_deg == 270) {
        size_t px_count = (size_t)w * (size_t)h;
        if (px_count > s_rot_buf_px) {
            if (s_rot_buf) heap_caps_free(s_rot_buf);
            s_rot_buf = (uint16_t *)heap_caps_malloc(px_count * sizeof(uint16_t),
                                                    MALLOC_CAP_SPIRAM);
            assert(s_rot_buf);
            s_rot_buf_px = px_count;
        }
        rotate_tile_rgb565((const uint16_t *)color_map, s_rot_buf,
                           w, h, s_rotate_deg);
        push_buf = (uint8_t *)s_rot_buf;
        map_area_to_physical(area, s_rotate_deg, s_phys_w, s_phys_h,
                             &phys_x, &phys_y, &phys_w, &phys_h);
    }

    /* Fast path: if the backend supports it, push the LVGL RGB565
     * framebuffer directly to the panel without going through our
     * per-pixel 1-bpp conversion. The epdiy and color LCD backends
     * use this; the RLCD backend returns false and we fall back to
     * the legacy per-pixel path below. */
    if (display_push_rgb565(phys_x, phys_y, phys_w, phys_h, push_buf)) {
        /* LVGL may slice a single dirty region into multiple
         * draw-buffer-sized chunks (in PARTIAL render mode) or invalidate
         * several disjoint regions in one cycle. Only trigger the panel
         * refresh on the final chunk so all pushes accumulate into a
         * single dirty bounding box and produce a single e-paper
         * refresh - otherwise the user sees the same screen update
         * twice (once for the cursor and once for the title bar) on
         * every keystroke. */
        if (last) {
            display_flush();
        }
        lv_disp_flush_ready(disp);
        return;
    }

    uint16_t *buf = (uint16_t *)color_map;
    for (int y = area->y1; y <= area->y2; y++) {
        for (int x = area->x1; x <= area->x2; x++) {
#if defined(CONFIG_DRAFTLING_DISPLAY_COLOR)
            /* Color backends should always have handled the push via
             * display_push_rgb565() above; reaching here means the
             * backend declined a region we cannot otherwise convert
             * faithfully (we have no per-pixel color setter). Drop
             * the per-pixel call so we do not collapse the color
             * image to monochrome. */
            (void)x; (void)y; (void)buf;
#else
            uint8_t color = (*buf < 0x7FFF) ? 0x00 : 0xFF;
            int px = x, py = y;
            switch (s_rotate_deg) {
            case 90:  px = s_phys_w - 1 - y; py = x; break;
            case 180: px = s_phys_w - 1 - x; py = s_phys_h - 1 - y; break;
            case 270: px = y; py = s_phys_h - 1 - x; break;
            default:  break;
            }
            display_set_pixel(px, py, color);
#endif
            buf++;
        }
    }
    if (last) {
        display_flush();
    }
    lv_disp_flush_ready(disp);
}

static void tick_cb(void *arg)
{
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

static void lvgl_task(void *arg)
{
    uint32_t delay_ms = LVGL_TASK_MAX_DELAY_MS;
    for (;;) {
        if (draftling_lvgl_port_lock(-1)) {
            delay_ms = lv_timer_handler();
            draftling_lvgl_port_unlock();
        }
        if (delay_ms > LVGL_TASK_MAX_DELAY_MS) delay_ms = LVGL_TASK_MAX_DELAY_MS;
        if (delay_ms < LVGL_TASK_MIN_DELAY_MS) delay_ms = LVGL_TASK_MIN_DELAY_MS;
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

extern "C" void draftling_lvgl_port_init(int width, int height, int rotate_deg)
{
    /* Stash the user-requested rotation and the backend-native panel
     * dimensions so flush_cb can software-rotate each LVGL tile back
     * into physical panel coordinates (LVGL v9.2 swaps the reported
     * horizontal/vertical resolution after lv_display_set_rotation
     * but does not rotate the rendered buffer for us). */
    s_base_rotate_deg = (rotate_deg % 360 + 360) % 360;
    if (s_base_rotate_deg != 0 && s_base_rotate_deg != 90 &&
        s_base_rotate_deg != 180 && s_base_rotate_deg != 270) {
        s_base_rotate_deg = 0;
    }
    s_flip180    = false;
    s_rotate_deg = s_base_rotate_deg;
    s_phys_w = width;
    s_phys_h = height;

    /* Recursive mutex so a task that already holds it can call
     * draftling_lvgl_port_lock() again without deadlocking. The "Sleep now"
     * menu path is the motivating case: it runs inside the LVGL
     * timer/key handler (mutex held) and then calls
     * standby_enter_sleep() -> pre_sleep_autosave(), which itself
     * takes draftling_lvgl_port_lock() to wipe the panel to white before deep
     * sleep. With a plain mutex the second take would block forever
     * and the screen would never get the white frame. */
    s_lvgl_mux = xSemaphoreCreateRecursiveMutex();
    lv_init();

    lv_display_t *disp = lv_display_create(width, height);
    s_disp = disp;
    /* LVGL v9 defaults to LV_COLOR_FORMAT_RGB888 (4 bytes per pixel).
     * Our flush path expects RGB565, and BYTES_PER_PIXEL below is
     * sized for RGB565. Declare it explicitly before allocating
     * buffers; otherwise LVGL writes 4-byte pixels into a half-sized
     * buffer and the panel renders mangled, ~4x-shrunk content. */
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(disp, flush_cb);

    /* Apply display rotation */
    lv_display_rotation_t rot = LV_DISPLAY_ROTATION_0;
    switch (s_rotate_deg) {
    case 90:  rot = LV_DISPLAY_ROTATION_90;  break;
    case 180: rot = LV_DISPLAY_ROTATION_180; break;
    case 270: rot = LV_DISPLAY_ROTATION_270; break;
    default:  rot = LV_DISPLAY_ROTATION_0;   break;
    }
    lv_display_set_rotation(disp, rot);

#if defined(CONFIG_DRAFTLING_DISPLAY_EPD) || \
    defined(CONFIG_DRAFTLING_DISPLAY_COLOR)
    /* E-paper / colour LCD path: use PARTIAL render mode so flush_cb is
     * called once per invalidated rectangle with a tightly-packed
     * pixel buffer. This lets the display backend issue a partial
     * e-paper refresh of just the changed region (typically the
     * cursor or a single edited line) instead of repainting all
     * 540x960 pixels and running the full grayscale waveform on
     * every keystroke.
     *
     * AXS15231B color LCDs use the same PARTIAL mode: the backend
     * keeps an RGB565 framebuffer in PSRAM and streams just the
     * dirty rectangle over QSPI on every flush, avoiding a full
     * panel repaint per keystroke.
     *
     * A draw buffer sized to ~1/8 of the screen is enough to hold any
     * single LVGL invalidated region the editor produces (status bar,
     * a line of text, a dialog box) in one flush. */
    size_t buf_size = (size_t)width * height * BYTES_PER_PIXEL / 8;
    uint8_t *buf1 = (uint8_t *)heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);
    uint8_t *buf2 = (uint8_t *)heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);
    assert(buf1 && buf2);
    lv_display_set_buffers(disp, buf1, buf2, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
#else
    size_t buf_size = (size_t)width * height * BYTES_PER_PIXEL;
    uint8_t *buf1 = (uint8_t *)heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);
    uint8_t *buf2 = (uint8_t *)heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);
    assert(buf1 && buf2);
    lv_display_set_buffers(disp, buf1, buf2, buf_size, LV_DISPLAY_RENDER_MODE_FULL);
#endif

    /* Tick timer */
    esp_timer_create_args_t timer_args = {};
    timer_args.callback = tick_cb;
    timer_args.name     = "lvgl_tick";
    esp_timer_handle_t timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(timer, LVGL_TICK_PERIOD_MS * 1000));

    xTaskCreatePinnedToCore(lvgl_task, "LVGL", 8 * 1024, NULL, 5, NULL, 0);
    ESP_LOGI(TAG, "LVGL port initialized (%dx%d, rotation=%d)", width, height, s_rotate_deg);
}

/* Runtime 180-degree flip toggle.
 *
 * The effective rotation handed to LVGL and flush_cb is the build-time
 * base rotation optionally combined with an extra 180 degrees. A 180
 * flip leaves the reported horizontal/vertical resolution unchanged
 * (unlike a 90/270 swap), so the LVGL widget tree does not need to be
 * rebuilt -- we just update lv_display_set_rotation, recompute the
 * effective rotation used by flush_cb's software-rotate path, and
 * invalidate the active screen so the whole panel repaints in the new
 * orientation. Safe to call from the LVGL task context (e.g. the
 * Settings key handler), which already holds the LVGL lock. */
extern "C" void draftling_lvgl_port_set_flip180(bool flip)
{
    if (!s_disp) return;
    if (s_flip180 == flip) return;
    s_flip180    = flip;
    s_rotate_deg = (s_base_rotate_deg + (flip ? 180 : 0)) % 360;

    lv_display_rotation_t rot = LV_DISPLAY_ROTATION_0;
    switch (s_rotate_deg) {
    case 90:  rot = LV_DISPLAY_ROTATION_90;  break;
    case 180: rot = LV_DISPLAY_ROTATION_180; break;
    case 270: rot = LV_DISPLAY_ROTATION_270; break;
    default:  rot = LV_DISPLAY_ROTATION_0;   break;
    }
    lv_display_set_rotation(s_disp, rot);

    /* Repaint everything in the new orientation. display_clear() also
     * flags the next flush as a full refresh on backends that track
     * it, clearing any residue the rotated layout does not overwrite. */
    display_clear(0xFF);
    lv_obj_invalidate(lv_screen_active());
    ESP_LOGI(TAG, "Display flip180=%d (effective rotation=%d)",
             (int)flip, s_rotate_deg);
}

extern "C" bool draftling_lvgl_port_get_flip180(void)
{
    return s_flip180;
}

extern "C" bool draftling_lvgl_port_lock(int timeout_ms)
{
    TickType_t t = (timeout_ms == -1) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTakeRecursive(s_lvgl_mux, t) == pdTRUE;
}

extern "C" void draftling_lvgl_port_unlock(void)
{
    xSemaphoreGiveRecursive(s_lvgl_mux);
}
