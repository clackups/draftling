#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*
 * Initialize the display hardware.
 *
 * Parameter mapping depends on the selected hardware model:
 *
 *  Waveshare RLCD (display_rlcd.cpp):
 *      mosi, sck, dc, cs, rst, busy=-1, width, height
 *
 *  M5Stack PaperS3 (display_epdiy.cpp + epd_board_papers3.c):
 *      all pin parameters are ignored - the vroland/epdiy library
 *      configures the parallel-bus and control GPIOs internally
 *      from the in-tree PaperS3 board definition. Width/height
 *      must still match the panel (960 x 540 landscape).
 *
 *  LilyGO T5 E-Paper S3 Pro / Pro Lite (display_epdiy.cpp):
 *      all pin parameters are ignored - epdiy's built-in
 *      epd_board_v7 owns the GPIOs. Width/height must match.
 */
void display_init(int pin_a, int pin_b, int pin_c, int pin_d,
                  int pin_e, int pin_f, int width, int height);

void display_clear(uint8_t color);
void display_set_pixel(uint16_t x, uint16_t y, uint8_t color);

/*
 * Push the framebuffer to the panel.
 *
 * On the e-paper backends this performs an automatic frame
 * diff against the last displayed frame and uses a partial refresh
 * when only a small region changed, falling back to a full refresh
 * once every N partials to clear residual ghosting (the threshold
 * is configurable via Kconfig). On the RLCD model it always pushes
 * the entire framebuffer.
 */
void display_flush(void);

/*
 * Force a full refresh that clears any accumulated ghosting on the
 * e-paper. On the RLCD model this is equivalent to display_flush().
 */
void display_full_refresh(void);

/*
 * Request that the *next* display_flush() be promoted to a full
 * (ghost-clearing) refresh, without performing a flush right now.
 *
 * Use this from contexts that are about to mark several LVGL regions
 * dirty in rapid succession (e.g. closing an overlay AND switching
 * screens), so that the resulting redraws coalesce into a single
 * full refresh instead of two back-to-back partial refreshes. On
 * the epdiy backend this avoids a known LCD-render wedge where the
 * `epd_prep` feeder tasks busy-spin at top priority across both
 * cores, starve IDLE0 and trip the task watchdog when large GL16
 * partials are issued back-to-back before the EPD rail has fully
 * settled. The latch is consumed by the next display_flush().
 *
 * On backends that do not maintain a partial-refresh state machine
 * (RLCD, AXS15231B, MIPI-DSI) this is a no-op -- those backends
 * always push a full frame anyway.
 */
void display_request_full_refresh(void);

uint8_t *display_get_buffer(void);
int display_get_buffer_size(void);

#include <stdbool.h>

/*
 * Optional fast path: push an LVGL RGB565 framebuffer area directly
 * to the panel without going through the per-pixel display_set_pixel
 * conversion. Returns true if the backend handled the push (in which
 * case the caller still needs to call display_flush() to trigger the
 * panel refresh), or false to fall back to the legacy per-pixel
 * path.
 *
 * Implemented by the epdiy and colour LCD backends, which can
 * convert RGB565 -> grayscale (or push RGB565 directly) far more
 * efficiently than our RGB565 -> 1-bit -> 8-bit grayscale round-trip.
 * Other backends return false.
 */
bool display_push_rgb565(int x, int y, int w, int h, const void *color_map);

/*
 * Optional: clip the next display_flush()'s panel refresh region.
 *
 * After this call, the *next* display_flush() refreshes only pixels
 * inside the intersection of its accumulated dirty bounding box and
 * the rectangle (x, y, w, h). The clip is one-shot: it is consumed
 * (and reset) by the next display_flush(), regardless of whether the
 * intersection was empty.
 *
 * The caller must guarantee that no pixels outside the clip rectangle
 * actually changed since the previous flush -- otherwise stale
 * content remains visible on the e-paper. The framebuffer pushed via
 * display_push_rgb565()/display_set_pixel() is always written in
 * full; only the panel refresh is narrowed.
 *
 * Pass w <= 0 or h <= 0 to clear any pending clip.
 *
 * Implemented by the epdiy e-paper backend (display_epdiy.cpp).
 * On other backends this is a no-op (the full dirty bbox is
 * refreshed).
 */
void display_set_partial_clip(int x, int y, int w, int h);

/*
 * Put the display panel into low-power sleep mode.
 *
 * On color-LCD backends (AXS15231B) this turns the backlight
 * off and sends DISPOFF + SLPIN so the controller drops to its
 * sleep current. On e-paper / reflective-LCD / RLCD backends where
 * the panel retains its image without power the call is a no-op.
 *
 * Used by the standby manager's CONFIG_DRAFTLING_STANDBY_DISPLAY_OFF
 * code path, which keeps the MCU running but blanks the display
 * until an input event arrives. Pair with display_wake() to bring
 * the panel back without re-running the full init sequence.
 */
void display_sleep(void);

/*
 * Wake the display panel from display_sleep(). Sends SLPOUT + DISPON
 * on backends that implement display_sleep(), turns the backlight
 * back on, and (where supported) requests a full repaint on the
 * next display_flush() so any framebuffer drift is corrected. No-op
 * on backends with no display_sleep().
 */
void display_wake(void);

/*
 * Prepare the panel + backlight to survive deep sleep with the
 * display visibly OFF.
 *
 * Unlike display_sleep() (which is paired with display_wake() and
 * is used when the MCU stays running), this is a one-shot call
 * made by the standby manager just before esp_deep_sleep_start().
 * After this call returns, deep sleep is expected to follow
 * immediately; there is no symmetric "after-deep-sleep" routine
 * because deep sleep wake is a hard reset that re-runs all init
 * code from scratch.
 *
 * On backends with a controllable backlight GPIO this drives the
 * pin to its OFF level and uses gpio_hold_en() +
 * gpio_deep_sleep_hold_en() so the LOW level survives into deep
 * sleep (otherwise the IO mux releases the pin and any external
 * pull-up would re-light the backlight). On backends without a
 * controllable backlight (reflective LCD, e-paper) this is a
 * no-op.
 */
void display_deep_sleep_prepare(void);

/*
 * AXS15231B QSPI color-LCD driver init.
 *
 * Used by boards with CONFIG_DRAFTLING_DISPLAY_AXS15231B (Waveshare
 * ESP32-S3-Touch-LCD-3.49 and Guition JC3248W535). The driver needs
 * 9 GPIOs -- more than display_init()'s 6 pin slots can carry --
 * so it has its own struct-based init. Call this *instead of*
 * display_init() on AXS15231B boards.
 *
 * After this returns, display_clear/display_set_pixel/
 * display_push_rgb565/display_flush behave like on any other backend.
 */
typedef struct {
    int cs;
    int sck;
    int d0;
    int d1;
    int d2;
    int d3;
    int rst;
    int te;       /* tearing-effect input, -1 if unused */
    int bl;       /* backlight enable, -1 if always-on / external.
                   * When >= 0 the backend drives this GPIO with LEDC
                   * PWM whose duty is set via display_set_backlight();
                   * the editor's F1 -> Settings menu persists the
                   * brightness in NVS. */
    int width;
    int height;
    /* Software 90-deg-CW rotation. Set to true on boards whose
     * AXS15231B silently ignores the MADCTL MV (swap-XY) bit but
     * whose physical panel is mounted in portrait orientation
     * (native scanout is height x width). When set, the backend
     * keeps its internal framebuffer in logical orientation
     * (width x height) so push/clear/set-pixel paths stay
     * unchanged, and transposes per-row into the DMA scratch
     * buffer at flush time, addressing the panel in its native
     * portrait coordinate space. Known true on Guition
     * JC3248W535; left false on Waveshare Touch-LCD-3.49 whose
     * 640x172 panel is natively landscape and works with
     * MADCTL=0x00 directly. */
    bool swap_xy;
    /* Skip the long AXS15231B vendor-register init block
     * (0xBB unlock + 0xA0/0xA2/0xC1/0xC4..0xE2/... + 0xBB lock).
     *
     * The vendor block our driver sends was hand-copied from the
     * Guition JC3248W535 reference; it is the panel-vendor recipe
     * for that board's 480x320 panel and is required to make it
     * display anything (the JC3248W535 ships with vendor regs at
     * defaults that produce only a brief flash of garbage on
     * display-on). Other AXS15231B boards may ship pre-tuned at
     * their factory POR defaults and do NOT want our JC3248W535
     * recipe written on top -- the Waveshare ESP32-S3-Touch-LCD-3.49
     * is one such board: its 172x640 panel is correct out of POR,
     * and its reference firmware (using the upstream
     * `espressif/esp_lcd_axs15231b` IDF component) supplies only
     * `{SLPOUT, DISPON}` as init commands. Sending the JC3248W535
     * vendor values to that panel clobbers the correct factory
     * defaults with values intended for a different panel geometry,
     * producing a black screen on cold boot until the user presses
     * RESET (warm reset masks the bug because the panel had pixels
     * in internal RAM from the previous run, and the 250 ms RST-LOW
     * pulse does not fully wipe that state on a panel whose VCC
     * has been continuous).
     *
     * Set true on boards whose panel does NOT need our JC3248W535
     * vendor block; leave false (default) on JC3248W535 itself. */
    bool skip_vendor_init;
    /* Optional: GPIO pin to drive LOW + hold across deep sleep to
     * cut the backlight, when the normal `bl` field is not suitable.
     *
     * Used by boards whose BL-enable pin cannot be actively driven
     * during normal operation (the boost circuit only works when the
     * pin is left high-Z, with an external pull-up holding it HIGH),
     * but CAN be driven LOW to cut power. On those boards, `bl` is
     * -1 (no normal-operation drive) and `bl_deep_sleep_cut` carries
     * the GPIO number that display_deep_sleep_prepare() drives LOW
     * + latches with gpio_hold_en + gpio_deep_sleep_hold_en before
     * esp_deep_sleep_start. On the next boot, display_axs15231b_init
     * calls gpio_hold_dis on this pin and returns it to high-Z (no
     * gpio_config call), so the external pull-up re-lights the BL
     * exactly as on a true cold boot.
     *
     * Set to -1 (default) on boards that do not need this. Kept
     * available for future boards whose BL boost circuit cannot be
     * driven at all during normal operation; no current board sets
     * it (the Waveshare ESP32-S3-Touch-LCD-3.49 previously used
     * this path but now drives its active-LOW BL on GPIO 8 with
     * normal LEDC PWM, matching Waveshare's reference firmware). */
    int bl_deep_sleep_cut;
    /* True when the BL pin (`bl`) is active LOW: a logical LOW level
     * (LEDC duty 0) turns the backlight ON at full brightness, and a
     * logical HIGH (LEDC duty == DUTY_MAX) turns it OFF. This matches
     * the Waveshare ESP32-S3-Touch-LCD-3.49 boost circuit, whose
     * reference firmware (Examples/ESP-IDF/10_LVGL_V9_Test/components/
     * lcd_bl_pwm_bsp/lcd_bl_pwm_bsp.h) defines `LCD_PWM_MODE_255 =
     * (0xff - 255) = 0` as the "fully bright" duty.
     *
     * When false (default), the BL pin is active HIGH: duty MAX = ON,
     * duty 0 = OFF (Guition JC3248W535 convention). */
    bool bl_active_low;
} display_axs15231b_config_t;

/*
 * Set the LCD backlight brightness.
 *
 * percent is clamped to [0, 100]. 0 = backlight off, 100 = full
 * brightness. The exact PWM frequency / resolution is backend-
 * specific (LEDC PWM at ~5 kHz on the AXS15231B
 * backend).
 *
 * No-op on backends that have no controllable backlight
 * (reflective LCD, e-paper). Boards whose displays expose a
 * backlight set CONFIG_DRAFTLING_DISPLAY_HAS_BACKLIGHT in
 * Kconfig.projbuild; the editor uses that flag to decide whether
 * to show a "Backlight" entry in the F1 Settings menu.
 */
void display_set_backlight(int percent);

void display_axs15231b_init(const display_axs15231b_config_t *cfg);

/*
 * ST7789 SPI color-LCD driver init (via the stock ESP-IDF `esp_lcd`
 * component's esp_lcd_new_panel_st7789() API; no managed component
 * required).
 *
 * Used by boards with CONFIG_DRAFTLING_DISPLAY_ST7789 (RockBase
 * NM-CYD-C5). The panel, MicroSD card, and (optionally) a resistive
 * touch controller all share one physical SPI bus with separate CS
 * lines, so `spi_host` lets the caller pick which SPI peripheral the
 * backend should initialize; main.cpp reuses the same host id (with
 * mosi/sck/miso passed as -1) when it later calls sd_card_init_spi().
 * Call this *instead of* display_init() on ST7789 boards.
 *
 * After this returns, display_clear/display_set_pixel/
 * display_push_rgb565/display_flush behave like on any other backend.
 */
typedef struct {
    int spi_host; /* e.g. SPI2_HOST; the bus is shared with the SD
                  * card (and, on boards with resistive touch, the
                  * touch controller) via separate CS lines. */
    int cs;
    int sck;
    int mosi;
    int miso; /* -1 if the panel never reads back (write-only) */
    int dc;
    int rst;  /* -1 if RESET is tied to the board RST / not wired */
    int bl;   /* backlight enable, -1 if always-on / external.
              * When >= 0 the backend drives this GPIO with LEDC PWM
              * whose duty is set via display_set_backlight(). */
    int width;
    int height;
    /* IPS ST7789 panels need color inversion (INVON) for pixel
     * value 0x0000 to render black and 0xFFFF to render white;
     * without it the output is inverted. */
    bool invert_colors;
} display_st7789_config_t;

void display_st7789_init(const display_st7789_config_t *cfg);

/*
 * Hand a caller-created (driver-NG) I2C master bus handle to the
 * display backend. Must be called *before* display_init() to take
 * effect.
 *
 * On the EPDIY backend (LilyGO T5 E-Paper S3 Pro / Pro Lite), the
 * on-board I2C bus is physically shared with the GT911 touch
 * controller. The shared-bus path lets `main.cpp` create the
 * `i2c_master_bus_handle_t` once and pass it to both epdiy (via
 * `epd_init_with_config()` -> `EpdInitConfig.i2c.bus_handle`) and
 * the touchscreen component (via `touchscreen_config_t.i2c_bus`),
 * which is the only way ESP-IDF allows two driver-NG consumers to
 * coexist on the same I2C port. When this function is not called
 * (or `bus_handle` is NULL) the EPDIY backend creates its own bus
 * internally, matching epdiy's default behaviour.
 *
 * The parameter is `void *` rather than `i2c_master_bus_handle_t`
 * to keep `display.h` free of an ESP-IDF driver include in callers
 * that do not need the shared-bus path; the EPDIY backend casts
 * back to `i2c_master_bus_handle_t` internally. The bus is created
 * and owned by the caller; the display backend never destroys it.
 *
 * No-op on backends that do not use I2C (RLCD, MIPI-DSI, PaperS3
 * epdiy board) or whose I2C does not need to be shared with another
 * component (AXS15231B's optional touch sits on a separate bus
 * today).
 */
void display_set_shared_i2c_bus(void *bus_handle);

#ifdef __cplusplus
}
#endif
