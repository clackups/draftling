/*
 * Power-latch + PWR-button driver. See include/power.h for the
 * public API.
 *
 * Architecture
 * ------------
 * The TCA9554 IO-expander is the only device this code talks to over
 * I2C, so we open a dedicated I2C master bus on the pins provided
 * via power_config_t (the same ESP-IDF v5.x i2c_master API the
 * touchscreen component uses, which avoids pulling in a heavy
 * esp_io_expander_tca9554 dependency for the two writes we need to
 * make).
 *
 * TCA9554 register layout (only the two writes we issue):
 *   0x01  Output Port      -- HIGH bit = pin high
 *   0x03  Configuration    -- 0 = output, 1 = input  (POR default = all-input)
 *
 * Sequence in power_init():
 *   1. Set up the PWR-button GPIO and start its polling timer
 *      UNCONDITIONALLY -- the button must work even if the TCA9554
 *      I2C transaction fails (and an early bring-up bug where we
 *      bailed out of init on I2C error meant the button was
 *      silently disarmed).
 *   2. Best-effort TCA9554 init:
 *        a. Open I2C bus and add device.
 *        b. Write Output : latch_bit = 1 (so the pin goes HIGH as
 *                          soon as it becomes an output).
 *        c. Write Config : latch_bit = 0 (output), every other bit
 *                          = 1 (input). POR default is all-input
 *                          so a single write is enough.
 *      If any step fails we log a warning and leave s_dev = NULL.
 *      power_off() then becomes a no-op (useful on USB power where
 *      there is no battery rail to cut anyway).
 *
 * Sequence in power_off():
 *   1. Write Output : latch_bit = 0 -> rail drops out. No-op when
 *                     the TCA9554 init failed.
 *
 * PWR-button polling
 * ------------------
 * A 50 ms periodic esp_timer reads the PWR button GPIO (configured
 * as input with internal pull-up enabled). Consecutive samples that
 * read LOW grow a press counter; when it reaches POWER_LONG_PRESS_MS
 * we invoke the registered long-press callback (typically
 * standby_enter_sleep, which runs the editor's pre-sleep auto-save,
 * cuts the backlight, cuts the latch via power_off() and finally
 * enters deep sleep). Releasing the button before the threshold
 * resets the counter. We deliberately do not act on a short press:
 * the user uses short presses to *power on* the board (which is
 * handled by hardware, before our code even runs), so short presses
 * while powered should be a no-op.
 */
#include "sdkconfig.h"
#include "power.h"

#if !defined(CONFIG_DRAFTLING_HAS_POWER_LATCH)
/* Stubs for boards without a power latch. */

extern "C" int  power_init(const power_config_t *cfg)            { (void)cfg; return 0; }
extern "C" void power_set_long_press_cb(power_long_press_cb_t cb){ (void)cb; }
extern "C" void power_off(void)                                  { /* no-op */ }

#else /* CONFIG_DRAFTLING_HAS_POWER_LATCH */

#include <cstring>
#include <esp_log.h>
#include <esp_err.h>
#include <esp_timer.h>
#include <driver/gpio.h>
#include <driver/i2c_master.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *TAG = "Power";

/* TCA9554 register addresses. */
#define TCA9554_REG_OUTPUT  0x01
#define TCA9554_REG_CONFIG  0x03

/* Polling period for the PWR button. 50 ms is fast enough that the
 * worst-case latency before we notice a press / release is
 * imperceptible, and slow enough that the polling cost is
 * negligible. */
#define POWER_BUTTON_POLL_MS 50

static power_config_t          s_cfg = {};
static i2c_master_bus_handle_t s_bus = NULL;
static i2c_master_dev_handle_t s_dev = NULL;
static esp_timer_handle_t      s_btn_timer = NULL;
static power_long_press_cb_t   s_long_press_cb = NULL;
static int                     s_press_ms = 0;     /* accumulated hold time */
static bool                    s_long_press_fired = false;

/* Drive the latch pin to `level` (0 = LOW, 1 = HIGH). The other 7
 * expander pins are left as inputs by power_init(); writing the
 * Output register without touching the Config register is therefore
 * safe (the value latches but has no electrical effect on the input
 * pins). */
static esp_err_t write_latch(int level)
{
    if (!s_dev) return ESP_ERR_INVALID_STATE;
    uint8_t buf[2] = {
        TCA9554_REG_OUTPUT,
        (uint8_t)((level ? 1 : 0) << s_cfg.latch_bit),
    };
    return i2c_master_transmit(s_dev, buf, sizeof(buf), 100);
}

/* Configure the latch pin as output, every other pin as input. The
 * POR default for the Config register is all-input (0xFF); writing
 * it once with just the latch bit cleared is sufficient. */
static esp_err_t write_config_latch_output(void)
{
    if (!s_dev) return ESP_ERR_INVALID_STATE;
    uint8_t buf[2] = {
        TCA9554_REG_CONFIG,
        (uint8_t)(0xFF & ~(1U << s_cfg.latch_bit)),
    };
    return i2c_master_transmit(s_dev, buf, sizeof(buf), 100);
}

/* PWR-button polling timer callback. */
static void btn_poll_cb(void *arg)
{
    (void)arg;
    int level = gpio_get_level((gpio_num_t)s_cfg.pwr_button_gpio);
    if (level == 0) {
        /* Button is being held down. */
        s_press_ms += POWER_BUTTON_POLL_MS;
        if (!s_long_press_fired && s_press_ms >= POWER_LONG_PRESS_MS) {
            s_long_press_fired = true;
            ESP_LOGI(TAG, "PWR long press (%d ms) -- shutting down",
                     s_press_ms);
            if (s_long_press_cb) {
                /* Typically wired to standby_enter_sleep(), which
                 * runs the full shutdown sequence (auto-save +
                 * BL cut + latch cut + deep sleep). The cb may
                 * never return (deep sleep); if it does, we fall
                 * through and the next poll tick resets state. */
                s_long_press_cb();
            } else {
                /* Fallback for callers who did not wire the hook:
                 * at least try to cut the rail directly. On USB
                 * this is a no-op, but on battery it powers the
                 * board off cleanly. */
                ESP_LOGW(TAG, "No long-press cb registered -- "
                              "calling power_off() directly");
                power_off();
            }
        }
    } else {
        /* Button released -- reset the counter so the next press
         * starts from zero. We also clear the latched flag so a
         * second long press after this one still fires. */
        s_press_ms = 0;
        s_long_press_fired = false;
    }
}

/* Best-effort TCA9554 latch init. Logs a warning on failure and
 * leaves s_dev = NULL so power_off() becomes a no-op. Failures
 * are non-fatal: on USB power the latch has no electrical effect
 * anyway, and the PWR-button polling (set up separately) keeps
 * working. */
static void try_latch_init(void)
{
    /* I2C master bus on the dedicated TCA9554 pins.
     *
     * Pin to I2C port 1 explicitly. The touchscreen component
     * unconditionally requests port 0 (see main.cpp), and on the
     * ESP32-S3 there are only two I2C controllers. If we leave this
     * at -1 ("auto-pick a free port"), the driver picks port 0
     * here (since power_init runs first) and the subsequent
     * touchscreen_init() fails with "port already in use" -- the
     * touch screen then silently stops working. Hard-coding port 1
     * keeps both bring-ups happy. */
    i2c_master_bus_config_t bus_cfg = {};
    bus_cfg.i2c_port          = 1;
    bus_cfg.sda_io_num        = (gpio_num_t)s_cfg.i2c_sda;
    bus_cfg.scl_io_num        = (gpio_num_t)s_cfg.i2c_scl;
    bus_cfg.clk_source        = I2C_CLK_SRC_DEFAULT;
    bus_cfg.glitch_ignore_cnt = 7;
    bus_cfg.flags.enable_internal_pullup = true;

    esp_err_t err = i2c_new_master_bus(&bus_cfg, &s_bus);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "i2c_new_master_bus failed: %s -- latch disabled",
                 esp_err_to_name(err));
        s_bus = NULL;
        return;
    }

    i2c_device_config_t dev_cfg = {};
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address  = s_cfg.tca9554_addr;
    dev_cfg.scl_speed_hz    = 300000;

    err = i2c_master_bus_add_device(s_bus, &dev_cfg, &s_dev);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "i2c_master_bus_add_device failed: %s -- latch disabled",
                 esp_err_to_name(err));
        i2c_del_master_bus(s_bus);
        s_bus = NULL;
        s_dev = NULL;
        return;
    }

    /* Drive the latch HIGH (rail held on) before changing its
     * direction to output, so the pin never glitches LOW mid-init
     * and brown-outs us off battery. */
    err = write_latch(1);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "TCA9554 output write failed: %s -- latch disabled",
                 esp_err_to_name(err));
        s_dev = NULL;   /* mark latch unusable; bus left open */
        return;
    }
    err = write_config_latch_output();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "TCA9554 config write failed: %s -- latch disabled",
                 esp_err_to_name(err));
        s_dev = NULL;
        return;
    }
    ESP_LOGI(TAG, "Power latch closed via TCA9554 IO%d (addr 0x%02X)",
             (int)s_cfg.latch_bit, (unsigned)s_cfg.tca9554_addr);
}

extern "C" int power_init(const power_config_t *cfg)
{
    if (s_btn_timer) return 0;  /* already initialized */
    if (!cfg || cfg->pwr_button_gpio < 0 || cfg->i2c_sda < 0 || cfg->i2c_scl < 0) {
        ESP_LOGE(TAG, "power_init: invalid config");
        return -1;
    }
    s_cfg = *cfg;

    /* ---- 1. PWR-button GPIO + polling timer (always succeeds) ----
     *
     * Set this up BEFORE the I2C latch init: a failure on the I2C
     * side must not prevent the button from working. This was the
     * original bring-up bug: a TCA9554 transmit error left the
     * button disarmed and the user could no longer power off the
     * device. */
    gpio_config_t g = {};
    g.intr_type    = GPIO_INTR_DISABLE;
    g.mode         = GPIO_MODE_INPUT;
    g.pin_bit_mask = (1ULL << s_cfg.pwr_button_gpio);
    g.pull_up_en   = GPIO_PULLUP_ENABLE;
    g.pull_down_en = GPIO_PULLDOWN_DISABLE;
    if (gpio_config(&g) != ESP_OK) {
        ESP_LOGW(TAG, "PWR button GPIO%d config failed", s_cfg.pwr_button_gpio);
    }

    esp_timer_create_args_t args = {};
    args.callback = btn_poll_cb;
    args.name     = "pwr_btn";
    esp_err_t err = esp_timer_create(&args, &s_btn_timer);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_timer_create failed: %s", esp_err_to_name(err));
        return -1;
    }
    err = esp_timer_start_periodic(s_btn_timer,
                                   (uint64_t)POWER_BUTTON_POLL_MS * 1000ULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_timer_start_periodic failed: %s", esp_err_to_name(err));
        return -1;
    }
    ESP_LOGI(TAG, "PWR button GPIO%d armed (long-press = %d ms)",
             s_cfg.pwr_button_gpio, POWER_LONG_PRESS_MS);

    /* ---- 2. TCA9554 latch (best-effort) ---- */
    try_latch_init();

    return 0;
}

extern "C" void power_set_long_press_cb(power_long_press_cb_t cb)
{
    s_long_press_cb = cb;
}

extern "C" void power_off(void)
{
    if (!s_dev) {
        ESP_LOGI(TAG, "power_off(): no latch available -- skipping");
        return;
    }
    ESP_LOGI(TAG, "Cutting power latch -- bye");

    /* Drive the latch LOW. On battery the 3V3 rail collapses
     * within a few ms and we are gone before this function
     * returns. On USB the latch has no electrical effect; we
     * return so the caller can fall through to a regular
     * esp_deep_sleep_start(). */
    esp_err_t err = write_latch(0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "TCA9554 output write failed: %s", esp_err_to_name(err));
    }

    /* Brief wait so the rail has time to drop on battery before
     * the caller proceeds. On USB this is harmless. */
    vTaskDelay(pdMS_TO_TICKS(100));
}

#endif /* CONFIG_DRAFTLING_HAS_POWER_LATCH */
