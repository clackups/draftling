#include "sdkconfig.h"
#if defined(CONFIG_DRAFTLING_DISPLAY_H752_EPD)

#include <algorithm>
#include <cstring>

#include <driver/gpio.h>
#include <driver/ledc.h>
#include <esp_heap_caps.h>
#include <esp_log.h>

#include <FastEPD.h>

#include "display.h"

static const char *TAG = "DisplayH752";

static constexpr int PANEL_WIDTH = 960;
static constexpr int PANEL_HEIGHT = 540;
static constexpr int FRAMEBUFFER_BYTES = PANEL_WIDTH * PANEL_HEIGHT / 8;

#ifdef CONFIG_DRAFTLING_DISPLAY_SCALE
#define H752_SCALE CONFIG_DRAFTLING_DISPLAY_SCALE
#else
#define H752_SCALE 1
#endif

#ifdef CONFIG_DRAFTLING_EPD_FULL_REFRESH_INTERVAL
#define H752_FULL_REFRESH_INTERVAL CONFIG_DRAFTLING_EPD_FULL_REFRESH_INTERVAL
#else
#define H752_FULL_REFRESH_INTERVAL 30
#endif

#define H752_BL_LEDC_TIMER       LEDC_TIMER_0
#define H752_BL_LEDC_MODE        LEDC_LOW_SPEED_MODE
#define H752_BL_LEDC_CHANNEL     LEDC_CHANNEL_0
#define H752_BL_LEDC_DUTY_RES    LEDC_TIMER_10_BIT
#define H752_BL_LEDC_DUTY_MAX    ((1 << 10) - 1)
#define H752_BL_LEDC_FREQ_HZ     5000
#define H752_BL_PIN              40

static uint8_t *s_fb = nullptr;
static FASTEPD *s_epd = nullptr;
static bool s_initialized = false;
static bool s_bl_inited = false;
static bool s_force_full = true;
static int s_partial_count = 0;
static int s_dirty_x0 = -1;
static int s_dirty_y0 = -1;
static int s_dirty_x1 = -1;
static int s_dirty_y1 = -1;
static int s_clip_x0 = -1;
static int s_clip_y0 = -1;
static int s_clip_x1 = -1;
static int s_clip_y1 = -1;
static int s_width = PANEL_WIDTH;
static int s_height = PANEL_HEIGHT;

static inline void clear_dirty(void)
{
    s_dirty_x0 = s_dirty_y0 = s_dirty_x1 = s_dirty_y1 = -1;
}

static inline void mark_dirty_rect(int x, int y, int w, int h)
{
    if (w <= 0 || h <= 0) return;
    int x0 = std::max(0, x);
    int y0 = std::max(0, y);
    int x1 = std::min(s_width,  x + w);
    int y1 = std::min(s_height, y + h);
    if (x1 <= x0 || y1 <= y0) return;
    if (s_dirty_x0 < 0) {
        s_dirty_x0 = x0;
        s_dirty_y0 = y0;
        s_dirty_x1 = x1 - 1;
        s_dirty_y1 = y1 - 1;
        return;
    }
    s_dirty_x0 = std::min(s_dirty_x0, x0);
    s_dirty_y0 = std::min(s_dirty_y0, y0);
    s_dirty_x1 = std::max(s_dirty_x1, x1 - 1);
    s_dirty_y1 = std::max(s_dirty_y1, y1 - 1);
}

static inline void set_panel_pixel(int x, int y, bool black)
{
    if ((unsigned)x >= (unsigned)s_width || (unsigned)y >= (unsigned)s_height) {
        return;
    }
    size_t idx = (size_t)y * (s_width / 8) + (size_t)(x >> 3);
    uint8_t mask = (uint8_t)(0x80U >> (x & 7));
    if (black) {
        s_fb[idx] &= (uint8_t)~mask;
    } else {
        s_fb[idx] |= mask;
    }
}

static void fill_panel_rect(int x, int y, int w, int h, bool black)
{
    if (!s_fb || w <= 0 || h <= 0) return;
    int x0 = std::max(0, x);
    int y0 = std::max(0, y);
    int x1 = std::min(s_width, x + w);
    int y1 = std::min(s_height, y + h);
    if (x1 <= x0 || y1 <= y0) return;

    if (x0 == 0 && x1 == s_width) {
        const size_t row_bytes = (size_t)s_width / 8;
        for (int py = y0; py < y1; ++py) {
            memset(s_fb + (size_t)py * row_bytes, black ? 0x00 : 0xFF, row_bytes);
        }
        return;
    }

    for (int py = y0; py < y1; ++py) {
        for (int px = x0; px < x1; ++px) {
            set_panel_pixel(px, py, black);
        }
    }
}

static inline bool rgb565_is_black(uint16_t v)
{
    /* Green is the highest-resolution RGB565 channel and a good enough
     * threshold for Draftling's mostly black/white LVGL output. */
    return ((v >> 5) & 0x3F) < 32;
}

static void backlight_pwm_init(void)
{
    if (s_bl_inited) return;
    ledc_timer_config_t t = {};
    t.speed_mode = H752_BL_LEDC_MODE;
    t.duty_resolution = H752_BL_LEDC_DUTY_RES;
    t.timer_num = H752_BL_LEDC_TIMER;
    t.freq_hz = H752_BL_LEDC_FREQ_HZ;
    t.clk_cfg = LEDC_AUTO_CLK;
    ESP_ERROR_CHECK(ledc_timer_config(&t));

    gpio_hold_dis((gpio_num_t)H752_BL_PIN);
    ledc_channel_config_t c = {};
    c.gpio_num = H752_BL_PIN;
    c.speed_mode = H752_BL_LEDC_MODE;
    c.channel = H752_BL_LEDC_CHANNEL;
    c.timer_sel = H752_BL_LEDC_TIMER;
    c.intr_type = LEDC_INTR_DISABLE;
    c.duty = 0;
    c.hpoint = 0;
    ESP_ERROR_CHECK(ledc_channel_config(&c));
    s_bl_inited = true;
}

static bool ensure_epd(void)
{
    if (s_epd) return true;

    s_epd = new FASTEPD();
    if (!s_epd) {
        ESP_LOGE(TAG, "Failed to allocate FastEPD backend");
        return false;
    }

    int rc = s_epd->initPanel(BB_PANEL_LILYGO_T5PRO, 28000000);
    if (rc != BBEP_SUCCESS) {
        ESP_LOGE(TAG, "FastEPD initPanel failed: %d", rc);
        delete s_epd;
        s_epd = nullptr;
        return false;
    }

    s_epd->setMode(BB_MODE_1BPP);
    if (!s_epd->currentBuffer()) {
        ESP_LOGE(TAG, "FastEPD did not allocate a current buffer");
        s_epd->deInit();
        delete s_epd;
        s_epd = nullptr;
        return false;
    }

    return true;
}

extern "C" void display_set_shared_i2c_bus(void *bus_handle)
{
    (void)bus_handle;
}

extern "C" void display_init(int, int, int, int, int, int, int width, int height)
{
    if (s_initialized) return;

    s_fb = (uint8_t *)heap_caps_malloc(FRAMEBUFFER_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_fb) {
        s_fb = (uint8_t *)heap_caps_malloc(FRAMEBUFFER_BYTES, MALLOC_CAP_8BIT);
    }
    if (!s_fb) {
        ESP_LOGE(TAG, "Failed to allocate %d-byte framebuffer", FRAMEBUFFER_BYTES);
        return;
    }

    if (!ensure_epd()) {
        heap_caps_free(s_fb);
        s_fb = nullptr;
        return;
    }

    s_width = PANEL_WIDTH;
    s_height = PANEL_HEIGHT;
    if (width != s_width || height != s_height) {
        ESP_LOGW(TAG, "Configured size %dx%d differs from H752 panel %dx%d",
                 width, height, s_width, s_height);
    }

    memset(s_fb, 0xFF, FRAMEBUFFER_BYTES);
    s_epd->setMode(BB_MODE_1BPP);
    s_epd->fillScreen(BBEP_WHITE);
    s_epd->backupPlane();

    backlight_pwm_init();
    s_force_full = true;
    clear_dirty();
    s_initialized = true;

    ESP_LOGI(TAG, "H752 EPD47 display initialized (%dx%d panel, scale=%d -> %dx%d logical)",
             s_width, s_height, H752_SCALE, s_width / H752_SCALE, s_height / H752_SCALE);
}

extern "C" void display_clear(uint8_t color)
{
    if (!s_initialized || !s_fb) return;
    memset(s_fb, color ? 0xFF : 0x00, FRAMEBUFFER_BYTES);
    mark_dirty_rect(0, 0, s_width, s_height);
    s_force_full = true;
}

extern "C" void display_set_pixel(uint16_t x, uint16_t y, uint8_t color)
{
    if (!s_initialized || !s_fb) return;
    int px = (int)x * H752_SCALE;
    int py = (int)y * H752_SCALE;
    fill_panel_rect(px, py, H752_SCALE, H752_SCALE, color == 0);
    mark_dirty_rect(px, py, H752_SCALE, H752_SCALE);
}

extern "C" bool display_push_rgb565(int x, int y, int w, int h, const void *color_map)
{
    if (!s_initialized || !s_fb || !color_map || w <= 0 || h <= 0) return false;
    const uint16_t *src = (const uint16_t *)color_map;
    for (int sy = 0; sy < h; ++sy) {
        for (int sx = 0; sx < w; ++sx) {
            bool black = rgb565_is_black(src[(size_t)sy * w + sx]);
            fill_panel_rect((x + sx) * H752_SCALE, (y + sy) * H752_SCALE,
                            H752_SCALE, H752_SCALE, black);
        }
    }
    mark_dirty_rect(x * H752_SCALE, y * H752_SCALE,
                    w * H752_SCALE, h * H752_SCALE);
    return true;
}

extern "C" void display_flush(void)
{
    if (!s_initialized || !s_fb) return;
    if (!ensure_epd()) return;
    if (s_dirty_x0 < 0) return;

    int x0 = s_dirty_x0;
    int y0 = s_dirty_y0;
    int x1 = s_dirty_x1;
    int y1 = s_dirty_y1;
    s_dirty_x0 = s_dirty_y0 = s_dirty_x1 = s_dirty_y1 = -1;

    /* s_clip_* use -1 as the "unset" sentinel; require a non-negative
     * origin so an unset clip is not applied (-1 >= -1 is true and
     * would clamp every flush to an empty rect). */
    if (s_clip_x0 >= 0 && s_clip_y0 >= 0 &&
        s_clip_x1 >= s_clip_x0 && s_clip_y1 >= s_clip_y0) {
        x0 = std::max(x0, s_clip_x0);
        y0 = std::max(y0, s_clip_y0);
        x1 = std::min(x1, s_clip_x1);
        y1 = std::min(y1, s_clip_y1);
        s_clip_x0 = s_clip_y0 = s_clip_x1 = s_clip_y1 = -1;
    }

    if (x1 < x0 || y1 < y0) {
        s_force_full = false;
        s_clip_x0 = s_clip_y0 = s_clip_x1 = s_clip_y1 = -1;
        return;
    }

    s_epd->setMode(BB_MODE_1BPP);
    uint8_t *target = s_epd->currentBuffer();
    if (!target) {
        ESP_LOGE(TAG, "FastEPD current buffer unavailable");
        return;
    }
    memcpy(target, s_fb, FRAMEBUFFER_BYTES);

    /* Small changes (typing, cursor, deletions) go through FastEPD's
     * partialUpdate: it diffs the current plane against the previous
     * plane and drives only the changed pixels in the given row range,
     * in both directions (white->black and black->white), with no
     * full-screen flash. Promote to a flashing fullUpdate when one was
     * explicitly requested, when the dirty area covers most of the
     * screen (a giant partial is slower and dirtier than a full), or
     * every H752_FULL_REFRESH_INTERVAL partials to clear the residual
     * ghosting partial waveforms accumulate. */
    long dirty_area = (long)(x1 - x0 + 1) * (y1 - y0 + 1);
    bool huge = dirty_area * 4 > (long)s_width * s_height * 3;
    bool do_full = s_force_full || huge ||
                   s_partial_count >= H752_FULL_REFRESH_INTERVAL;

    int rc;
    if (do_full) {
        int clear_mode = s_force_full ? CLEAR_SLOW : CLEAR_FAST;
        ESP_LOGD(TAG, "H752 full %d,%d-%d,%d clear=%d", x0, y0, x1, y1, clear_mode);
        rc = s_epd->fullUpdate(clear_mode, false);
        if (rc != BBEP_SUCCESS) {
            ESP_LOGW(TAG, "FastEPD fullUpdate failed: %d", rc);
        }
        s_epd->backupPlane();
        s_partial_count = 0;
    } else {
        ESP_LOGD(TAG, "H752 partial rows %d-%d", y0, y1);
        /* partialUpdate copies the driven rows into the previous plane
         * itself; calling backupPlane() here would also back up rows it
         * did NOT drive and leave them permanently undrawn. */
        rc = s_epd->partialUpdate(false, y0, y1);
        if (rc != BBEP_SUCCESS) {
            ESP_LOGW(TAG, "FastEPD partialUpdate failed: %d", rc);
        }
        s_partial_count++;
    }
    s_clip_x0 = s_clip_y0 = s_clip_x1 = s_clip_y1 = -1;
    s_force_full = false;
}

extern "C" void display_full_refresh(void)
{
    if (!s_initialized) return;
    s_force_full = true;
    s_clip_x0 = s_clip_y0 = s_clip_x1 = s_clip_y1 = -1;
    mark_dirty_rect(0, 0, s_width, s_height);
    display_flush();
}

extern "C" void display_request_full_refresh(void)
{
    if (!s_initialized) return;
    s_force_full = true;
}

extern "C" void display_set_partial_clip(int x, int y, int w, int h)
{
    if (w <= 0 || h <= 0) {
        s_clip_x0 = s_clip_y0 = s_clip_x1 = s_clip_y1 = -1;
        return;
    }
    s_clip_x0 = std::max(0, x * H752_SCALE);
    s_clip_y0 = std::max(0, y * H752_SCALE);
    s_clip_x1 = std::min(s_width  - 1, (x + w) * H752_SCALE - 1);
    s_clip_y1 = std::min(s_height - 1, (y + h) * H752_SCALE - 1);
}

extern "C" uint8_t *display_get_buffer(void)
{
    return s_fb;
}

extern "C" int display_get_buffer_size(void)
{
    return FRAMEBUFFER_BYTES;
}

extern "C" void display_set_backlight(int percent)
{
    if (!s_bl_inited) return;
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    uint32_t duty = (uint32_t)((H752_BL_LEDC_DUTY_MAX * percent) / 100);
    ESP_ERROR_CHECK(ledc_set_duty(H752_BL_LEDC_MODE, H752_BL_LEDC_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(H752_BL_LEDC_MODE, H752_BL_LEDC_CHANNEL));
}

extern "C" void display_sleep(void)
{
}

extern "C" void display_wake(void)
{
}

extern "C" void display_deep_sleep_prepare(void)
{
    if (s_bl_inited) {
        ledc_set_duty(H752_BL_LEDC_MODE, H752_BL_LEDC_CHANNEL, 0);
        ledc_update_duty(H752_BL_LEDC_MODE, H752_BL_LEDC_CHANNEL);
    }
    gpio_hold_dis((gpio_num_t)H752_BL_PIN);
    gpio_reset_pin((gpio_num_t)H752_BL_PIN);
    gpio_set_direction((gpio_num_t)H752_BL_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)H752_BL_PIN, 0);
    gpio_hold_en((gpio_num_t)H752_BL_PIN);

    if (s_epd) {
        s_epd->deInit();
        delete s_epd;
        s_epd = nullptr;
    }
    s_initialized = false;
}

#endif /* CONFIG_DRAFTLING_DISPLAY_H752_EPD */
