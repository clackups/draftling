#include "sdkconfig.h"
#if defined(CONFIG_DRAFTLING_TOUCHSCREEN)

/*
 * I2C touchscreen driver + LVGL pointer input device.
 *
 * Supports two controllers, picked at build time via
 * CONFIG_DRAFTLING_TOUCH_CONTROLLER:
 *
 *   * AXS5106L (Allystar, e.g. Guition JC3248W535):
 *     Speaks a small "magic packet" I2C protocol -- the host writes
 *     an 8-byte read-request packet and reads back 8 bytes
 *     containing a touch count and a single point. The protocol is
 *     shared across Allystar's AXS5106 / AXS5106L variants and
 *     matches the public LovyanGFX / Tactility / Arduino
 *     implementations widely cited for this controller.
 *
 *   * GT911 (Goodix, e.g. M5Stack PaperS3):
 *     Register-addressed protocol -- 16-bit register address sent
 *     MSB-first, status byte at 0x814E (bit 7 = buffer ready, low
 *     nibble = touch-point count), point data at 0x814F+ (8 bytes
 *     per point, X / Y little-endian). After reading, the host
 *     must clear 0x814E so the controller can fill the next frame.
 *     The GT911 listens on either 0x5D or 0x14 depending on the
 *     INT level seen during its internal power-on reset; we probe
 *     both before binding the I2C device. On boards where RST is
 *     not wired to an ESP32 GPIO (PaperS3) the address-select
 *     reset sequence cannot be performed, but probing covers it.
 *
 * In both cases the driver registers an LVGL pointer indev which
 * feeds touch coordinates to the LVGL event system, so widgets
 * receive standard click / press / gesture events. The transform
 * from the controller's native coordinate space to LVGL logical
 * coordinates is parameterised in touchscreen_config_t
 * (mirror_x / mirror_y / swap_xy + native / logical sizes), so the
 * same code works for panels mounted in different orientations
 * relative to the LCD.
 */

#include <cstring>
#include <cstdio>
#include <esp_log.h>
#include <esp_timer.h>
#include <driver/gpio.h>
#include <driver/i2c_master.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "lvgl.h"

#include "touchscreen.h"

/* On the M5Stack Tab5 we delegate the controller-specific bring-up
 * to the upstream espressif/m5stack_tab5 BSP via bsp_touch_new(),
 * which auto-detects v1 (GT911 at 0x14 backup) vs v2 (ST7123 at
 * 0x55) and returns an esp_lcd_touch_handle_t. We then poll through
 * the generic esp_lcd_touch core (esp_lcd_touch_read_data +
 * esp_lcd_touch_get_coordinates), so the same Draftling source
 * handles both board variants transparently. The direct-I2C GT911
 * / AXS5106L paths below are unaffected on every other board. */
#if defined(CONFIG_DRAFTLING_MODEL_M5STACK_TAB5)
#  define DRAFTLING_TOUCH_BSP_M5STACK_TAB5 1
#  include "bsp/touch.h"
#  include "esp_lcd_touch.h"
#endif

static const char *TAG = "Touch";

static bool s_initialized = false;
static touchscreen_config_t s_cfg;
static i2c_master_bus_handle_t s_bus = NULL;
/* True if we created s_bus ourselves and must free it on (future)
 * deinit. False when the bus was supplied by the caller (shared
 * with another driver-NG consumer such as epdiy on the LilyGO T5
 * E-Paper S3 Pro). Mirrors epdiy's own `owns_bus` flag. */
static bool s_owns_bus = false;
static i2c_master_dev_handle_t s_dev = NULL;
static SemaphoreHandle_t s_mux = NULL;
static lv_indev_t *s_indev = NULL;

#if defined(DRAFTLING_TOUCH_BSP_M5STACK_TAB5)
/* esp_lcd_touch handle owned by the m5stack_tab5 BSP. Created by
 * bsp_touch_new() in touchscreen_init() when the Tab5 backend is
 * selected; polled via the generic esp_lcd_touch_* API. */
static esp_lcd_touch_handle_t s_bsp_tp = NULL;
#endif

/* Latched "current touch" state, updated on every successful poll. */
static volatile bool s_pressed_latch = false;
static volatile int  s_x_latch = 0;
static volatile int  s_y_latch = 0;

static inline int clamp(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

/* Map a raw (native_x, native_y) reading to logical LVGL coordinates
 * using the configured orientation flags. */
static void native_to_logical(int nx, int ny, int *ox, int *oy)
{
    if (s_cfg.mirror_x) nx = (s_cfg.native_width  - 1) - nx;
    if (s_cfg.mirror_y) ny = (s_cfg.native_height - 1) - ny;

    int sx_native = s_cfg.native_width;
    int sy_native = s_cfg.native_height;
    int rx = nx;
    int ry = ny;
    if (s_cfg.swap_xy) {
        int t = rx; rx = ry; ry = t;
        t = sx_native; sx_native = sy_native; sy_native = t;
    }

    /* Scale native -> logical. Guard against zero. */
    if (sx_native <= 0) sx_native = 1;
    if (sy_native <= 0) sy_native = 1;
    int lx = (int)((int64_t)rx * s_cfg.logical_width  / sx_native);
    int ly = (int)((int64_t)ry * s_cfg.logical_height / sy_native);

    lx = clamp(lx, 0, s_cfg.logical_width  - 1);
    ly = clamp(ly, 0, s_cfg.logical_height - 1);

    /* Apply the user-requested display rotation so the reported
     * coordinates match the rotated UI seen on screen. */
    int rot = s_cfg.user_rotate_deg;
    if (rot < 0) rot = ((rot % 360) + 360) % 360;
    else        rot = rot % 360;
    switch (rot) {
    case 90:
        *ox = ly;
        *oy = (s_cfg.logical_width  - 1) - lx;
        break;
    case 180:
        *ox = (s_cfg.logical_width  - 1) - lx;
        *oy = (s_cfg.logical_height - 1) - ly;
        break;
    case 270:
        *ox = (s_cfg.logical_height - 1) - ly;
        *oy = lx;
        break;
    default:
        *ox = lx;
        *oy = ly;
        break;
    }
}

/* ---- AXS5106L (magic-packet) driver ---- */
#if defined(CONFIG_DRAFTLING_TOUCH_AXS5106L)

/* AXS5106L "read 1 touch point" magic packet. Bytes 0-3 are the
 * vendor preamble (0xB5, 0xAB, 0xA5, 0x5A); the trailing 0x08 is
 * the number of bytes to return. The controller responds with 8
 * bytes laid out as:
 *   [0]      gesture id (unused)
 *   [1]      touch points (0 or 1)
 *   [2]      event | x_high (high nibble of x in bits 0..3)
 *   [3]      x_low
 *   [4]      finger id | y_high
 *   [5]      y_low
 *   [6..7]   weight / area (ignored)
 */
static const uint8_t AXS5106_READ_CMD[8] = {
    0xB5, 0xAB, 0xA5, 0x5A, 0x00, 0x00, 0x00, 0x08
};

/* Issue a single read of the controller. Returns true if a touch is
 * currently active and fills *out_x / *out_y with logical coords. */
static bool poll_axs5106l(int *out_x, int *out_y)
{
    if (!s_dev) return false;

    uint8_t resp[8] = { 0 };
    esp_err_t err = i2c_master_transmit_receive(
        s_dev,
        AXS5106_READ_CMD, sizeof(AXS5106_READ_CMD),
        resp, sizeof(resp),
        50 /* ms */);
    if (err != ESP_OK) {
        /* I2C transient -- silently drop the sample. Log at DEBUG so
         * we do not flood the console when no controller is wired. */
        ESP_LOGD(TAG, "i2c read failed: %s", esp_err_to_name(err));
        return false;
    }

    /* Some firmwares report the touch count in resp[1]; others put a
     * "frame valid" flag in resp[0]. Accept either:
     *  - resp[1] in [1..5] (explicit point count), OR
     *  - resp[0] == 0 (no gesture) AND (x | y) != 0 (LilyGo's check
     *    in their official AXS15231B driver -- the controller does
     *    not always populate the point-count byte).
     * If none of the above, treat as "finger up". */
    uint8_t points  = resp[1] & 0x0F;
    uint8_t gesture = resp[0];

    int nx = ((int)(resp[2] & 0x0F) << 8) | resp[3];
    int ny = ((int)(resp[4] & 0x0F) << 8) | resp[5];

    bool valid = (points >= 1 && points <= 5) ||
                 (gesture == 0 && (nx != 0 || ny != 0));
    if (!valid) {
        return false;
    }

    int lx, ly;
    native_to_logical(nx, ny, &lx, &ly);
    if (out_x) *out_x = lx;
    if (out_y) *out_y = ly;
    return true;
}

#endif /* CONFIG_DRAFTLING_TOUCH_AXS5106L */

/* ---- GT911 (register-based) driver ---- */
#if defined(CONFIG_DRAFTLING_TOUCH_GT911)

/* GT911 register addresses (datasheet section 7). Sent over I2C as
 * two bytes, high-byte first. */
#define GT911_REG_STATUS    0x814E  /* status / buffer-ready / point count */
#define GT911_REG_POINT1    0x814F  /* first touch point (8 bytes) */
#define GT911_REG_COMMAND   0x8040  /* command register (0x05 = sleep) */
#define GT911_CMD_SLEEP     0x05    /* enter low-power sleep mode */

static esp_err_t gt911_read_reg(uint16_t reg, uint8_t *buf, size_t len)
{
    uint8_t addr[2] = { (uint8_t)((reg >> 8) & 0xFF),
                        (uint8_t)(reg & 0xFF) };
    return i2c_master_transmit_receive(
        s_dev, addr, sizeof(addr), buf, len, 50 /* ms */);
}

static esp_err_t gt911_write_reg(uint16_t reg, uint8_t val)
{
    uint8_t packet[3] = { (uint8_t)((reg >> 8) & 0xFF),
                          (uint8_t)(reg & 0xFF),
                          val };
    return i2c_master_transmit(s_dev, packet, sizeof(packet), 50 /* ms */);
}

/* Number of consecutive I2C-failure responses from the GT911 before
 * we assume the controller has re-POR'd (likely after an epdiy
 * `epd_poweroff()` yanked its RST line via the shared TPS65185 /
 * PCA9535 expander on the LilyGO T5 E-Paper S3 Pro) and try to
 * rebind it. Small enough to recover within a few LVGL ticks
 * (~30 ms at the default 10 ms tick), large enough to ride out
 * benign bus contention with epdiy's own I2C traffic. */
#define GT911_RECOVERY_FAIL_THRESHOLD 3

static uint8_t s_gt911_consec_fail = 0;

/* Re-probe the GT911 at both possible I2C addresses (0x5D and 0x14)
 * and rebind the driver-NG device handle to whichever ACKs. Called
 * from the poll path after several consecutive I2C failures.
 *
 * Background: on the LilyGO T5 E-Paper S3 Pro the GT911's RST line
 * is routed through the TPS65185 / PCA9535 I2C expander used by
 * epdiy. Each display_flush() does epd_poweron() + epd_poweroff();
 * the latter can momentarily yank the expander outputs and the
 * GT911 then re-POR's, picking an I2C address based on the INT
 * level at that instant -- which on this board can land on the
 * backup 0x14 instead of the configured 0x5D. With the I2C device
 * still bound to 0x5D every subsequent register read NAKs and
 * touch is silently dead. The original boot-time probe in
 * touchscreen_init() does not run again, so we have to recover at
 * runtime here. Returns true if a working address was (re)bound. */
static bool gt911_try_recover(void)
{
    if (!s_bus) return false;

    /* Always try both possible GT911 addresses, configured first.
     * GT911 only ever lands on 0x5D or 0x14; if `configured` is
     * something exotic we still try the two canonical addresses so
     * recovery has a chance to succeed. */
    uint8_t configured = s_cfg.i2c_addr;
    uint8_t order[2] = { configured,
                         static_cast<uint8_t>((configured == 0x14) ? 0x5D : 0x14) };

    for (int i = 0; i < 2; i++) {
        uint8_t addr = order[i];
        if (i2c_master_probe(s_bus, addr, 50) != ESP_OK) continue;

        if (addr == s_cfg.i2c_addr) {
            /* Same address still answers -- controller did not
             * drift; the failures were transient bus contention.
             * Keep the existing device handle. */
            ESP_LOGW(TAG, "GT911 recovered at 0x%02X after %u failures",
                     addr, (unsigned)s_gt911_consec_fail);
            return true;
        }

        /* Drift detected -- rebind to the new address. The window
         * between updating s_dev / s_cfg.i2c_addr and removing the
         * old handle is safe because gt911_try_recover() is only
         * ever invoked from poll_gt911(), which runs under s_mux
         * (held by indev_read_cb / touchscreen_read), and all
         * other poll paths take the same mutex -- no other task
         * can be poking the stale handle through s_cfg.i2c_addr. */
        i2c_device_config_t dev_cfg = {};
        dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
        dev_cfg.device_address  = addr;
        dev_cfg.scl_speed_hz    = s_cfg.i2c_hz ? s_cfg.i2c_hz : 400000;
        i2c_master_dev_handle_t newdev = NULL;
        if (i2c_master_bus_add_device(s_bus, &dev_cfg, &newdev) != ESP_OK ||
            !newdev) {
            ESP_LOGW(TAG, "GT911 rebind add_device failed for 0x%02X", addr);
            continue;
        }
        i2c_master_dev_handle_t old = s_dev;
        s_dev = newdev;
        s_cfg.i2c_addr = addr;
        if (old) {
            /* Best-effort removal of the stale handle; on driver-NG
             * the bus survives this and stays shared with epdiy. */
            (void)i2c_master_bus_rm_device(old);
        }
        ESP_LOGW(TAG, "GT911 drifted to 0x%02X (was 0x%02X); rebound",
                 addr, configured);
        return true;
    }
    return false;
}

/* GT911 "key" support: the controller scans an optional capacitive
 * touch key (the round marker below the visible panel on the LilyGO
 * T5 e-paper modules) and reports it in status bit 4 (0x10). Fire
 * the registered callback once per press with a short suppression
 * window so a steady finger produces one event, not a stream. */
static touchscreen_button_cb_t s_button_cb = NULL;
static bool    s_gt911_key_was_down = false;
static int64_t s_gt911_key_last_fire_us = 0;
#define GT911_STATUS_HAVE_KEY      0x10
#define GT911_KEY_REFIRE_US        300000

static bool poll_gt911(int *out_x, int *out_y)
{
    if (!s_dev) return false;

    uint8_t status = 0;
    if (gt911_read_reg(GT911_REG_STATUS, &status, 1) != ESP_OK) {
        ESP_LOGD(TAG, "gt911 status read failed");
        if (s_gt911_consec_fail < 0xFF) s_gt911_consec_fail++;
        if (s_gt911_consec_fail >= GT911_RECOVERY_FAIL_THRESHOLD) {
            if (gt911_try_recover()) {
                s_gt911_consec_fail = 0;
            }
        }
        return false;
    }
    s_gt911_consec_fail = 0;

    /* Buffer-ready bit (0x80) not set -> no new frame. The controller
     * sets this bit when it has populated 0x814F+ with fresh point
     * data; we must clear it after reading so the next frame can be
     * written. Until then there is no usable data to fetch. */
    if ((status & 0x80) == 0) {
        /* Nothing to clear -- the controller has not flagged a new
         * frame, so writing 0 is a no-op but also wastes an I2C
         * transaction. Just return. */
        return false;
    }

    /* Touch key (only meaningful in ready frames). */
    {
        bool key_down = (status & GT911_STATUS_HAVE_KEY) != 0;
        if (key_down && !s_gt911_key_was_down && s_button_cb) {
            int64_t now = esp_timer_get_time();
            if (now - s_gt911_key_last_fire_us > GT911_KEY_REFIRE_US) {
                s_gt911_key_last_fire_us = now;
                s_button_cb();
            }
        }
        s_gt911_key_was_down = key_down;
    }

    bool pressed = false;
    uint8_t cnt = status & 0x0F;
    if (cnt >= 1 && cnt <= 5) {
        /* Read only the first point (index 0). Each point is 8 bytes:
         *   [0]    track id
         *   [1..2] x (little-endian)
         *   [3..4] y (little-endian)
         *   [5..6] strength (LE, unused)
         *   [7]    reserved
         */
        uint8_t pt[8] = { 0 };
        if (gt911_read_reg(GT911_REG_POINT1, pt, sizeof(pt)) == ESP_OK) {
            int nx = ((int)pt[2] << 8) | pt[1];
            int ny = ((int)pt[4] << 8) | pt[3];
            int lx, ly;
            native_to_logical(nx, ny, &lx, &ly);
            if (out_x) *out_x = lx;
            if (out_y) *out_y = ly;
            pressed = true;
        } else {
            ESP_LOGD(TAG, "gt911 point read failed");
        }
    }

    /* Always clear the buffer-ready flag so the controller can fill
     * the next frame -- even if we decided the frame was invalid. */
    gt911_write_reg(GT911_REG_STATUS, 0x00);
    return pressed;
}

#endif /* CONFIG_DRAFTLING_TOUCH_GT911 */

/* ---- M5Stack Tab5 (BSP-delegated) driver ---- */
#if defined(DRAFTLING_TOUCH_BSP_M5STACK_TAB5)

/* Drain one frame from the BSP-managed touch handle. The BSP wraps
 * either esp_lcd_touch_gt911 (board v1) or esp_lcd_touch_st7123
 * (board v2) -- both expose the same generic esp_lcd_touch_*
 * interface, so this poll function is variant-agnostic. */
static bool poll_bsp_touch(int *out_x, int *out_y)
{
    if (!s_bsp_tp) return false;

    if (esp_lcd_touch_read_data(s_bsp_tp) != ESP_OK) {
        ESP_LOGD(TAG, "bsp touch read_data failed");
        return false;
    }

    uint16_t x = 0, y = 0, strength = 0;
    uint8_t  cnt = 0;
    /* esp_lcd_touch_get_coordinates has a deprecation attribute in
     * the upstream header targeting esp_lcd_touch 2.0; we keep
     * using it because it is the only read API stably available
     * across the 1.x line that the m5stack_tab5 BSP currently
     * pins. The warning is benign. */
    bool pressed = esp_lcd_touch_get_coordinates(
        s_bsp_tp, &x, &y, &strength, &cnt, 1);
    if (!pressed || cnt == 0) return false;

    int lx, ly;
    native_to_logical((int)x, (int)y, &lx, &ly);
#if defined(CONFIG_DRAFTLING_TOUCH_DEBUG_LOG)
    /* Diagnostic log: raw native coordinates from the BSP-managed
     * esp_lcd_touch driver and the logical coordinates we hand to
     * LVGL after native_to_logical(). Used to dial in TOUCH_SWAP_XY /
     * TOUCH_MIRROR_* on new boards. Gated behind
     * DRAFTLING_TOUCH_DEBUG_LOG (off by default) so the logs do not
     * spam the console during normal use. */
    ESP_LOGI(TAG, "bsp touch raw=(%u,%u) cnt=%u -> logical=(%d,%d)",
             (unsigned)x, (unsigned)y, (unsigned)cnt, lx, ly);
#endif
    if (out_x) *out_x = lx;
    if (out_y) *out_y = ly;
    return true;
}

#endif /* DRAFTLING_TOUCH_BSP_M5STACK_TAB5 */

/* Dispatch to the controller-specific poll. */
static bool poll_controller(int *out_x, int *out_y)
{
#if defined(DRAFTLING_TOUCH_BSP_M5STACK_TAB5)
    return poll_bsp_touch(out_x, out_y);
#elif defined(CONFIG_DRAFTLING_TOUCH_GT911)
    return poll_gt911(out_x, out_y);
#elif defined(CONFIG_DRAFTLING_TOUCH_AXS5106L)
    return poll_axs5106l(out_x, out_y);
#else
#  error "No touch controller driver selected (CONFIG_DRAFTLING_TOUCH_*)"
#endif
}

/* INT is a brief per-frame pulse on the AXS15231B (not held low
 * while a finger is down), so we cannot use it to gate the I2C
 * poll: by the time LVGL's read_cb runs the pulse is long gone.
 * We therefore poll on every read_cb invocation and rely on the
 * controller's response (gesture / point-count / zero coords) to
 * decide whether a finger is currently down. The INT GPIO is still
 * used by the standby manager as the EXT0 deep-sleep wake source. */

static void indev_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;

    int x = s_x_latch;
    int y = s_y_latch;
    bool pressed = false;

    if (s_mux && xSemaphoreTake(s_mux, 0) == pdTRUE) {
        pressed = poll_controller(&x, &y);
        if (pressed) {
            s_x_latch = x;
            s_y_latch = y;
        }
        s_pressed_latch = pressed;
        xSemaphoreGive(s_mux);
    } else {
        /* Mutex contended -- report release for this frame; LVGL will
         * call us again next tick. */
        s_pressed_latch = false;
    }

    if (s_pressed_latch) {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = s_x_latch;
        data->point.y = s_y_latch;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
        data->point.x = s_x_latch;
        data->point.y = s_y_latch;
    }
}

extern "C" void touchscreen_init(const touchscreen_config_t *cfg)
{
    if (s_initialized || !cfg) return;

    s_cfg = *cfg;
    s_mux = xSemaphoreCreateMutex();
    if (!s_mux) {
        ESP_LOGE(TAG, "mutex alloc failed");
        return;
    }

#if defined(DRAFTLING_TOUCH_BSP_M5STACK_TAB5)
    /* Tab5 backend: hand off bring-up to the m5stack_tab5 BSP. The
     * BSP probes I2C 0x55 first (ST7123, board v2) and falls back
     * to 0x14 (GT911 backup, board v1), then constructs the matching
     * esp_lcd_touch driver. We do NOT manage rst/intr GPIOs, an I2C
     * bus, or a per-controller I2C device handle on this board -- the
     * BSP owns all of that. s_cfg.{native,logical,mirror,swap,rotate}
     * are still consulted by native_to_logical() in poll_bsp_touch(). */
    esp_err_t err = bsp_touch_new(NULL, &s_bsp_tp);
    if (err != ESP_OK || !s_bsp_tp) {
        ESP_LOGE(TAG, "bsp_touch_new failed: %s", esp_err_to_name(err));
        s_bsp_tp = NULL;
        return;
    }

    s_indev = lv_indev_create();
    if (s_indev) {
        lv_indev_set_type(s_indev, LV_INDEV_TYPE_POINTER);
        lv_indev_set_read_cb(s_indev, indev_read_cb);
        lv_indev_set_display(s_indev, lv_display_get_default());
    } else {
        ESP_LOGW(TAG, "lv_indev_create failed (LVGL not initialized?)");
    }

    s_initialized = true;
    {
        lv_display_t *d = lv_display_get_default();
        int32_t dw = d ? lv_display_get_horizontal_resolution(d) : -1;
        int32_t dh = d ? lv_display_get_vertical_resolution(d) : -1;
        ESP_LOGI(TAG, "Tab5 BSP touch initialized "
                      "(handle=%p, native=%dx%d, logical=%dx%d, "
                      "mirror_x=%d mirror_y=%d swap_xy=%d, "
                      "user_rotate_deg=%d, lvgl_disp=%ldx%ld)",
                 (void *)s_bsp_tp,
                 s_cfg.native_width, s_cfg.native_height,
                 s_cfg.logical_width, s_cfg.logical_height,
                 (int)s_cfg.mirror_x, (int)s_cfg.mirror_y, (int)s_cfg.swap_xy,
                 s_cfg.user_rotate_deg, (long)dw, (long)dh);
    }
    return;
#else

    /* Optional dedicated reset line. (On the JC3248W535 the touch
     * reset is tied to the LCD reset, so this is usually -1.) */
    if (s_cfg.rst >= 0) {
        gpio_config_t g = {};
        g.intr_type    = GPIO_INTR_DISABLE;
        g.mode         = GPIO_MODE_OUTPUT;
        g.pin_bit_mask = (1ULL << s_cfg.rst);
        gpio_config(&g);
        gpio_set_level((gpio_num_t)s_cfg.rst, 0);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level((gpio_num_t)s_cfg.rst, 1);
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    /* INT line as plain input with pull-up. We do *not* attach a
     * GPIO ISR -- LVGL's read_cb polls the level cheaply and it is
     * easier to share the pin with the standby manager's EXT0 wake
     * configuration when we are not contending for the GPIO IRQ.
     *
     * For GT911 this doubles as a gentle wake-from-sleep: the chip,
     * if it was put into low-power mode by a prior touchscreen_sleep()
     * (cmd 0x05 to 0x8040 ahead of deep sleep), exits sleep when it
     * sees INT held HIGH for >5 ms (datasheet section 5.7). When the
     * chip is asleep it does not drive INT, so the host pull-up alone
     * brings the line high and triggers the wake -- with no push-pull
     * contention against the chip's own INT driver once it resumes
     * normal scan mode. We then settle for ~10 ms before issuing any
     * I2C transactions so a sleeping chip has time to come back. */
    if (s_cfg.intr >= 0) {
        gpio_config_t g = {};
        g.intr_type    = GPIO_INTR_DISABLE;
        g.mode         = GPIO_MODE_INPUT;
        g.pin_bit_mask = (1ULL << s_cfg.intr);
        g.pull_up_en   = GPIO_PULLUP_ENABLE;
        g.pull_down_en = GPIO_PULLDOWN_DISABLE;
        gpio_config(&g);
#if defined(CONFIG_DRAFTLING_TOUCH_GT911)
        vTaskDelay(pdMS_TO_TICKS(10));
#endif
    }

    /* I2C master bus + device handle (ESP-IDF v5.x i2c_master API).
     *
     * If the caller passed an existing driver-NG bus handle via
     * s_cfg.i2c_bus we adopt it instead of creating our own. This
     * is the only way to coexist with another driver-NG consumer
     * on the same physical I2C port (currently used by the LilyGO
     * T5 E-Paper S3 Pro / Pro Lite, where vroland/epdiy owns the
     * on-board I2C bus shared with the GT911). */
    if (s_cfg.i2c_bus) {
        s_bus      = (i2c_master_bus_handle_t)s_cfg.i2c_bus;
        s_owns_bus = false;
    } else {
        i2c_master_bus_config_t bus_cfg = {};
        bus_cfg.i2c_port          = s_cfg.i2c_port;
        bus_cfg.sda_io_num        = (gpio_num_t)s_cfg.sda;
        bus_cfg.scl_io_num        = (gpio_num_t)s_cfg.scl;
        bus_cfg.clk_source        = I2C_CLK_SRC_DEFAULT;
        bus_cfg.glitch_ignore_cnt = 7;
        bus_cfg.flags.enable_internal_pullup = true;

        esp_err_t err = i2c_new_master_bus(&bus_cfg, &s_bus);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "i2c_new_master_bus failed: %s", esp_err_to_name(err));
            return;
        }
        s_owns_bus = true;
    }

#if defined(CONFIG_DRAFTLING_TOUCH_GT911)
    /* GT911 selects its I2C address (0x5D or 0x14) based on the INT
     * level seen during its internal power-on reset. On boards where
     * the GT911 RST line is not wired to an ESP32-S3 GPIO (M5Stack
     * PaperS3) we cannot drive the address-select reset sequence,
     * so probe both possible addresses and stick with whichever one
     * acknowledges. The configured value (s_cfg.i2c_addr, typically
     * 0x5D) is tried first; the alternate is the fallback. */
    {
        uint8_t primary = s_cfg.i2c_addr;
        uint8_t alt     = (primary == 0x5D) ? 0x14 :
                          (primary == 0x14) ? 0x5D : primary;
        if (i2c_master_probe(s_bus, primary, 100) != ESP_OK &&
            alt != primary &&
            i2c_master_probe(s_bus, alt, 100) == ESP_OK) {
            ESP_LOGI(TAG, "GT911 responded at 0x%02X (configured 0x%02X)",
                     alt, primary);
            s_cfg.i2c_addr = alt;
        }
    }
#endif

    i2c_device_config_t dev_cfg = {};
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address  = s_cfg.i2c_addr;
    dev_cfg.scl_speed_hz    = s_cfg.i2c_hz ? s_cfg.i2c_hz : 400000;

    esp_err_t err = i2c_master_bus_add_device(s_bus, &dev_cfg, &s_dev);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_master_bus_add_device failed: %s", esp_err_to_name(err));
        /* Only tear down the bus if we created it ourselves --
         * a caller-supplied shared bus is still owned (and used)
         * by the other consumer. */
        if (s_owns_bus) {
            i2c_del_master_bus(s_bus);
        }
        s_bus      = NULL;
        s_owns_bus = false;
        return;
    }

#if defined(CONFIG_DRAFTLING_TOUCH_GT911)
    /* Force the GT911 into normal coordinate-scan mode (cmd 0x00 at
     * register 0x8040) and clear the buffer-ready status register
     * (0x814E). This guarantees the controller is actively scanning
     * regardless of any residual state from before:
     *
     *   * On a fresh cold boot from power-on the chip is already in
     *     normal mode; this write is a benign no-op.
     *   * On wake from deep sleep, touchscreen_sleep() wrote 0x05
     *     (sleep) just before esp_deep_sleep_start(). The chip is
     *     usually POR'd back to default state by epdiy's power-cycle
     *     of the EPD rail during display_init() on the LilyGO T5
     *     E-Paper S3 Pro (the GT911 RST line is routed through the
     *     TPS65185 / PCA9535 expander), so I2C probes succeed --
     *     but on some wakes the chip ends up in a state where it
     *     ACKs reads yet never sets the buffer-ready bit. Writing
     *     0x00 (Read coord state) here resumes coordinate scanning
     *     definitively. Failure is logged but not fatal: the runtime
     *     gt911_try_recover() path in poll_gt911() will retry if the
     *     chip remains unresponsive. */
    (void)gt911_write_reg(GT911_REG_COMMAND, 0x00);
    (void)gt911_write_reg(GT911_REG_STATUS,  0x00);
#endif

    /* Register LVGL pointer indev. lv_init() was already called by
     * draftling_lvgl_port_init(); we just attach the new device to the default
     * display. */
    s_indev = lv_indev_create();
    if (s_indev) {
        lv_indev_set_type(s_indev, LV_INDEV_TYPE_POINTER);
        lv_indev_set_read_cb(s_indev, indev_read_cb);
        lv_indev_set_display(s_indev, lv_display_get_default());
    } else {
        ESP_LOGW(TAG, "lv_indev_create failed (LVGL not initialized?)");
    }

    s_initialized = true;
#if defined(CONFIG_DRAFTLING_TOUCH_GT911)
    const char *ctrl = "GT911";
#elif defined(CONFIG_DRAFTLING_TOUCH_AXS5106L)
    const char *ctrl = "AXS5106L";
#else
    const char *ctrl = "?";
#endif
    ESP_LOGI(TAG, "%s touchscreen initialized "
                  "(I2C addr=0x%02X, INT=%d, native=%dx%d, logical=%dx%d, "
                  "mirror_x=%d mirror_y=%d swap_xy=%d)",
             ctrl,
             s_cfg.i2c_addr, s_cfg.intr,
             s_cfg.native_width, s_cfg.native_height,
             s_cfg.logical_width, s_cfg.logical_height,
             (int)s_cfg.mirror_x, (int)s_cfg.mirror_y, (int)s_cfg.swap_xy);
#endif /* DRAFTLING_TOUCH_BSP_M5STACK_TAB5 */
}

extern "C" void touchscreen_set_button_callback(touchscreen_button_cb_t cb)
{
#if defined(CONFIG_DRAFTLING_TOUCH_GT911)
    s_button_cb = cb;
#else
    /* Only the GT911 backend has a hardware touch key. */
    (void)cb;
#endif
}

extern "C" bool touchscreen_is_initialized(void)
{
    return s_initialized;
}

extern "C" int touchscreen_get_int_gpio(void)
{
    if (!s_initialized) return -1;
    return s_cfg.intr;
}

extern "C" bool touchscreen_read(int *out_x, int *out_y)
{
    if (!s_initialized || !s_mux) return false;
    bool pressed = false;
    if (xSemaphoreTake(s_mux, pdMS_TO_TICKS(20)) == pdTRUE) {
        pressed = poll_controller(out_x ? out_x : NULL,
                                  out_y ? out_y : NULL);
        if (pressed) {
            if (out_x) s_x_latch = *out_x;
            if (out_y) s_y_latch = *out_y;
        }
        s_pressed_latch = pressed;
        xSemaphoreGive(s_mux);
    }
    if (!pressed) {
        if (out_x) *out_x = s_x_latch;
        if (out_y) *out_y = s_y_latch;
    }
    return pressed;
}

extern "C" bool touchscreen_is_pressed(void)
{
    return s_pressed_latch;
}

extern "C" void touchscreen_sleep(void)
{
    if (!s_initialized) return;
#if defined(DRAFTLING_TOUCH_BSP_M5STACK_TAB5)
    if (!s_bsp_tp) return;
    /* esp_lcd_touch_enter_sleep dispatches to the BSP-selected
     * controller's vendor sleep sequence (GT911 cmd 0x05 on v1,
     * ST7123 equivalent on v2). Take the mutex so we don't race
     * a concurrent poll. */
    if (s_mux && xSemaphoreTake(s_mux, pdMS_TO_TICKS(50)) == pdTRUE) {
        esp_err_t err = esp_lcd_touch_enter_sleep(s_bsp_tp);
        xSemaphoreGive(s_mux);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Tab5 touch entered sleep mode");
        } else {
            ESP_LOGW(TAG, "Tab5 touch sleep cmd failed: %s",
                     esp_err_to_name(err));
        }
    }
#elif defined(CONFIG_DRAFTLING_TOUCH_GT911)
    if (!s_dev) return;
    /* GT911 datasheet, section 6 (command register 0x8040):
     * writing 0x05 puts the controller into sleep mode (typical
     * standby current < 10 uA). It wakes when the INT pin is
     * driven high by the host, or on the next reset. We hold the
     * mutex so we don't race a concurrent poll_controller(). */
    if (s_mux && xSemaphoreTake(s_mux, pdMS_TO_TICKS(50)) == pdTRUE) {
        esp_err_t err = gt911_write_reg(GT911_REG_COMMAND, GT911_CMD_SLEEP);
        xSemaphoreGive(s_mux);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "GT911 entered sleep mode");
        } else {
            ESP_LOGW(TAG, "GT911 sleep cmd failed: %s",
                     esp_err_to_name(err));
        }
    }
#else
    /* AXS5106L has no documented sleep command in the public
     * register map; it idles to ~100 uA on its own once polling
     * stops, which is acceptable on the Tab5 (separate rail). */
#endif
}

#else /* !CONFIG_DRAFTLING_TOUCHSCREEN */

/* Provide weak stubs so callers can compile-and-link without
 * #ifdef'ing every call site. The header still hides the
 * configuration struct behind the same Kconfig guard, so these are
 * only reached via translation units that explicitly check for
 * touchscreen support; keeping them keeps the link graph simple. */

#include "touchscreen.h"

extern "C" void touchscreen_init(const touchscreen_config_t *cfg) { (void)cfg; }
extern "C" void touchscreen_set_button_callback(touchscreen_button_cb_t cb) { (void)cb; }
extern "C" bool touchscreen_is_initialized(void) { return false; }
extern "C" int  touchscreen_get_int_gpio(void)    { return -1; }
extern "C" bool touchscreen_read(int *x, int *y)  { (void)x; (void)y; return false; }
extern "C" bool touchscreen_is_pressed(void)      { return false; }
extern "C" void touchscreen_sleep(void)           { }

#endif /* CONFIG_DRAFTLING_TOUCHSCREEN */
