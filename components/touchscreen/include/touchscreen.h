#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/*
 * Touchscreen driver + LVGL pointer input device.
 *
 * Supports two controllers, selected at build time via
 * CONFIG_DRAFTLING_TOUCH_CONTROLLER:
 *   - AXS5106L (magic-packet I2C, e.g. Guition JC3248W535)
 *   - GT911    (register-based I2C, e.g. M5Stack PaperS3)
 *
 * Compiled in only when CONFIG_DRAFTLING_TOUCHSCREEN is set.
 * Initialize from main.cpp after the LVGL port and the LCD have
 * been brought up.
 *
 * After touchscreen_init() returns:
 *   - An LVGL pointer indev is registered against the default display
 *     and routes click / press / gesture events to widgets normally.
 *   - touchscreen_get_int_gpio() returns the (RTC-capable) INT GPIO
 *     for the standby manager to arm EXT0 wake.
 *   - touchscreen_is_pressed() reports the current touch state for
 *     code paths that need to poll directly (e.g. the display-off
 *     standby loop).
 *
 * The coordinate transform is parameterised so the same driver works
 * on touch panels mounted in different orientations relative to the
 * LCD. The driver returns logical LVGL coordinates (after the display
 * backend's software rotation and the DRAFTLING_DISPLAY_SCALE down-
 * scale), so the caller does not need to do further math.
 */

typedef struct {
    int sda;            /* I2C SDA GPIO */
    int scl;            /* I2C SCL GPIO */
    int rst;            /* RST GPIO, -1 if tied to LCD reset / fixed */
    int intr;           /* INT GPIO, -1 if not wired */
    uint8_t i2c_addr;   /* 7-bit I2C address (0x3B for AXS5106L,
                         * 0x5D primary / 0x14 backup for GT911). */
    int i2c_port;       /* 0 or 1 (I2C_NUM_0 / I2C_NUM_1) */
    uint32_t i2c_hz;    /* I2C clock, typically 400 kHz */

    /* Native panel resolution as reported by the touch controller
     * (i.e. before any logical-coordinate transform). For the
     * JC3248W535 the panel is natively portrait 320x480, so set
     * native_width=320, native_height=480 even though the LCD is
     * software-rotated to 480x320 landscape. */
    int native_width;
    int native_height;

    /* Logical (LVGL) coordinate range. Touch reports will be
     * transformed and clamped to [0, logical_width) x [0, logical_height).
     * For boards with CONFIG_DRAFTLING_DISPLAY_SCALE > 1 these are the
     * logical (scaled-down) values, not the panel pixel counts. */
    int logical_width;
    int logical_height;

    /* Orientation transforms, applied in this order:
     *   1. mirror_x:  raw_x = native_width  - 1 - raw_x
     *   2. mirror_y:  raw_y = native_height - 1 - raw_y
     *   3. swap_xy:   swap raw_x and raw_y
     *   4. Scale (native -> logical):
     *        out_x = raw_x * logical_width  / native_width_after_swap
     *        out_y = raw_y * logical_height / native_height_after_swap
     *
     * Tune these to match the physical mounting of the touch panel
     * relative to the LCD. */
    bool mirror_x;
    bool mirror_y;
    bool swap_xy;

    /* User-requested display rotation (DRAFTLING_DISPLAY_ROTATE_ANGLE,
     * one of 0 / 90 / 180 / 270). Applied AFTER the
     * native -> logical transform above so the touch coordinates
     * returned to LVGL match the rotated UI seen by the user. With
     * 90 or 270 the output x/y ranges are the swapped
     * (logical_height, logical_width). */
    int user_rotate_deg;
    /* Optional caller-supplied driver-NG I2C master bus handle. When
     * non-NULL, touchscreen_init() skips i2c_new_master_bus() and
     * adds its device to this existing bus instead. Used on boards
     * where another driver-NG consumer (currently vroland/epdiy on
     * the LilyGO T5 E-Paper S3 Pro / Pro Lite) already owns the
     * physical I2C port carrying the touch controller -- ESP-IDF
     * only allows one i2c_new_master_bus() per port. The handle
     * type is `void *` so this header does not need to drag in
     * `driver/i2c_master.h`; the implementation casts back to
     * `i2c_master_bus_handle_t`.
     *
     * When NULL (default) the touchscreen component creates its own
     * bus from sda/scl/i2c_port/i2c_hz, matching the pre-shared-bus
     * behaviour on every other board. The bus is destroyed in
     * future deinit only when the touchscreen component created it
     * itself; an externally supplied handle is left intact for the
     * other consumer to clean up. */
    void *i2c_bus;
} touchscreen_config_t;

/* Initialize the touchscreen and register an LVGL pointer indev.
 * Idempotent: a second call is a no-op. */
void touchscreen_init(const touchscreen_config_t *cfg);

/* True if touchscreen_init() has been called successfully. */
bool touchscreen_is_initialized(void);

/* Return the INT GPIO number passed to touchscreen_init, or -1
 * if not initialized or no INT was wired. Used by the standby
 * manager to choose the EXT0 wake source. */
int touchscreen_get_int_gpio(void);

/* Poll the controller and return the current touch state.
 *
 * Returns true if a finger is currently down. When true, *out_x /
 * *out_y carry the logical coordinates (after the configured
 * orientation transform). out_x / out_y may be NULL.
 *
 * Safe to call from any task; the driver guards I2C access with
 * its own mutex.
 */
bool touchscreen_read(int *out_x, int *out_y);

/* True if a touch was detected on the most recent poll (either via
 * the LVGL read_cb or an explicit touchscreen_read). Used by the
 * standby manager. */
bool touchscreen_is_pressed(void);

/* Optional callback fired when the touch panel's dedicated touch
 * button (the GT911 "key" zone below the visible screen on the
 * LilyGO T5 E-Paper S3 Pro panels) is tapped. Fired once per press
 * (rising edge, with a short re-fire suppression window); the
 * callback runs in the LVGL task context of the indev read poll.
 * Only the GT911 backend implements this; other backends never
 * fire it. */
typedef void (*touchscreen_button_cb_t)(void);
void touchscreen_set_button_callback(touchscreen_button_cb_t cb);

/* Put the touch controller into its lowest-power sleep mode. The
 * controller stops scanning and drops to a few uA until the host
 * either resets it or wakes it via the INT line. Used by the
 * standby manager immediately before esp_deep_sleep_start() on
 * boards (LilyGO T5 E-Paper S3 Pro/Lite) where the controller
 * would otherwise keep scanning and draining the pack.
 *
 * On non-GT911 backends and on the no-touchscreen build this is
 * a no-op so callers don't need conditional compilation. */
void touchscreen_sleep(void);

#ifdef __cplusplus
}
#endif
