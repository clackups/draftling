#include "sdkconfig.h"
#if defined(CONFIG_DRAFTLING_DISPLAY_MIPI_DSI)

/*
 * M5Stack Tab5 (ESP32-P4) MIPI-DSI color LCD driver.
 *
 * Hardware
 * --------
 * M5Stack Tab5 is a 5", 1280 x 720 IPS tablet built on the
 * ESP32-P4 SoC. The panel is wired over a 2-lane MIPI-DSI link
 * (~1 Gbps per lane) and exists in two hardware revisions which
 * the upstream espressif/m5stack_tab5 BSP auto-detects by I2C
 * probe:
 *   - v1: Ilitek ILI9881C panel + Goodix GT911 touch controller
 *         (touch at I2C 0x14, backup address forced via INT-low
 *         on power-up).
 *   - v2: Sitronix ST7123 panel + ST7123 integrated touch.
 *
 * Both revisions share the same MIPI-DSI bus, the same PI4IOE5V6408
 * I/O expander (LCD_EN, TOUCH_EN, USB_EN, speaker, camera, wifi
 * power rails), the same LEDC backlight pin (GPIO22) and the same
 * Touch-INT pin (GPIO23). Panel reset is wired through the I/O
 * expander; the BSP owns that init too.
 *
 * Strategy
 * --------
 * We delegate every panel-specific detail (auto-detect, MIPI-DSI
 * PHY LDO power-up via esp_ldo_acquire_channel(), DPI timing, init
 * register sequence, IO-expander wiring, backlight LEDC config) to
 * the BSP's lower-level non-LVGL helpers:
 *
 *   - bsp_i2c_init()             - create the shared driver-NG
 *                                  I2C master bus on SDA=31/SCL=32.
 *                                  Handle is later handed to the
 *                                  Draftling touchscreen component
 *                                  via touchscreen_config_t.i2c_bus
 *                                  (set in main.cpp), the same
 *                                  pattern as on the LilyGO T5 EPD
 *                                  E-Paper boards.
 *   - bsp_display_brightness_init() - LEDC channel on GPIO22.
 *   - bsp_display_new_with_handles()- MIPI-DSI bus, IO, panel
 *                                  handles. esp_lcd_panel_init() is
 *                                  already called by the BSP; the
 *                                  panel is left OFF.
 *   - esp_lcd_panel_disp_on_off()  - turn the panel on.
 *
 * We deliberately do *not* use bsp_display_start() (which would
 * also instantiate esp_lvgl_port and create its own LVGL display);
 * Draftling owns LVGL itself in components/display/lvgl_port.cpp
 * and is the only LVGL consumer.
 *
 * DPI vs frame buffer
 * -------------------
 * On the ESP32-P4 MIPI-DSI controller the panel is scanned out
 * continuously from a framebuffer in PSRAM that the BSP creates
 * internally (CONFIG_BSP_LCD_DPI_BUFFER_NUMS=1 in our defaults).
 * esp_lcd_panel_draw_bitmap() copies our RGB565 tile into that
 * scanout buffer; the panel sees the change on the next vsync.
 * There is no explicit "flush" step required, so display_flush()
 * is a no-op here.
 *
 * Scaling
 * -------
 * Draftling renders LVGL 1:1 at the panel resolution (framebuffer
 * upscaling was removed; the Tab5 uses the larger Hack font instead).
 * MDSI_SCALE is fixed at 1, so the copy paths below are effectively a
 * straight blit; they are kept in their generic form in case a future
 * board needs expansion again.
 *
 * Rotation
 * --------
 * The panel is native portrait (720 wide x 1280 tall) but the
 * tablet is held landscape, so lvgl_port.cpp software-rotates each
 * LVGL tile 90 degrees before calling display_push_rgb565(); we
 * receive coordinates and a pixel buffer that are already in
 * panel-native (portrait) orientation, with x in [0, 720).
 */

#include <cstdio>
#include <cstring>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <esp_log.h>
#include <esp_heap_caps.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_mipi_dsi.h>

#include "bsp/display.h"
#include "bsp/m5stack_tab5.h"

#include "display.h"

static const char *TAG = "Display";

/* Panel renders 1:1; the former DRAFTLING_DISPLAY_SCALE upscale has
 * been removed (Tab5 uses a larger font instead). */
#define MDSI_SCALE   1

/* Upper bound on how long a single DMA2D frame-buffer copy may take
 * before we give up waiting for the color-trans-done callback. The
 * worst case (a full-screen 720x1280 RGB565 copy) completes in a few
 * milliseconds; 1 s is a generous safety net so a missed callback can
 * never wedge the LVGL task forever. */
#define MDSI_DRAW_TIMEOUT_MS 1000

static esp_lcd_panel_handle_t   s_panel = NULL;
static esp_lcd_panel_io_handle_t s_io    = NULL;
static int   s_width  = 0;   /* physical panel width  in pixels */
static int   s_height = 0;   /* physical panel height in pixels */

/* Binary semaphore signalled from the DPI panel's on_color_trans_done
 * callback once esp_lcd_panel_draw_bitmap() has finished copying our
 * buffer into the scanout framebuffer (the copy is performed
 * asynchronously by the DMA2D engine). dsi_blit() blocks on it after
 * every draw so we never issue a second draw_bitmap while the previous
 * DMA2D copy is still in flight -- the condition the ESP-LCD DSI driver
 * rejects with "previous draw operation is not finished" and which,
 * under the two-pane split refresh, used to back up until the LVGL task
 * spun and tripped the task watchdog. Keeping each blit synchronous
 * also means the source buffer (LVGL's draw buffer, our scaling scratch
 * buffer, or the clear scanline) is safe to reuse or free as soon as
 * dsi_blit() returns. */
static SemaphoreHandle_t s_draw_done = NULL;

static bool dsi_color_trans_done_cb(esp_lcd_panel_handle_t /*panel*/,
                                    esp_lcd_dpi_panel_event_data_t * /*edata*/,
                                    void * /*user_ctx*/)
{
    BaseType_t need_yield = pdFALSE;
    if (s_draw_done) {
        xSemaphoreGiveFromISR(s_draw_done, &need_yield);
    }
    return need_yield == pdTRUE;
}

/* Issue a draw_bitmap and block until the DMA2D copy completes. The
 * semaphore starts empty, so the take() below waits for the
 * on_color_trans_done callback that this draw will trigger. If the
 * semaphore has not been created yet (should not happen once
 * display_init() has run) the draw is issued without waiting. */
static void dsi_blit(int x1, int y1, int x2, int y2, const void *buf)
{
    esp_lcd_panel_draw_bitmap(s_panel, x1, y1, x2, y2, buf);
    if (s_draw_done) {
        if (xSemaphoreTake(s_draw_done, pdMS_TO_TICKS(MDSI_DRAW_TIMEOUT_MS)) != pdTRUE) {
            /* The draw-done callback never fired within the timeout.
             * Proceed anyway (the next draw may still recover), but log
             * it: a recurring warning here points at a stalled DMA2D /
             * DPI engine rather than a source-buffer reuse hazard. */
            ESP_LOGW(TAG, "draw_bitmap timeout (%d ms) waiting for DMA2D",
                     MDSI_DRAW_TIMEOUT_MS);
        }
    }
}

/* Scratch buffer used to nearest-neighbor expand a logical RGB565
 * tile into a SCALE x SCALE panel-pixel block. Grown on demand to
 * the largest tile we have ever been asked to push; lives in PSRAM
 * because internal SRAM is reserved for DMA descriptors on P4. */
static uint16_t *s_scratch    = NULL;
static size_t    s_scratch_px = 0;

static bool ensure_scratch(size_t px_needed)
{
    if (px_needed <= s_scratch_px) return true;
    if (s_scratch) heap_caps_free(s_scratch);
    s_scratch = (uint16_t *)heap_caps_malloc(px_needed * sizeof(uint16_t),
                                             MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_scratch) {
        s_scratch_px = 0;
        ESP_LOGE(TAG, "scratch alloc failed (%u px)", (unsigned)px_needed);
        return false;
    }
    s_scratch_px = px_needed;
    return true;
}

extern "C" void display_set_backlight(int percent)
{
    if (percent < 0)   percent = 0;
    if (percent > 100) percent = 100;
    esp_err_t err = bsp_display_brightness_set(percent);
    if (err != ESP_OK && err != ESP_ERR_NOT_SUPPORTED) {
        ESP_LOGW(TAG, "bsp_display_brightness_set(%d) failed: %s",
                 percent, esp_err_to_name(err));
    }
}

extern "C" void display_init(int /*pin_a*/, int /*pin_b*/, int /*pin_c*/,
                             int /*pin_d*/, int /*pin_e*/, int /*pin_f*/,
                             int width, int height)
{
    s_width  = width;
    s_height = height;

    /* The BSP owns the I2C master bus. If main.cpp has not called
     * bsp_i2c_init() yet (it does so right before display_init()
     * to share the handle with the touchscreen), call it here for
     * safety -- bsp_i2c_init() is idempotent and returns the same
     * handle on repeated calls. */
    esp_err_t err = bsp_i2c_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "bsp_i2c_init failed: %s", esp_err_to_name(err));
        return;
    }

    err = bsp_display_brightness_init();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "bsp_display_brightness_init failed: %s",
                 esp_err_to_name(err));
    }

    bsp_display_config_t cfg = {};
    cfg.dsi_bus.phy_clk_src        = MIPI_DSI_PHY_CLK_SRC_DEFAULT;
    cfg.dsi_bus.lane_bit_rate_mbps = BSP_LCD_MIPI_DSI_LANE_BITRATE_MBPS;

    bsp_lcd_handles_t handles = {};
    err = bsp_display_new_with_handles(&cfg, &handles);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "bsp_display_new_with_handles failed: %s",
                 esp_err_to_name(err));
        return;
    }
    s_panel = handles.panel;
    s_io    = handles.io;

    /* Create the draw-completion semaphore and register the DPI
     * color-trans-done callback before the first draw so dsi_blit()
     * can serialise draws against the DMA2D engine. The semaphore
     * starts empty (created "taken"): the first dsi_blit() issues its
     * draw and then waits for the callback this very draw raises. */
    if (!s_draw_done) {
        s_draw_done = xSemaphoreCreateBinary();
    }
    if (s_draw_done) {
        esp_lcd_dpi_panel_event_callbacks_t cbs = {};
        cbs.on_color_trans_done = dsi_color_trans_done_cb;
        esp_err_t cb_err = esp_lcd_dpi_panel_register_event_callbacks(
                               s_panel, &cbs, NULL);
        if (cb_err != ESP_OK) {
            ESP_LOGW(TAG, "register draw-done callback failed: %s; "
                          "freeing semaphore and falling back to "
                          "unsynchronised draws", esp_err_to_name(cb_err));
            vSemaphoreDelete(s_draw_done);
            s_draw_done = NULL;
        }
    }

    err = esp_lcd_panel_disp_on_off(s_panel, true);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "esp_lcd_panel_disp_on_off(true) failed: %s",
                 esp_err_to_name(err));
    }

    /* Start with the backlight off; lvgl_port + the user's saved
     * brightness setting will turn it on once the first frame is
     * rendered, avoiding a brief flash of an uninitialised
     * framebuffer at boot. */
    bsp_display_backlight_off();

    ESP_LOGI(TAG, "MIPI-DSI panel ready (%dx%d, scale=%d)",
             s_width, s_height, MDSI_SCALE);
}

extern "C" void display_clear(uint8_t color)
{
    if (!s_panel) return;
    /* Fill the entire panel scanout buffer with a single color.
     * 0xFF -> white, anything else -> black, matching the legacy
     * 1-bpp clear semantics used by the e-paper backends. */
    uint16_t fill = color ? 0xFFFF : 0x0000;
    /* Allocate a single scanline of fill pixels and stream it row
     * by row to avoid holding a full-frame staging buffer. */
    uint16_t *row = (uint16_t *)heap_caps_malloc((size_t)s_width * sizeof(uint16_t),
                                                 MALLOC_CAP_SPIRAM);
    if (!row) return;
    for (int x = 0; x < s_width; x++) row[x] = fill;
    for (int y = 0; y < s_height; y++) {
        dsi_blit(0, y, s_width, y + 1, row);
    }
    heap_caps_free(row);
}

extern "C" void display_set_pixel(uint16_t /*x*/, uint16_t /*y*/, uint8_t /*color*/)
{
    /* Color backend: per-pixel setting is unused (lvgl_port always
     * pushes RGB565 tiles via display_push_rgb565). */
}

extern "C" bool display_push_rgb565(int x, int y, int w, int h,
                                    const void *color_map)
{
    if (!s_panel)            return true;
    if (w <= 0 || h <= 0)    return true;

    if (MDSI_SCALE <= 1) {
        /* Fast path: 1:1, push the LVGL buffer straight into the
         * panel scanout framebuffer. */
        int x2 = x + w;
        int y2 = y + h;
        if (x2 > s_width)  x2 = s_width;
        if (y2 > s_height) y2 = s_height;
        if (x2 <= x || y2 <= y) return true;
        dsi_blit(x, y, x2, y2, color_map);
        return true;
    }

    /* Scaled path: nearest-neighbor expand (w x h) logical pixels
     * into (w*SCALE x h*SCALE) panel pixels in s_scratch, then push
     * that as a single rectangle. */
    int px = x * MDSI_SCALE;
    int py = y * MDSI_SCALE;
    int pw = w * MDSI_SCALE;
    int ph = h * MDSI_SCALE;
    if (px >= s_width || py >= s_height) return true;
    if (px + pw > s_width)  pw = s_width  - px;
    if (py + ph > s_height) ph = s_height - py;
    if (pw <= 0 || ph <= 0) return true;

    if (!ensure_scratch((size_t)pw * (size_t)ph)) return true;

    /* color_map is a tightly-packed RGB565 LVGL framebuffer; LVGL
     * v9 hands these out 2-byte aligned (lv_display_set_buffers
     * pads to a multiple of LV_DRAW_BUF_ALIGN, default 4). */
    const uint16_t *src = (const uint16_t *)color_map;
    /* For each source row, write SCALE destination rows; for each
     * source pixel, write SCALE destination pixels horizontally. */
    for (int sy = 0; sy < h; sy++) {
        const uint16_t *src_row = src + (size_t)sy * w;
        /* Build the first expanded destination row for this source
         * row, then memcpy it SCALE-1 more times. */
        uint16_t *dst_row0 = s_scratch + (size_t)sy * MDSI_SCALE * pw;
        int dx = 0;
        for (int sx = 0; sx < w; sx++) {
            uint16_t v = src_row[sx];
            int copies = MDSI_SCALE;
            if (dx + copies > pw) copies = pw - dx;
            if (copies <= 0) break;
            for (int k = 0; k < copies; k++) dst_row0[dx + k] = v;
            dx += copies;
        }
        for (int r = 1; r < MDSI_SCALE; r++) {
            if (sy * MDSI_SCALE + r >= ph) break;
            uint16_t *dst_row = s_scratch + ((size_t)sy * MDSI_SCALE + r) * pw;
            memcpy(dst_row, dst_row0, (size_t)pw * sizeof(uint16_t));
        }
    }
    dsi_blit(px, py, px + pw, py + ph, s_scratch);
    return true;
}

extern "C" void display_set_partial_clip(int /*x*/, int /*y*/,
                                         int /*w*/, int /*h*/)
{
    /* DPI panel updates the entire scanout buffer continuously; the
     * dirty bbox optimisation that e-paper backends use is not
     * meaningful here. */
}

extern "C" void display_flush(void)
{
    /* No-op: MIPI-DSI DPI panel scans the framebuffer out
     * continuously; esp_lcd_panel_draw_bitmap() in
     * display_push_rgb565() already published the change. */
}

extern "C" void display_full_refresh(void)
{
    /* No-op (no e-paper waveforms to clear). */
}

extern "C" void display_request_full_refresh(void)
{
    /* No-op: continuous scanout, nothing to latch. */
}

extern "C" uint8_t *display_get_buffer(void)
{
    /* The legacy 1-bpp framebuffer is not exposed by this backend;
     * Draftling code paths that touch it are e-paper-only. */
    return NULL;
}

extern "C" int display_get_buffer_size(void)
{
    return 0;
}

extern "C" void display_sleep(void)
{
    bsp_display_backlight_off();
    if (s_panel) esp_lcd_panel_disp_on_off(s_panel, false);
}

extern "C" void display_wake(void)
{
    if (s_panel) esp_lcd_panel_disp_on_off(s_panel, true);
    bsp_display_backlight_on();
}

extern "C" void display_deep_sleep_prepare(void)
{
    bsp_display_backlight_off();
    if (s_panel) esp_lcd_panel_disp_on_off(s_panel, false);
}

extern "C" void display_set_shared_i2c_bus(void * /*bus_handle*/)
{
    /* The MIPI-DSI backend does not consume an externally created
     * I2C bus -- the BSP creates and owns the I2C bus itself via
     * bsp_i2c_init(). main.cpp instead reads the bus back out via
     * bsp_i2c_get_handle() and hands it to the touchscreen
     * component (touchscreen_config_t.i2c_bus). */
}

#endif /* CONFIG_DRAFTLING_DISPLAY_MIPI_DSI */
