#include "sdkconfig.h"
#if defined(CONFIG_DRAFTLING_DISPLAY_EPDIY)

/*
 * LilyGO T5 E-Paper S3 Pro / Pro Lite e-paper display driver
 * (thin shim over vroland/epdiy).
 *
 * Hardware
 * --------
 * Both Pro and Pro Lite carry the same 4.7" 960x540 ED047TC1 e-paper
 * panel (landscape), driven by an 8-bit parallel data bus on direct
 * ESP32-S3 GPIOs plus a TPS65185 high-voltage supply commanded over
 * I2C through a PCA9535PW I/O expander. All those pins are owned by
 * the epdiy library's `epd_board_v7` configuration -- the matching
 * board id documented in vroland/epdiy's README "LilyGo Boards"
 * table for the "LilyGo T5 S3 E-Paper Pro".
 *
 * Architecture
 * ------------
 * epdiy's high-level API (`epd_hl_*`) keeps a 4-bpp grayscale
 * framebuffer (one nibble per panel pixel, ~253 KB at 960x540) in
 * PSRAM. We expose the optional display_push_rgb565() fast path that
 * lvgl_port.cpp uses on colour and e-paper backends and convert the
 * LVGL RGB565 chunks straight into that grayscale framebuffer 1:1
 * (one LVGL pixel per panel pixel). Pushes are batched
 * into a dirty bounding box; display_flush() turns on the panel
 * power rail, runs an `epd_hl_update_area` partial update over the
 * bbox, then powers the rail back off. Every
 * CONFIG_DRAFTLING_EPD_FULL_REFRESH_INTERVAL partial updates we
 * promote the next refresh to a full-screen `MODE_GC16` flashing
 * update to clear residual ghosting.
 *
 * Deep sleep
 * ----------
 * Per the epdiy README and the documented issue #14 ("battery drain
 * after deep sleep"), epd_poweroff() alone is NOT enough; the I2C
 * bus and the LCD-peripheral ISR must also be released with
 * epd_deinit() before esp_deep_sleep_start(). standby_enter_sleep
 * calls display_deep_sleep_prepare() right before the actual sleep
 * call, so this backend implements that as poweroff + deinit. The
 * panel keeps the previous image displayed without power.
 *
 * VCOM
 * ----
 * Each individual ED047TC1 panel has a slightly different optimal
 * VCOM voltage; epdiy's lilygo_board_s3.c hard-codes 1600 mV as the
 * default and the factory firmware exposes a calibration setting in
 * NVS. We use the same 1600 mV default; visible "fading" or uneven
 * grays during partial updates would indicate the value needs to be
 * tuned per board (e.g. via a future Draftling settings entry).
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_heap_caps.h>
#include <esp_timer.h>

#include "epdiy.h"
#include "epd_init_config.h"
#include "epd_board.h"
#include <driver/i2c_master.h>
#include <driver/ledc.h>
#include <driver/gpio.h>

#include "display.h"
#include "lvgl_port.h"

#if defined(CONFIG_DRAFTLING_EPDIY_BOARD_PAPERS3)
extern "C" const EpdBoardDefinition epd_board_papers3;
#define EPDIY_BOARD       (&epd_board_papers3)
/* Boards without a TPS65185 / VCOM register skip the per-board
 * VCOM calibration write. */
#define EPDIY_HAS_VCOM    0
/* PaperS3 has no I2C-attached EPD peripherals, so the shared-bus
 * plumbing main.cpp publishes for the LilyGO T5 (PCA9535 + TPS65185
 * on the same bus as GT911) is unused here. */
#define EPDIY_USES_I2C    0
#else
#define EPDIY_BOARD       (&epd_board_v7)
#define EPDIY_HAS_VCOM    1
#define EPDIY_USES_I2C    1
#endif

static const char *TAG = "DisplayEPDIY";

/* ---- Front-light (white edge-lit LED) PWM ----
 *
 * The LilyGO T5 E-Paper S3 Pro / Pro Lite carry a controllable
 * white front-light driven from GPIO 11 (BOARD_BL_EN in the
 * LilyGO factory firmware -- see Xinyuan-LilyGO/T5S3-4.7-e-paper-PRO
 * docs/pin_define.md and examples/factory/main/ui_port.cpp, which
 * pulses the pin with `analogWrite(BOARD_BL_EN, {0,50,100,230})`
 * for its four brightness levels). We drive it with the same LEDC
 * PWM topology the colour-LCD backends use (5 kHz / 10-bit,
 * timer/channel 0), so the editor's display_set_backlight(percent)
 * plumbing works unchanged. The pin is active HIGH (duty 0 = off,
 * duty MAX = full brightness); 0 % is a legitimate value -- e-paper
 * is reflective and stays readable without any front-light, so the
 * editor Settings cycle includes a 0 % step.
 *
 * Other epdiy board variants (none today) that lack a controllable
 * front-light leave EPDIY_BL_PIN unset and the whole LEDC block
 * compiles out; display_set_backlight() becomes a no-op. */
#if defined(CONFIG_DRAFTLING_MODEL_LILYGO_T5_EPD_S3_PRO)
#define EPDIY_BL_PIN        11
#endif

#if defined(EPDIY_BL_PIN)
#define EPDIY_HAS_BACKLIGHT 1
#define BL_LEDC_TIMER       LEDC_TIMER_0
#define BL_LEDC_MODE        LEDC_LOW_SPEED_MODE
#define BL_LEDC_CHANNEL     LEDC_CHANNEL_0
#define BL_LEDC_DUTY_RES    LEDC_TIMER_10_BIT
#define BL_LEDC_DUTY_MAX    ((1 << 10) - 1)
#define BL_LEDC_FREQ_HZ     5000

static bool s_bl_inited    = false;

static void backlight_pwm_init(int bl_pin)
{
    if (bl_pin < 0 || s_bl_inited) return;

    /* If we just woke from deep sleep, display_deep_sleep_prepare()
     * latched this pad LOW via gpio_hold_en() + (caller's)
     * gpio_deep_sleep_hold_en() so the LED stayed off through
     * sleep. The latch survives the wake reset on ESP32-S3 and
     * keeps the pad clamped LOW regardless of any GPIO matrix /
     * LEDC routing we configure here, which is why the front-light
     * stays dark after wake. Release the per-pin hold (no-op on a
     * genuine cold boot) so ledc_channel_config below can drive
     * the pin. */
    gpio_hold_dis((gpio_num_t)bl_pin);

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
    /* Start in the OFF state (matches the LilyGO factory firmware
     * which pulls BOARD_BL_EN low at boot). The editor restores
     * the persisted brightness from NVS via display_set_backlight()
     * once it has loaded the user setting. */
    c.duty       = 0;
    c.hpoint     = 0;
    ESP_ERROR_CHECK(ledc_channel_config(&c));

    s_bl_inited = true;
}
#endif  /* EPDIY_BL_PIN */

/* Caller-supplied I2C master bus handle (created in main.cpp before
 * display_init), so epdiy and the GT911 touch driver can share the
 * single on-board I2C port. NULL means "let epdiy create its own
 * bus internally" -- matches epdiy's default behaviour and the
 * pre-shared-bus code path. */
static i2c_master_bus_handle_t s_shared_i2c_bus = NULL;

/* High-level state object owned by epdiy (front+back 4-bpp
 * framebuffers, allocated by epd_hl_init). */
static EpdiyHighlevelState s_hl;
static uint8_t            *s_fb     = nullptr;  /* shortcut: epd_hl_get_framebuffer(&s_hl) */
static int                 s_width  = 0;        /* panel pixels */
static int                 s_height = 0;        /* panel pixels */
static bool                s_initialized = false;

/* Logical-to-panel scale (every logical LVGL pixel is rendered as a
 * SCALE x SCALE block of panel pixels). */
/* Panels render 1:1; the former DRAFTLING_DISPLAY_SCALE upscale
 * has been removed (high-density boards use a larger font instead). */
#define EPDIY_SCALE 1

#ifdef CONFIG_DRAFTLING_EPD_FULL_REFRESH_INTERVAL
#define EPDIY_FULL_REFRESH_INTERVAL CONFIG_DRAFTLING_EPD_FULL_REFRESH_INTERVAL
#else
#define EPDIY_FULL_REFRESH_INTERVAL 30
#endif

/* Dirty bounding box accumulated since the last display_flush() (in
 * panel pixels). */
static int  s_dx0 = 0, s_dy0 = 0, s_dx1 = -1, s_dy1 = -1;
static bool s_force_full = true;
static int  s_partial_count = 0;

/* Flush debounce. The epdiy LCD render path on the ESP32-S3 wedges
 * (both `epd_prep` feeder tasks busy-spinning at top priority on both
 * cores, starving IDLE0 and tripping the task watchdog) when EPD
 * updates are issued back-to-back before the previous one has fully
 * quiesced. On the M5Stack PaperS3 the direct-GPIO power gates
 * (EPD_EN/BST_EN) cycle in microseconds, so unlike the TPS65185-based
 * boards there is no natural inter-update spacing and rapid flushes
 * (SD-init failure status, BLE bring-up redraws, fast scrolling) reach
 * epdiy back-to-back and wedge it at boot. Coalesce flushes that
 * arrive within EPDIY_DEBOUNCE_MS of the previous one into a single
 * deferred update, restoring the inter-update spacing the old M5GFX
 * driver provided. Full refreshes (display_clear/display_full_refresh)
 * bypass the debounce. */
#define EPDIY_DEBOUNCE_MS 120
static int64_t            s_last_flush_us    = 0;
static esp_timer_handle_t s_deferred_timer   = nullptr;
static bool               s_deferred_pending = false;
static bool               s_in_deferred_flush = false;

static inline void mark_dirty_rect(int x, int y, int w, int h)
{
    if (w <= 0 || h <= 0) return;
    int x0 = x;
    int y0 = y;
    int x1 = x + w - 1;
    int y1 = y + h - 1;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= s_width)  x1 = s_width  - 1;
    if (y1 >= s_height) y1 = s_height - 1;
    if (x1 < x0 || y1 < y0) return;
    if (s_dx1 < s_dx0 || s_dy1 < s_dy0) {
        s_dx0 = x0; s_dy0 = y0; s_dx1 = x1; s_dy1 = y1;
    } else {
        if (x0 < s_dx0) s_dx0 = x0;
        if (y0 < s_dy0) s_dy0 = y0;
        if (x1 > s_dx1) s_dx1 = x1;
        if (y1 > s_dy1) s_dy1 = y1;
    }
}

static inline void clear_dirty(void)
{
    s_dx0 = 0; s_dy0 = 0; s_dx1 = -1; s_dy1 = -1;
}

/* Write a single panel pixel into the 4-bpp framebuffer. Pixels are
 * packed two-per-byte: low nibble = even x, high nibble = odd x
 * (epdiy convention, see vroland/epdiy:src/output_common/render.c
 * pixel layout). color is the 4-bit gray level (0x0 = black,
 * 0xF = white) in the low nibble of the passed byte. */
static inline void put_panel_pixel(int x, int y, uint8_t gray4)
{
    if ((unsigned)x >= (unsigned)s_width)  return;
    if ((unsigned)y >= (unsigned)s_height) return;
    size_t idx = (size_t)y * (size_t)(s_width / 2) + (size_t)(x >> 1);
    uint8_t b = s_fb[idx];
    if (x & 1) {
        b = (uint8_t)((b & 0x0F) | ((gray4 & 0x0F) << 4));
    } else {
        b = (uint8_t)((b & 0xF0) | (gray4 & 0x0F));
    }
    s_fb[idx] = b;
}

/* Fill a (panel-pixel) rectangle in the 4-bpp framebuffer with a
 * solid gray. Equivalent to per-pixel put_panel_pixel but unrolled
 * for the common case of contiguous runs. */
static void fill_panel_rect(int x, int y, int w, int h, uint8_t gray4)
{
    if (w <= 0 || h <= 0) return;
    int x0 = (x < 0) ? 0 : x;
    int y0 = (y < 0) ? 0 : y;
    int x1 = x + w; if (x1 > s_width)  x1 = s_width;
    int y1 = y + h; if (y1 > s_height) y1 = s_height;
    if (x1 <= x0 || y1 <= y0) return;
    uint8_t packed = (uint8_t)(((gray4 & 0x0F) << 4) | (gray4 & 0x0F));
    int row_bytes = s_width / 2;
    for (int py = y0; py < y1; py++) {
        uint8_t *row = s_fb + (size_t)py * row_bytes;
        int px = x0;
        /* Leading odd byte (high nibble only). */
        if (px & 1) {
            uint8_t &b = row[px >> 1];
            b = (uint8_t)((b & 0x0F) | ((gray4 & 0x0F) << 4));
            px++;
        }
        /* Whole bytes. */
        int end_aligned = x1 & ~1;
        if (end_aligned > px) {
            memset(row + (px >> 1), packed, (size_t)((end_aligned - px) >> 1));
            px = end_aligned;
        }
        /* Trailing pixel (low nibble only). */
        if (px < x1) {
            uint8_t &b = row[px >> 1];
            b = (uint8_t)((b & 0xF0) | (gray4 & 0x0F));
        }
    }
}

/* Threshold an RGB565 word to a 4-bit gray level. We render mostly
 * black-on-white (or white-on-black) UI so a binary threshold on
 * luminance is adequate; intermediate gray values produced by LVGL's
 * font antialiasing collapse to the nearest of pure black / pure
 * white, which keeps the editor's body text crisp. */
static inline uint8_t rgb565_to_gray4(uint16_t v)
{
    /* Quick luma approximation: take the green channel as a proxy.
     * 6-bit green > 31 means "bright" -> white. */
    return ((v >> 5) & 0x3F) >= 32 ? 0x0F : 0x00;
}

/* ---- public API ---- */

extern "C" void display_set_shared_i2c_bus(void *bus_handle)
{
    /* Called by main.cpp before display_init() on epdiy boards. We
     * stash the handle; display_init() reads it and routes epdiy
     * through epd_init_with_config(). Idempotent and NULL-safe. */
    s_shared_i2c_bus = (i2c_master_bus_handle_t)bus_handle;
}

extern "C" void display_init(int /*pin_a*/, int /*pin_b*/, int /*pin_c*/,
                             int /*pin_d*/, int /*pin_e*/, int /*pin_f*/,
                             int width, int height)
{
    if (s_initialized) return;

    /* Bring up the EPD board + display. epd_board_v7 + ED047TC1 is
     * the documented combination for the LilyGO T5 E-Paper S3 Pro
     * (see vroland/epdiy README "LilyGo Boards" table and the
     * factory firmware Xinyuan-LilyGO/T5S3-4.7-e-paper-PRO
     * examples/factory/main/main.cpp `#define DEMO_BOARD epd_board_v7`).
     */
    /* LUT size: on ESP32-S3 epdiy uses RENDER_METHOD_LCD with the
     * optimized PIE vector lookup, which only ever touches the first
     * 1 KB of the conversion LUT (epdiy logs "only 1k of 65536 LUT
     * in use!" with EPD_LUT_64K). The 64K LUT is allocated from
     * MALLOC_CAP_INTERNAL and permanently reserves 64 KB of internal
     * DRAM (src/render.c init_lut_table), which on this board is
     * scarce -- once Bluedroid, LVGL, and the epdiy framebuffers are
     * up, the remaining internal heap falls below the threshold
     * esp_wifi_init() needs for its DMA-only static RX buffers and
     * WiFi connect fails with ESP_ERR_NO_MEM ("Expected to init 4
     * rx buffer, actual is 0"). EPD_LUT_1K matches what the S3
     * vector path actually uses and frees the other 63 KB for WiFi. */

    /* If main.cpp created the shared I2C bus for us, hand it to
     * epdiy via the post-PR-#475 entry point so epdiy adds its
     * TPS65185 / PCA9535 devices to that bus instead of creating
     * its own. Without this the touchscreen component (which also
     * uses driver-NG) cannot coexist on the same physical bus.
     * `epd_init_with_config` supersedes `epd_init` and is called
     * INSTEAD of it; we only reach this branch when a shared bus
     * was published before display_init(). */
    if (s_shared_i2c_bus && EPDIY_USES_I2C) {
        EpdI2cConfig  i2c_cfg = {};
        i2c_cfg.bus_handle    = s_shared_i2c_bus;
        EpdInitConfig cfg     = {};
        cfg.i2c               = &i2c_cfg;
        epd_init_with_config(EPDIY_BOARD, &ED047TC1, EPD_LUT_1K, &cfg);
    } else {
        epd_init(EPDIY_BOARD, &ED047TC1, EPD_LUT_1K);
    }

#if EPDIY_HAS_VCOM
    /* Per-panel VCOM calibration would ideally come from NVS. 1600
     * mV is the epdiy default and the value baked into
     * lilygo_board_s3.c; symptoms of wrong VCOM are fading partials
     * and uneven grays. */
    epd_set_vcom(1600);
#endif

    /* Allocate front+back 4-bpp grayscale framebuffers in PSRAM. */
    s_hl = epd_hl_init(EPD_BUILTIN_WAVEFORM);
    s_fb = epd_hl_get_framebuffer(&s_hl);

    /* Present the panel as 960x540 landscape (USB-C on the right
     * matches EPD_ROT_LANDSCAPE for this board family). */
    epd_set_rotation(EPD_ROT_LANDSCAPE);

    s_width  = epd_rotated_display_width();
    s_height = epd_rotated_display_height();
    if (s_width != width || s_height != height) {
        ESP_LOGW(TAG,
                 "Configured framebuffer size %dx%d does not match epdiy "
                 "panel size %dx%d; using panel size. Update "
                 "CONFIG_DRAFTLING_DISPLAY_WIDTH/HEIGHT (delete sdkconfig "
                 "and rebuild to pick up the new defaults).",
                 width, height, s_width, s_height);
    }

    /* Start with a clean white panel. */
    fill_panel_rect(0, 0, s_width, s_height, 0x0F);
    epd_poweron();
    epd_clear();
    epd_hl_update_screen(&s_hl, MODE_GC16, 25);
    epd_poweroff();

    clear_dirty();
    s_force_full     = true;
    s_partial_count  = 0;
    s_initialized    = true;

#if defined(EPDIY_HAS_BACKLIGHT)
    /* Bring up the front-light PWM. We do this after epdiy has
     * claimed its own pins so an accidental GPIO conflict shows up
     * here (LEDC owns the pin once ledc_channel_config returns). */
    backlight_pwm_init(EPDIY_BL_PIN);
#endif

    ESP_LOGI(TAG, "epdiy display initialized "
                  "(%dx%d panel, scale=%d -> %dx%d logical), full refresh "
                  "every %d partials",
             s_width, s_height, EPDIY_SCALE,
             s_width / EPDIY_SCALE, s_height / EPDIY_SCALE,
             EPDIY_FULL_REFRESH_INTERVAL);
}

extern "C" void display_clear(uint8_t color)
{
    if (!s_initialized) return;
    uint8_t g = (color != 0) ? 0x0F : 0x00;
    fill_panel_rect(0, 0, s_width, s_height, g);
    mark_dirty_rect(0, 0, s_width, s_height);
    s_force_full = true;
}

extern "C" void display_set_pixel(uint16_t x, uint16_t y, uint8_t color)
{
    if (!s_initialized) return;
    /* Coordinates are *logical* pixels -- expand to a SCALE x SCALE
     * block of panel pixels. */
    int px = (int)x * EPDIY_SCALE;
    int py = (int)y * EPDIY_SCALE;
    fill_panel_rect(px, py, EPDIY_SCALE, EPDIY_SCALE,
                    (color != 0) ? 0x0F : 0x00);
    mark_dirty_rect(px, py, EPDIY_SCALE, EPDIY_SCALE);
}

extern "C" bool display_push_rgb565(int x, int y, int w, int h,
                                    const void *color_map)
{
    if (!s_initialized || color_map == nullptr) return false;
    if (w <= 0 || h <= 0) return false;
    const uint16_t *src = (const uint16_t *)color_map;

    /* Per-logical-pixel: write a SCALE x SCALE block into the
     * grayscale framebuffer. */
    for (int sy = 0; sy < h; sy++) {
        for (int sx = 0; sx < w; sx++) {
            uint16_t v = src[(size_t)sy * w + sx];
            uint8_t g = rgb565_to_gray4(v);
            int px = (x + sx) * EPDIY_SCALE;
            int py = (y + sy) * EPDIY_SCALE;
            fill_panel_rect(px, py, EPDIY_SCALE, EPDIY_SCALE, g);
        }
    }

    int dx = x * EPDIY_SCALE;
    int dy = y * EPDIY_SCALE;
    int dw = w * EPDIY_SCALE;
    int dh = h * EPDIY_SCALE;
    mark_dirty_rect(dx, dy, dw, dh);
    return true;
}

/* Deferred-flush timer callback. Runs in the esp_timer task, so it
 * must take the LVGL port lock to serialise with the LVGL task's
 * flush callback (two concurrent epd_hl_update_area() calls would
 * themselves wedge the render path). s_in_deferred_flush tells the
 * re-entered display_flush() to skip the debounce guard and perform
 * the actual update. */
static void deferred_flush_cb(void *arg)
{
    (void)arg;
    draftling_lvgl_port_lock(-1);
    s_in_deferred_flush = true;
    display_flush();
    s_in_deferred_flush = false;
    draftling_lvgl_port_unlock();
}

extern "C" void display_flush(void)
{
    if (!s_initialized) return;
    if (s_dx1 < s_dx0 || s_dy1 < s_dy0) return;

    /* Debounce: coalesce rapid back-to-back flushes so consecutive EPD
     * updates never start closer than EPDIY_DEBOUNCE_MS apart (see the
     * s_last_flush_us comment block). Full refreshes bypass this. The
     * deferred callback re-enters display_flush with s_in_deferred_flush
     * set, which skips this guard. */
    if (!s_in_deferred_flush && !s_force_full) {
        int64_t now = esp_timer_get_time();
        int64_t elapsed_ms = (now - s_last_flush_us) / 1000;
        if (s_last_flush_us != 0 && elapsed_ms < EPDIY_DEBOUNCE_MS) {
            if (!s_deferred_pending) {
                if (!s_deferred_timer) {
                    const esp_timer_create_args_t args = {
                        .callback = &deferred_flush_cb,
                        .arg = nullptr,
                        .dispatch_method = ESP_TIMER_TASK,
                        .name = "epd_flush",
                        .skip_unhandled_events = true,
                    };
                    esp_err_t terr = esp_timer_create(&args, &s_deferred_timer);
                    if (terr != ESP_OK) {
                        ESP_LOGE(TAG, "deferred flush timer create failed: %s",
                                 esp_err_to_name(terr));
                    }
                }
                if (s_deferred_timer) {
                    int64_t remaining_ms = EPDIY_DEBOUNCE_MS - elapsed_ms;
                    if (remaining_ms < 1) remaining_ms = 1;
                    s_deferred_pending = true;
                    esp_timer_start_once(s_deferred_timer,
                                         (uint64_t)remaining_ms * 1000);
                }
            }
            return;
        }
    }
    s_deferred_pending = false;

    int dx0 = s_dx0, dy0 = s_dy0, dx1 = s_dx1, dy1 = s_dy1;
    int dw  = dx1 - dx0 + 1;
    int dh  = dy1 - dy0 + 1;

    /* If the dirty area covers most of the screen, a full refresh is
     * cheaper (and cleaner) than a giant partial. */
    bool huge = ((long)dw * dh) * 4 > ((long)s_width * s_height) * 3;

    /* Under rapid back-to-back flushes (e.g. typing at the top of a
     * long document, which reflows nearly every line), epdiy's LCD
     * render path on the ESP32-S3 has been observed to wedge both
     * `epd_prep` feeder tasks busy-spinning at top priority on both
     * cores, starving IDLE0 and tripping the task watchdog with no
     * recovery. The trigger is large MODE_GL16 partials issued
     * back-to-back before the EPD rail has fully settled. Promote
     * near-full partials to a MODE_GC16 full refresh whenever the
     * previous flush finished less than `FAST_SCROLL_GAP_MS` ago:
     * one ~430 ms full refresh is more predictable than a sequence
     * of ~430 ms 60-90% partials, and the flash is barely visible
     * during fast scrolling because the content is changing anyway.
     * Threshold of 40% of screen area is well below the 75% `huge`
     * cutoff above, so steady-state small partials (cursor blink,
     * status bar) are unaffected. */
    static const int     FAST_SCROLL_GAP_MS       = 800;
    static const long    FAST_SCROLL_AREA_NUM     = 2;   /* >= 2/5 ... */
    static const long    FAST_SCROLL_AREA_DEN     = 5;   /* ... of screen */
    int64_t now_us = esp_timer_get_time();
    bool fast_scroll =
        (s_last_flush_us != 0) &&
        ((now_us - s_last_flush_us) < (int64_t)FAST_SCROLL_GAP_MS * 1000) &&
        (((long)dw * dh) * FAST_SCROLL_AREA_DEN >
         ((long)s_width * s_height) * FAST_SCROLL_AREA_NUM);

    bool do_full = s_force_full || huge || fast_scroll ||
                   s_partial_count >= EPDIY_FULL_REFRESH_INTERVAL;

    epd_poweron();
    if (do_full) {
        /* MODE_GC16: full-screen 16-gray flashing update, clears any
         * residual ghosting. */
        epd_hl_update_screen(&s_hl, MODE_GC16, 25);
        s_partial_count = 0;
        s_force_full = false;
    } else {
        /* MODE_GL16: 16-gray non-flashing partial update. Good for
         * UI changes where the user should not see a black flash. */
        EpdRect r = { .x = dx0, .y = dy0, .width = dw, .height = dh };
        epd_hl_update_area(&s_hl, MODE_GL16, 25, r);
        s_partial_count++;
    }
    epd_poweroff();

    s_last_flush_us = esp_timer_get_time();
    clear_dirty();
}

extern "C" void display_full_refresh(void)
{
    if (!s_initialized) return;
    mark_dirty_rect(0, 0, s_width, s_height);
    s_force_full = true;
    display_flush();
}

extern "C" void display_request_full_refresh(void)
{
    /* Latch the next display_flush() to a GC16 full refresh.  Safe
     * to call from any task: this is a single bool write, and the
     * flush itself will be issued from the LVGL render task as
     * usual. Multiple requests coalesce naturally because the flag
     * is only consumed (cleared) by the next flush.  We deliberately
     * do NOT mark the framebuffer dirty here -- if no LVGL redraw
     * follows, no flush happens and the latch simply waits for the
     * next legitimate flush, at which point the dirty bbox is
     * irrelevant because the GC16 path refreshes the whole panel. */
    if (!s_initialized) return;
    s_force_full = true;
}

extern "C" void display_set_partial_clip(int /*x*/, int /*y*/,
                                         int /*w*/, int /*h*/)
{
    /* No-op on this backend; the dirty bbox already drives the
     * refresh region. */
}

extern "C" uint8_t *display_get_buffer(void)
{
    /* No external 1-bpp framebuffer; epdiy owns the 4-bpp one. */
    return nullptr;
}

extern "C" int display_get_buffer_size(void)
{
    return 0;
}

extern "C" void display_set_backlight(int percent)
{
#if defined(EPDIY_HAS_BACKLIGHT)
    if (percent < 0)   percent = 0;
    if (percent > 100) percent = 100;
    if (!s_bl_inited) return;
    uint32_t duty = (uint32_t)((BL_LEDC_DUTY_MAX * percent) / 100);
    ESP_ERROR_CHECK(ledc_set_duty(BL_LEDC_MODE, BL_LEDC_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(BL_LEDC_MODE, BL_LEDC_CHANNEL));
#else
    (void)percent;
    /* This epdiy board variant has no controllable front-light. */
#endif
}

extern "C" void display_sleep(void)
{
    /* E-paper retains its image without power. No-op (the standby
     * manager wipes the panel to white separately before deep sleep). */
}

extern "C" void display_wake(void)
{
    /* No-op (see display_sleep). */
}

extern "C" void display_deep_sleep_prepare(void)
{
#if defined(EPDIY_HAS_BACKLIGHT)
    /* Cut the front-light so it does not burn battery while the
     * MCU is asleep. We drive the pin directly via the LEDC duty
     * register (faster than tearing the timer down) and leave it
     * up to the post-wake display_init() to re-initialize the
     * channel from scratch. */
    if (s_bl_inited) {
        ledc_set_duty(BL_LEDC_MODE, BL_LEDC_CHANNEL, 0);
        ledc_update_duty(BL_LEDC_MODE, BL_LEDC_CHANNEL);
    }
    /* Setting LEDC duty to 0 stops switching, but the LEDC peripheral
     * is powered down by the digital domain in deep sleep, which
     * releases the pad. On an active-HIGH LED driver (the LilyGO T5
     * Pro front-light is wired this way) the pin floats and the
     * LED can dimly glow on leakage current. Reclaim the pin as a
     * plain GPIO driven LOW and latch the level with gpio_hold_en()
     * so the bond pad stays at 0 V across deep sleep. The caller
     * (T5 pre-sleep hook) is responsible for calling
     * gpio_deep_sleep_hold_en() once before esp_deep_sleep_start()
     * so the hold actually survives into sleep. */
    if (EPDIY_BL_PIN >= 0) {
        gpio_hold_dis((gpio_num_t)EPDIY_BL_PIN);
        gpio_reset_pin((gpio_num_t)EPDIY_BL_PIN);
        gpio_set_direction((gpio_num_t)EPDIY_BL_PIN, GPIO_MODE_OUTPUT);
        gpio_set_level((gpio_num_t)EPDIY_BL_PIN, 0);
        gpio_hold_en((gpio_num_t)EPDIY_BL_PIN);
    }
#endif
    if (!s_initialized) return;
    /* epdiy README + issue #14: epd_poweroff() alone leaks battery
     * after deep sleep -- the I2C bus and LCD-peripheral ISR must
     * also be released via epd_deinit() before esp_deep_sleep_start.
     * The wake from deep sleep is a hard reset that re-runs
     * display_init() from scratch, so it is safe to fully release
     * here. epd_poweroff() drives the TPS65185 WAKEUP line LOW via
     * the PCA9535 expander, dropping the EPD power IC into its
     * <1 uA standby. */
    epd_poweroff();
    epd_deinit();
    s_initialized = false;
}

#endif /* CONFIG_DRAFTLING_DISPLAY_EPDIY */
