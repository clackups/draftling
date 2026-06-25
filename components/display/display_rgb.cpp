#include "sdkconfig.h"
#if defined(CONFIG_DRAFTLING_DISPLAY_RGB)

/*
 * Parallel RGB565 color-LCD driver (ESP32-S3 LCD RGB peripheral).
 *
 * Used by:
 *   - Sunton ESP32-8048S070C (7", 800x480, 16-bit parallel RGB)
 *   - Sunton ESP32-8048S043C (4.3", 800x480, ST7262, 16-bit RGB)
 *
 * The ESP32-S3 LCD peripheral drives a "dumb" RGB TFT directly:
 * a continuously-scanned-out framebuffer in PSRAM is shifted out
 * pixel-by-pixel on the 16 data lines under the HSYNC / VSYNC / DE /
 * PCLK timing programmed below. esp_lcd_new_rgb_panel() owns all the
 * panel GPIOs; esp_lcd_panel_draw_bitmap() copies a rectangle into
 * the scan-out framebuffer.
 *
 * Architecture
 * ------------
 * This backend keeps its own RGB565 framebuffer in PSRAM (sized to
 * width * height * 2). The LVGL port's flush_cb pushes RGB565
 * rectangles into it via display_push_rgb565() (with SCALE x SCALE
 * nearest-neighbor expansion of each logical pixel), accumulating a
 * dirty bounding box; display_flush() copies just that region to the
 * panel via esp_lcd_panel_draw_bitmap(). A degraded per-pixel
 * fallback (display_set_pixel, interpreting 0/0xFF as black/white) is
 * provided for the splash-screen logo path in editor_ui.cpp.
 *
 * Per-board pin map / timings are selected at build time on
 * CONFIG_DRAFTLING_RGB_BOARD_S043 (set for the 4.3" ESP32-8048S043C;
 * unset for the default 7" ESP32-8048S070C). The two boards share the
 * 800x480 resolution but differ in their control-pin map (VSYNC/DE
 * swapped), panel timings (12.5 MHz pclk_active_neg vs 12 MHz
 * pclk_idle_high) and data-line order (R,G,B vs B,G,R). The data
 * GPIOs are always listed in the order required by the panel so a
 * standard RGB565 pixel lands on the correct color lines.
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_heap_caps.h>
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_rgb.h>

#include "display.h"

static const char *TAG = "DisplayRGB";

/* ---- Panel pin map + timings (per board) ----
 *
 * Backlight is GPIO2 on both Sunton boards. The control pins, panel
 * timings and data-line order differ between the 7" ESP32-8048S070C
 * (default) and the 4.3" ESP32-8048S043C
 * (CONFIG_DRAFTLING_RGB_BOARD_S043). esp_lcd_new_rgb_panel maps
 * data_gpio_nums[0] to the least-significant bit of the 16-bit RGB565
 * word and [15] to the most-significant; RGB565 packs as
 * R[15:11] G[10:5] B[4:0]. Each board's data array is listed in the
 * order its panel wiring requires so a standard RGB565 pixel lands on
 * the correct color lines. */
#define RGB_DISP_GPIO       -1
#define RGB_BL_GPIO         2

#if defined(CONFIG_DRAFTLING_RGB_BOARD_S043)
/* ---- Sunton ESP32-8048S043C (4.3", ST7262) ---- */
#define RGB_PCLK_HZ         (12500 * 1000)   /* 12.5 MHz */
#define RGB_HSYNC_GPIO      39
#define RGB_VSYNC_GPIO      41
#define RGB_DE_GPIO         40
#define RGB_PCLK_GPIO       42
#define RGB_HSYNC_PULSE     4
#define RGB_HSYNC_BACK      8
#define RGB_HSYNC_FRONT     8
#define RGB_VSYNC_PULSE     4
#define RGB_VSYNC_BACK      8
#define RGB_VSYNC_FRONT     8
#define RGB_PCLK_ACTIVE_NEG 1   /* ST7262 panel needs active-low PCLK */
#define RGB_PCLK_IDLE_HIGH  0

/* Data lines in R,G,B order (the 4.3" panel's required wiring).
 * R0-R4 -> bits 0..4, G0-G5 -> 5..10, B0-B4 -> 11..15. */
static const int kDataGpios[16] = {
    8, 3, 46, 9, 1,       /* R0-R4 */
    5, 6, 7, 15, 16, 4,   /* G0-G5 */
    45, 48, 47, 21, 14    /* B0-B4 */
};

#else
/* ---- Sunton ESP32-8048S070C (7", default) ---- */
#define RGB_PCLK_HZ         (12 * 1000 * 1000)   /* 12 MHz */
#define RGB_HSYNC_GPIO      39
#define RGB_VSYNC_GPIO      40
#define RGB_DE_GPIO         41
#define RGB_PCLK_GPIO       42
#define RGB_HSYNC_PULSE     2
#define RGB_HSYNC_BACK      43
#define RGB_HSYNC_FRONT     8
#define RGB_VSYNC_PULSE     2
#define RGB_VSYNC_BACK      12
#define RGB_VSYNC_FRONT     8
#define RGB_PCLK_ACTIVE_NEG 0
#define RGB_PCLK_IDLE_HIGH  1

/* Data lines in B,G,R order. The verified Sunton 7" wiring lists the
 * physical pins in R,G,B order; we place them in B,G,R order here so
 * a standard RGB565 pixel lands on the correct color lines. With the
 * R,G,B order red and blue came out swapped (e.g. "orange on black"
 * rendered as blue on black).
 * B0-B4 -> bits 0..4, G0-G5 -> 5..10, R0-R4 -> 11..15. */
static const int kDataGpios[16] = {
    15, 7, 6, 5, 4,       /* B0-B4 */
    9, 46, 3, 8, 16, 1,   /* G0-G5 */
    14, 21, 47, 48, 45    /* R0-R4 */
};
#endif

/* ---- Backlight LEDC ---- */
#define BL_LEDC_TIMER       LEDC_TIMER_0
#define BL_LEDC_MODE        LEDC_LOW_SPEED_MODE
#define BL_LEDC_CHANNEL     LEDC_CHANNEL_0
#define BL_LEDC_DUTY_RES    LEDC_TIMER_8_BIT
#define BL_LEDC_DUTY_MAX    ((1 << 8) - 1)
#define BL_LEDC_FREQ_HZ     1000

/* Logical-to-panel pixel scale (Kconfig). Each logical LVGL pixel is
 * rendered as SCALE x SCALE physical panel pixels. */
#ifdef CONFIG_DRAFTLING_DISPLAY_SCALE
#define RGB_SCALE CONFIG_DRAFTLING_DISPLAY_SCALE
#else
#define RGB_SCALE 1
#endif

static esp_lcd_panel_handle_t s_panel = NULL;
static int s_width  = 0;
static int s_height = 0;

/* Host-side RGB565 framebuffer (PSRAM). */
static uint16_t *s_fb = NULL;
static size_t    s_fb_pixels = 0;

/* Accumulated dirty bounding box (inclusive); (-1,...) means clean. */
static int s_dirty_x1 = -1;
static int s_dirty_y1 = -1;
static int s_dirty_x2 = -1;
static int s_dirty_y2 = -1;

static int  s_bl_pin = -1;
static int  s_bl_last_pct = 100;

static void backlight_pwm_init(int bl_pin)
{
    if (bl_pin < 0) return;

    ledc_timer_config_t t = {};
    t.speed_mode      = BL_LEDC_MODE;
    t.duty_resolution = BL_LEDC_DUTY_RES;
    t.timer_num       = BL_LEDC_TIMER;
    t.freq_hz         = BL_LEDC_FREQ_HZ;
    t.clk_cfg         = LEDC_AUTO_CLK;
    ESP_ERROR_CHECK(ledc_timer_config(&t));

    ledc_channel_config_t c = {};
    c.gpio_num   = bl_pin;
    c.speed_mode = BL_LEDC_MODE;
    c.channel    = BL_LEDC_CHANNEL;
    c.timer_sel  = BL_LEDC_TIMER;
    c.intr_type  = LEDC_INTR_DISABLE;
    c.duty       = BL_LEDC_DUTY_MAX;  /* full brightness at boot */
    c.hpoint     = 0;
    ESP_ERROR_CHECK(ledc_channel_config(&c));
}

extern "C" void display_set_backlight(int percent)
{
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    if (s_bl_pin < 0) return;
    s_bl_last_pct = percent;
    uint32_t duty = (uint32_t)((BL_LEDC_DUTY_MAX * percent) / 100);
    ESP_ERROR_CHECK(ledc_set_duty(BL_LEDC_MODE, BL_LEDC_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(BL_LEDC_MODE, BL_LEDC_CHANNEL));
}

/* ---------------- Public API ---------------- */

extern "C" void display_init(int /*pin_a*/, int /*pin_b*/, int /*pin_c*/,
                             int /*pin_d*/, int /*pin_e*/, int /*pin_f*/,
                             int width, int height)
{
    s_width  = width;
    s_height = height;

    s_bl_pin = RGB_BL_GPIO;
    backlight_pwm_init(s_bl_pin);

    esp_lcd_rgb_panel_config_t cfg = {};
    cfg.clk_src   = LCD_CLK_SRC_DEFAULT;
    cfg.timings.pclk_hz           = RGB_PCLK_HZ;
    cfg.timings.h_res             = s_width;
    cfg.timings.v_res             = s_height;
    cfg.timings.hsync_pulse_width = RGB_HSYNC_PULSE;
    cfg.timings.hsync_back_porch  = RGB_HSYNC_BACK;
    cfg.timings.hsync_front_porch = RGB_HSYNC_FRONT;
    cfg.timings.vsync_pulse_width = RGB_VSYNC_PULSE;
    cfg.timings.vsync_back_porch  = RGB_VSYNC_BACK;
    cfg.timings.vsync_front_porch = RGB_VSYNC_FRONT;
    cfg.timings.flags.pclk_active_neg = RGB_PCLK_ACTIVE_NEG;
    cfg.timings.flags.pclk_idle_high  = RGB_PCLK_IDLE_HIGH;
    cfg.data_width   = 16;
    cfg.bits_per_pixel = 16;
    cfg.num_fbs      = 1;
    cfg.bounce_buffer_size_px = s_width * 16;  /* 16 lines */
    cfg.flags.fb_in_psram = 1;
    cfg.hsync_gpio_num = RGB_HSYNC_GPIO;
    cfg.vsync_gpio_num = RGB_VSYNC_GPIO;
    cfg.de_gpio_num    = RGB_DE_GPIO;
    cfg.pclk_gpio_num  = RGB_PCLK_GPIO;
    cfg.disp_gpio_num  = RGB_DISP_GPIO;
    for (int i = 0; i < 16; i++) cfg.data_gpio_nums[i] = kDataGpios[i];

    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&cfg, &s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel));

    /* Host framebuffer (RGB565) in PSRAM. */
    s_fb_pixels = (size_t)s_width * s_height;
    s_fb = (uint16_t *)heap_caps_malloc(s_fb_pixels * sizeof(uint16_t),
                                        MALLOC_CAP_SPIRAM);
    assert(s_fb);
    memset(s_fb, 0, s_fb_pixels * sizeof(uint16_t));

    display_clear(0x00);
    display_full_refresh();

    ESP_LOGI(TAG, "RGB panel %dx%d initialized", s_width, s_height);
}

extern "C" void display_clear(uint8_t color)
{
    /* 1-bpp legacy API: 0xFF = white, 0x00 = black. */
    memset(s_fb, color ? 0xFF : 0x00, s_fb_pixels * sizeof(uint16_t));
    s_dirty_x1 = 0;
    s_dirty_y1 = 0;
    s_dirty_x2 = s_width  - 1;
    s_dirty_y2 = s_height - 1;
}

extern "C" void display_set_pixel(uint16_t x, uint16_t y, uint8_t color)
{
    int px = (int)x * RGB_SCALE;
    int py = (int)y * RGB_SCALE;
    if (px >= s_width || py >= s_height) return;
    uint16_t v = (color == 0) ? 0x0000 : 0xFFFF;
    int x_end = px + RGB_SCALE; if (x_end > s_width)  x_end = s_width;
    int y_end = py + RGB_SCALE; if (y_end > s_height) y_end = s_height;
    for (int yy = py; yy < y_end; yy++) {
        uint16_t *row = s_fb + (size_t)yy * s_width + px;
        for (int xx = px; xx < x_end; xx++) *row++ = v;
    }
    int x2 = x_end - 1;
    int y2 = y_end - 1;
    if (s_dirty_x1 < 0) {
        s_dirty_x1 = px;  s_dirty_y1 = py;
        s_dirty_x2 = x2;  s_dirty_y2 = y2;
    } else {
        if (px < s_dirty_x1) s_dirty_x1 = px;
        if (x2 > s_dirty_x2) s_dirty_x2 = x2;
        if (py < s_dirty_y1) s_dirty_y1 = py;
        if (y2 > s_dirty_y2) s_dirty_y2 = y2;
    }
}

extern "C" bool display_push_rgb565(int x, int y, int w, int h,
                                    const void *color_map)
{
    if (w <= 0 || h <= 0) return true;
    int px = x * RGB_SCALE;
    int py = y * RGB_SCALE;
    int pw = w * RGB_SCALE;
    int ph = h * RGB_SCALE;
    if (px < 0 || py < 0) return true;
    int x2 = px + pw - 1;
    int y2 = py + ph - 1;
    if (x2 >= s_width)  x2 = s_width  - 1;
    if (y2 >= s_height) y2 = s_height - 1;
    int eff_w = x2 - px + 1;
    int eff_h = y2 - py + 1;
    if (eff_w <= 0 || eff_h <= 0) return true;

    const uint16_t *src = (const uint16_t *)color_map;
    if (RGB_SCALE == 1) {
        for (int row = 0; row < eff_h; row++) {
            uint16_t *dst = s_fb + (size_t)(py + row) * s_width + px;
            memcpy(dst, src, eff_w * sizeof(uint16_t));
            src += w;
        }
    } else {
        for (int sy = 0; sy < h; sy++) {
            int dy0 = py + sy * RGB_SCALE;
            if (dy0 >= s_height) break;
            uint16_t *drow0 = s_fb + (size_t)dy0 * s_width + px;
            uint16_t *p = drow0;
            int written = 0;
            for (int sx = 0; sx < w && written < eff_w; sx++) {
                uint16_t v = src[(size_t)sy * w + sx];
                for (int k = 0; k < RGB_SCALE && written < eff_w; k++) {
                    *p++ = v;
                    written++;
                }
            }
            for (int k = 1; k < RGB_SCALE; k++) {
                int dy = dy0 + k;
                if (dy >= s_height) break;
                memcpy(s_fb + (size_t)dy * s_width + px, drow0,
                       (size_t)eff_w * sizeof(uint16_t));
            }
        }
    }

    if (s_dirty_x1 < 0) {
        s_dirty_x1 = px;  s_dirty_y1 = py;
        s_dirty_x2 = x2;  s_dirty_y2 = y2;
    } else {
        if (px < s_dirty_x1) s_dirty_x1 = px;
        if (x2 > s_dirty_x2) s_dirty_x2 = x2;
        if (py < s_dirty_y1) s_dirty_y1 = py;
        if (y2 > s_dirty_y2) s_dirty_y2 = y2;
    }
    return true;
}

extern "C" void display_set_partial_clip(int /*x*/, int /*y*/,
                                         int /*w*/, int /*h*/)
{
    /* No-op: the RGB backend always pushes the full dirty bbox. */
}

extern "C" void display_flush(void)
{
    if (s_dirty_x1 < 0) return;

    int x1 = s_dirty_x1, y1 = s_dirty_y1;
    int x2 = s_dirty_x2, y2 = s_dirty_y2;
    s_dirty_x1 = s_dirty_y1 = s_dirty_x2 = s_dirty_y2 = -1;

    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 >= s_width)  x2 = s_width  - 1;
    if (y2 >= s_height) y2 = s_height - 1;
    if (x1 > x2 || y1 > y2) return;

    /* esp_lcd_panel_draw_bitmap reads a tightly-packed (w x h)
     * rectangle. Our framebuffer rows are s_width wide, so stream
     * the rectangle row-by-row through a small scratch buffer. */
    int w = x2 - x1 + 1;
    int h = y2 - y1 + 1;

    if (x1 == 0 && w == s_width) {
        /* Full-width rectangle: rows are already contiguous in the
         * framebuffer, so one draw_bitmap call covers it. */
        esp_lcd_panel_draw_bitmap(s_panel, x1, y1, x2 + 1, y2 + 1,
                                  s_fb + (size_t)y1 * s_width);
        return;
    }

    /* Partial-width rectangle: copy into a contiguous scratch buffer. */
    uint16_t *scratch = (uint16_t *)heap_caps_malloc(
        (size_t)w * h * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    if (!scratch) {
        /* Fall back to a full-frame push if the scratch alloc fails. */
        esp_lcd_panel_draw_bitmap(s_panel, 0, 0, s_width, s_height, s_fb);
        return;
    }
    for (int row = 0; row < h; row++) {
        memcpy(scratch + (size_t)row * w,
               s_fb + (size_t)(y1 + row) * s_width + x1,
               (size_t)w * sizeof(uint16_t));
    }
    esp_lcd_panel_draw_bitmap(s_panel, x1, y1, x2 + 1, y2 + 1, scratch);
    heap_caps_free(scratch);
}

extern "C" void display_full_refresh(void)
{
    s_dirty_x1 = 0;
    s_dirty_y1 = 0;
    s_dirty_x2 = s_width  - 1;
    s_dirty_y2 = s_height - 1;
    display_flush();
}

extern "C" void display_request_full_refresh(void)
{
    s_dirty_x1 = 0;
    s_dirty_y1 = 0;
    s_dirty_x2 = s_width  - 1;
    s_dirty_y2 = s_height - 1;
}

extern "C" uint8_t *display_get_buffer(void)
{
    return (uint8_t *)s_fb;
}

extern "C" int display_get_buffer_size(void)
{
    return (int)(s_fb_pixels * sizeof(uint16_t));
}

extern "C" void display_sleep(void)
{
    if (s_bl_pin >= 0) display_set_backlight(0);
}

extern "C" void display_wake(void)
{
    if (s_bl_pin >= 0) display_set_backlight(s_bl_last_pct);
    display_full_refresh();
}

extern "C" void display_deep_sleep_prepare(void)
{
    if (s_bl_pin >= 0) {
        ledc_set_duty(BL_LEDC_MODE, BL_LEDC_CHANNEL, 0);
        ledc_update_duty(BL_LEDC_MODE, BL_LEDC_CHANNEL);
    }
}

extern "C" void display_set_shared_i2c_bus(void * /*bus_handle*/)
{
    /* No-op: the RGB backend does not use I2C. */
}

#endif /* CONFIG_DRAFTLING_DISPLAY_RGB */
