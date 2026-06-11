/*
 * Standby / deep-sleep manager.
 *
 * Tracks user inactivity via an esp_timer. When the configured
 * timeout expires the board enters one of three power-save modes,
 * picked at build time:
 *
 *   * Default (deep sleep + button wake):
 *     esp_deep_sleep_start(); wake via EXT0 on
 *     CONFIG_DRAFTLING_WAKEUP_GPIO (the per-board BOOT button).
 *     On targets that have no EXT0 support (ESP32-P4) and no
 *     LP_IO-capable user input, this falls back to RESET-only
 *     wake (cold boot, autosave restore).
 *
 *   * CONFIG_DRAFTLING_STANDBY_WAKE_ON_TOUCH (deep sleep + touch wake):
 *     same as above but EXT0 is armed on the touchscreen INT pin
 *     reported by touchscreen_get_int_gpio(), so any tap wakes the
 *     device. Preferred on boards with no user buttons.
 *
 *   * CONFIG_DRAFTLING_STANDBY_DISPLAY_OFF (display off, MCU
 *     running): blank the display via display_sleep() and block in
 *     a low-priority FreeRTOS poll loop until any input arrives
 *     (touch INT low, or BLE keyboard press), then display_wake()
 *     and return without losing editor state. Draws more power than
 *     deep sleep but works on hardware with no RTC-capable wake
 *     source.
 *
 * This file is otherwise board-agnostic. The editor state lives in
 * PSRAM/heap and IS preserved across the display-off path; deep
 * sleep loses it (callers should auto-save first). The timeout
 * value is persisted in NVS.
 */

#include <cstring>
#include <cinttypes>
#include <esp_log.h>
#include <esp_sleep.h>
#include <esp_timer.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <soc/soc_caps.h>
#include "sdkconfig.h"

#include "standby.h"
#include "ble_keyboard.h"
#include "display.h"
#include "power.h"
#if defined(CONFIG_DRAFTLING_TOUCHSCREEN)
#include "touchscreen.h"
#endif
#if defined(CONFIG_DRAFTLING_HAS_USB_HOST)
#include "usb_kbd.h"
#endif

static const char *TAG = "Standby";

/* NVS namespace and key for the timeout value */
#define NVS_NAMESPACE  "standby"
#define NVS_KEY_TOUT   "timeout"

/* GPIO used as EXT0 wake-up source (active-low). The numeric value
 * is selected per board in main/Kconfig.projbuild via the hidden
 * DRAFTLING_WAKEUP_GPIO symbol so the standby code itself stays
 * board-agnostic. */
#define WAKEUP_GPIO    ((gpio_num_t)CONFIG_DRAFTLING_WAKEUP_GPIO)

static uint32_t s_timeout_sec = STANDBY_DEFAULT_TIMEOUT_SEC;
static esp_timer_handle_t s_timer = NULL;
static standby_pre_sleep_cb_t s_pre_sleep_cb = NULL;

/* "No keyboard connected" countdown. Armed in standby_init() from
 * CONFIG_DRAFTLING_NO_KEYBOARD_SLEEP_SEC. When it fires it checks
 * ble_keyboard_is_connected() and only enters deep sleep if no
 * keyboard is paired by then. Independent of the regular inactivity
 * timer above so a long inactivity timeout does not stop us from
 * sleeping when no keyboard is around at all. We poll the BLE state
 * (rather than registering a connect callback) because the BLE
 * connect-callback slot is already used by the editor UI. */
static esp_timer_handle_t s_kb_wait_timer = NULL;

#if defined(CONFIG_DRAFTLING_STANDBY_DISPLAY_OFF)
/* Forward declaration -- the display-off wait loop is implemented
 * further down, but standby_reset_timer() needs to flip this flag
 * to wake the loop when a BLE key event arrives. */
static volatile bool s_display_off_wake_req = false;
#endif

/* ---- timer callback ---- */

static void inactivity_cb(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "Inactivity timeout reached -- entering deep sleep");
    standby_enter_sleep();
}

#if CONFIG_DRAFTLING_NO_KEYBOARD_SLEEP_SEC > 0 && defined(CONFIG_DRAFTLING_HAS_BATTERY)
static void kb_wait_cb(void *arg)
{
    (void)arg;
    if (ble_keyboard_is_connected()) {
        ESP_LOGI(TAG, "No-keyboard timer fired but a BLE keyboard is connected -- staying awake");
        return;
    }
#if defined(CONFIG_DRAFTLING_HAS_USB_HOST)
    if (usb_kbd_is_connected()) {
        ESP_LOGI(TAG, "No-keyboard timer fired but a USB keyboard is connected -- staying awake");
        return;
    }
#endif
    ESP_LOGI(TAG, "No keyboard connected within %d s -- entering deep sleep",
             CONFIG_DRAFTLING_NO_KEYBOARD_SLEEP_SEC);
    standby_enter_sleep();
}
#endif

/* ---- helpers ---- */

static void start_timer(void)
{
    if (!s_timer || s_timeout_sec == 0) return;
    /* (Re)start one-shot timer */
    esp_timer_stop(s_timer);   /* ignore error if not running */
    esp_timer_start_once(s_timer, (uint64_t)s_timeout_sec * 1000000ULL);
}

static void stop_timer(void)
{
    if (s_timer) esp_timer_stop(s_timer);
}

/* ---- public API ---- */

extern "C" void standby_init(void)
{
    /* Load timeout from NVS (default if not found) */
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &h) == ESP_OK) {
        uint32_t val = 0;
        if (nvs_get_u32(h, NVS_KEY_TOUT, &val) == ESP_OK) {
            s_timeout_sec = val;
        }
        nvs_close(h);
    }

    /* Create the one-shot inactivity timer */
    esp_timer_create_args_t args = {};
    args.callback = inactivity_cb;
    args.name = "standby";
    ESP_ERROR_CHECK(esp_timer_create(&args, &s_timer));

    /* Start the timer if timeout is non-zero */
    if (s_timeout_sec > 0) {
        start_timer();
    }

    /* Arm the "no keyboard connected" deep-sleep countdown. When the
     * timer fires kb_wait_cb checks ble_keyboard_is_connected() and
     * only sleeps if no keyboard is paired by then. A value of 0
     * disables the feature (keep scanning indefinitely).
     *
     * The entire purpose of this countdown is to conserve battery on
     * portable boards that may have been powered on accidentally or
     * out of range of their paired keyboard. On USB-powered dev
     * boards with no battery (Guition JC3248W535) deep-sleeping
     * after 3 minutes of no keyboard just blanks the display
     * unexpectedly while the user is still setting things up, so we
     * gate the arming on CONFIG_DRAFTLING_HAS_BATTERY. */
#if CONFIG_DRAFTLING_NO_KEYBOARD_SLEEP_SEC > 0 && defined(CONFIG_DRAFTLING_HAS_BATTERY)
    {
        esp_timer_create_args_t kb_args = {};
        kb_args.callback = kb_wait_cb;
        kb_args.name = "standby_kb";
        ESP_ERROR_CHECK(esp_timer_create(&kb_args, &s_kb_wait_timer));
        esp_timer_start_once(s_kb_wait_timer,
            (uint64_t)CONFIG_DRAFTLING_NO_KEYBOARD_SLEEP_SEC * 1000000ULL);
        ESP_LOGI(TAG, "No-keyboard sleep armed (%d s)",
                 CONFIG_DRAFTLING_NO_KEYBOARD_SLEEP_SEC);
    }
#else
    ESP_LOGI(TAG, "No-keyboard sleep disabled "
                  "(no battery on this board, or timer = 0)");
#endif

    ESP_LOGI(TAG, "Standby initialized, timeout=%" PRIu32 " s", s_timeout_sec);
}

extern "C" void standby_reset_timer(void)
{
#if defined(CONFIG_DRAFTLING_STANDBY_DISPLAY_OFF)
    /* If the display is currently off, signal the wait loop to
     * exit. Whichever code path delivered this activity (BLE key,
     * touch, "Sleep now" cancelled) will then return through
     * enter_display_off and continue normally. */
    s_display_off_wake_req = true;
#endif
    if (s_timeout_sec > 0) {
        start_timer();
    }
}

extern "C" uint32_t standby_get_timeout(void)
{
    return s_timeout_sec;
}

extern "C" void standby_set_timeout(uint32_t seconds)
{
    s_timeout_sec = seconds;

    /* Persist to NVS */
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_u32(h, NVS_KEY_TOUT, seconds);
        nvs_commit(h);
        nvs_close(h);
    }

    if (seconds == 0) {
        stop_timer();
    } else {
        start_timer();
    }

    ESP_LOGI(TAG, "Timeout set to %" PRIu32 " s", seconds);
}

extern "C" void standby_set_pre_sleep_cb(standby_pre_sleep_cb_t cb)
{
    s_pre_sleep_cb = cb;
}

/* Pick the EXT0 wake GPIO. By default this is the per-board BOOT
 * button from Kconfig; when wake-on-touch is enabled and the
 * touchscreen driver has been initialised with an INT pin, that
 * INT pin overrides the default. */
static gpio_num_t resolve_wake_gpio(void)
{
    gpio_num_t g = WAKEUP_GPIO;
#if defined(CONFIG_DRAFTLING_STANDBY_WAKE_ON_TOUCH) && defined(CONFIG_DRAFTLING_TOUCHSCREEN)
    int tg = touchscreen_get_int_gpio();
    if (tg >= 0 && rtc_gpio_is_valid_gpio((gpio_num_t)tg)) {
        g = (gpio_num_t)tg;
    } else if (tg >= 0) {
        ESP_LOGW(TAG, "Touch INT GPIO%d is not RTC-capable; "
                      "falling back to GPIO%d for EXT0 wake",
                 tg, (int)WAKEUP_GPIO);
    }
#endif
    return g;
}

#if defined(CONFIG_DRAFTLING_STANDBY_DISPLAY_OFF)

/* Display-off standby: blank the display and idle the MCU until any
 * input arrives. Implemented as a busy wait inline on the caller's
 * task (the only caller is standby_enter_sleep, invoked from either
 * the LVGL timer callback or the inactivity esp_timer). The poll
 * loop checks:
 *   - touch INT GPIO low (any tap), or
 *   - BLE keyboard key event (ble_keyboard_is_connected -> reset
 *     timer in editor_ui resumes us via standby_reset_timer)
 * To keep this simple we poll only the touch INT and rely on
 * standby_reset_timer() being called from the BLE callback path
 * to invoke standby_display_off_wake() externally; in practice
 * the polling loop returns as soon as a touch is detected, which
 * is the primary input on display-off boards. */

static void enter_display_off(void)
{
    ESP_LOGI(TAG, "Standby: turning display off");
    if (s_pre_sleep_cb) {
        s_pre_sleep_cb();
    }
    display_sleep();

    s_display_off_wake_req = false;

#if defined(CONFIG_DRAFTLING_TOUCHSCREEN)
    int int_gpio = touchscreen_get_int_gpio();
#else
    int int_gpio = -1;
#endif

    /* Block until we see a wake signal. Poll every 100 ms; that is
     * fast enough to feel instant after a tap (a touch frame is
     * 10-20 ms) without burning the CPU. */
    while (!s_display_off_wake_req) {
        if (int_gpio >= 0 &&
            gpio_get_level((gpio_num_t)int_gpio) == 0) {
            break;
        }
        if (ble_keyboard_is_connected()) {
            /* BLE keyboard events route through editor_ui's
             * key handler which calls standby_reset_timer(); that
             * function flips s_display_off_wake_req. */
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    ESP_LOGI(TAG, "Standby: waking display");
    display_wake();
    /* Rearm the inactivity timer so we go back to sleep after the
     * user finishes interacting. */
    start_timer();
}

#endif /* CONFIG_DRAFTLING_STANDBY_DISPLAY_OFF */

extern "C" void standby_enter_sleep(void)
{
    stop_timer();
    if (s_kb_wait_timer) {
        esp_timer_stop(s_kb_wait_timer);
    }

#if defined(CONFIG_DRAFTLING_STANDBY_DISPLAY_OFF)
    enter_display_off();
    return;
#else

    if (s_pre_sleep_cb) {
        s_pre_sleep_cb();
    }

    /* Cut the backlight (BL pin driven LOW + held through deep
     * sleep) so the panel goes visually dark and the BL boost
     * circuit stops drawing current. We deliberately leave the
     * panel controller itself in its normal display-on state --
     * see display_deep_sleep_prepare() in display_axs15231b.cpp
     * for the rationale. No-op on reflective / e-paper backends
     * that have no backlight. */
    display_deep_sleep_prepare();

#if defined(CONFIG_DRAFTLING_HAS_POWER_LATCH)
    /* On boards with a hardware power latch (Waveshare Touch-LCD-
     * 3.49) the cleanest "deep sleep" is a full power-off: cut the
     * battery rail via the TCA9554 latch and let the user re-apply
     * VBAT with the PWR button to cold-boot. This is also the only
     * way to reliably extinguish the LCD on this board -- the
     * controller and BL boost circuit are downstream of the latch,
     * so as long as the rail is up the panel keeps drawing power
     * and may still show its last frame.
     *
     * power_off() returns when there is no battery to cut (USB-only
     * supply), in which case we fall through to the usual deep
     * sleep below so the chip still saves what little power it can. */
    ESP_LOGI(TAG, "Cutting power latch (battery shutdown)...");
    power_off();
    ESP_LOGI(TAG, "Power latch had no effect (USB power?) -- "
                  "falling back to deep sleep");
#endif

    gpio_num_t wake_gpio = resolve_wake_gpio();

#if defined(CONFIG_DRAFTLING_MODEL_LILYGO_T5_EPD_S3_PRO_H752)
    /* Original H752: the only user key is GPIO48, which is not an
     * RTC IO on the ESP32-S3 (EXT0/EXT1 cover GPIO0-21 only), so a
     * deep-sleeping chip could only be revived with the RESET
     * button. Mirror the known-good reader firmware
     * (t5s3-reader HalGPIO::startDeepSleep, BOARD_T5S3_H752 branch):
     * enter LIGHT sleep with a digital GPIO wake on the key, then
     * restart so the firmware cold-boots exactly as it would after
     * a deep-sleep wake. Light sleep idles at a few mA rather than
     * deep-sleep microamps, but it is the only configuration in
     * which the key can turn this hardware revision back on. */
    {
        gpio_config_t g = {};
        g.intr_type    = GPIO_INTR_DISABLE;
        g.mode         = GPIO_MODE_INPUT;
        g.pin_bit_mask = 1ULL << wake_gpio;
        g.pull_up_en   = GPIO_PULLUP_ENABLE;
        g.pull_down_en = GPIO_PULLDOWN_DISABLE;
        gpio_config(&g);

        /* If the key is still held from the press that triggered
         * sleep, wait for release so the LOW level does not wake us
         * straight back up. Cap the wait; a stuck key should not
         * block sleep forever. */
        int waited_ms = 0;
        while (gpio_get_level(wake_gpio) == 0 && waited_ms < 2000) {
            vTaskDelay(pdMS_TO_TICKS(20));
            waited_ms += 20;
        }

        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
        gpio_wakeup_enable(wake_gpio, GPIO_INTR_LOW_LEVEL);
        esp_sleep_enable_gpio_wakeup();
        ESP_LOGI(TAG, "Entering light sleep, wake on GPIO%d low...",
                 (int)wake_gpio);
        esp_light_sleep_start();
        gpio_wakeup_disable(wake_gpio);
        esp_restart();
        /* Does not return */
    }
#endif /* CONFIG_DRAFTLING_MODEL_LILYGO_T5_EPD_S3_PRO_H752 */

#if SOC_PM_SUPPORT_EXT0_WAKEUP
    /* EXT0's level comparator and the RTC IO pull-up we are about to
     * enable both live in the RTC_PERIPH power domain. A pre-sleep
     * callback may have called esp_sleep_pd_config(RTC_PERIPH, OFF)
     * to shave a few microamps -- on the LilyGO T5 E-Paper S3 Pro
     * this is set in pre_sleep_t5_deinit(). In theory IDF auto-
     * promotes the domain back to ON when EXT0 is armed, but on
     * ESP32-S3 with CONFIG_ESP_SLEEP_GPIO_RESET_WORKAROUND active we
     * have observed the chip wake immediately from deep sleep with
     * wakeup_cause=EXT0 in that configuration even when the wake
     * pin reads stable HIGH for many ms before arming. The most
     * plausible explanation is that the pull-up momentarily lifts
     * during the sleep-entry transition and EXT0 latches a strapping
     * pin (e.g. BOOT / GPIO0) low. Force the domain ON here to make
     * the intent explicit and pin the bias rails up across entry. */
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);

    /* Enable the internal RTC pull-up on the wake pin and disable any
     * pull-down so the EXT0 wake-on-low source does not fire
     * immediately. The supported buttons (RLCD-4.2 button on GPIO18
     * and PaperS3 BOOT on GPIO0 / strapping pin) already have
     * external pull-ups, so adding an internal pull-up here is
     * harmless: the two pull-ups simply parallel. We always enable
     * it so the behaviour is consistent across boards.
     *
     * On wake-on-touch boards (CONFIG_DRAFTLING_STANDBY_WAKE_ON_TOUCH)
     * the same pull-up keeps the AXS5106L's open-drain INT line
     * idle-high while no touch is active, so EXT0 only fires when
     * the controller actually pulls INT low.
     *
     * rtc_gpio_pullup_en() routes through the RTC IO mux, so the
     * pull-up survives into deep sleep. Errors are logged but not
     * fatal: a non-RTC-capable GPIO will be rejected here, and we
     * still want to attempt deep sleep with whatever wake-up
     * configuration we can make work. */
    if (rtc_gpio_is_valid_gpio(wake_gpio)) {
        esp_err_t err = rtc_gpio_pullup_en(wake_gpio);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "rtc_gpio_pullup_en(GPIO%d) failed: %s",
                     (int)wake_gpio, esp_err_to_name(err));
        }
        err = rtc_gpio_pulldown_dis(wake_gpio);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "rtc_gpio_pulldown_dis(GPIO%d) failed: %s",
                     (int)wake_gpio, esp_err_to_name(err));
        }
    } else {
        ESP_LOGW(TAG, "GPIO%d is not RTC-capable; cannot enable internal pull-up",
                 (int)wake_gpio);
    }

    /* Wait for the wake pin to actually settle HIGH before arming
     * EXT0. Without this, we have observed boards (LilyGO T5 E-Paper
     * S3 Pro) waking immediately from deep sleep with
     * reset_reason=ESP_RST_DEEPSLEEP + wakeup_cause=EXT0 on the very
     * first boot line, even though no button was pressed and no
     * touch occurred. The cause is that the wake pin is briefly LOW
     * at the moment esp_sleep_enable_ext0_wakeup() is called -- e.g.
     * the BOOT-button net dipping while the EPD power rail collapses
     * during display_deep_sleep_prepare(), a still-held button, or
     * a touch controller INT line that has not released yet on
     * wake-on-touch boards. EXT0 is a level detector, so any LOW
     * sample at arm time latches the wake source.
     *
     * Poll the pad every 20 ms and require 3 consecutive HIGH reads
     * (~60 ms quiet window) before we arm EXT0. Cap the total wait
     * at 500 ms; if the pin is still LOW past that we skip EXT0
     * arming entirely and fall through to RESET-only wake -- far
     * better than burning the battery in a wake-sleep loop. We use
     * gpio_get_level() because at this point the pad is still on
     * the digital GPIO matrix; esp_sleep_enable_ext0_wakeup() is
     * what later switches it to the RTC IO mux. */
    bool wake_pin_ready = true;
    {
        const int     poll_ms        = 20;
        const int     required_high  = 3;
        const int     max_wait_ms    = 500;
        int           consec_high    = 0;
        int           waited_ms      = 0;
        while (consec_high < required_high) {
            int lvl = gpio_get_level(wake_gpio);
            if (lvl == 1) {
                consec_high++;
            } else {
                consec_high = 0;
            }
            if (consec_high >= required_high) {
                break;
            }
            if (waited_ms >= max_wait_ms) {
                ESP_LOGW(TAG, "Wake pin GPIO%d still LOW after %d ms -- "
                              "skipping EXT0 arm to avoid immediate "
                              "wake; device will only wake via RESET",
                         (int)wake_gpio, max_wait_ms);
                wake_pin_ready = false;
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(poll_ms));
            waited_ms += poll_ms;
        }
        if (wake_pin_ready && waited_ms > 0) {
            ESP_LOGI(TAG, "Wake pin GPIO%d settled HIGH after %d ms",
                     (int)wake_gpio, waited_ms);
        }
    }

    /* Configure wake-up GPIO as EXT0 wake-up source (wake on low level) */
    esp_err_t wake_err = ESP_FAIL;
    if (wake_pin_ready) {
        wake_err = esp_sleep_enable_ext0_wakeup(wake_gpio, 0);
        if (wake_err != ESP_OK) {
            ESP_LOGW(TAG, "esp_sleep_enable_ext0_wakeup(GPIO%d) failed: %s -- "
                          "device will only wake via RESET",
                     (int)wake_gpio, esp_err_to_name(wake_err));
        } else {
            ESP_LOGI(TAG, "Entering deep sleep, wake on GPIO%d (final level=%d)...",
                     (int)wake_gpio, gpio_get_level(wake_gpio));
        }
    } else {
        ESP_LOGI(TAG, "Entering deep sleep (EXT0 skipped, wake via RESET)...");
    }
#else
    /* Target has no EXT0 wake source (ESP32-P4, ESP32-C2/C3/C6/H2/...).
     * Some of these support EXT1 / LP_IO GPIO wakeup but only for a
     * restricted set of pins (LP_IO range, e.g. GPIO0-15 on P4). On
     * the only P4 board currently supported (M5Stack Tab5) no user
     * button or touch INT lands on an LP_IO pin, so the only viable
     * wake path is the hardware RESET button -- the chip cold-boots
     * and the editor restores state from autosave on the next run.
     * We therefore skip GPIO-wake arming entirely on these targets;
     * any timer / touch wake source can still be enabled by the
     * caller via the standard esp_sleep_* APIs before invoking
     * standby_enter_sleep(). */
    (void)wake_gpio;
    ESP_LOGI(TAG, "Entering deep sleep (no GPIO wake source on this target; "
                  "wake via RESET)...");
#endif
    esp_deep_sleep_start();
    /* Does not return */
#endif /* !CONFIG_DRAFTLING_STANDBY_DISPLAY_OFF */
}
