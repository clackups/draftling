#include <cstdio>
#include <cinttypes>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <driver/spi_common.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include <driver/uart.h>
#include <esp_sleep.h>
#if defined(CONFIG_DRAFTLING_DISPLAY_EPDIY) || defined(CONFIG_DRAFTLING_DISPLAY_H752_EPD)
#include <driver/i2c_master.h>
#endif
#if defined(CONFIG_DRAFTLING_DISPLAY_MIPI_DSI)
#include "bsp/m5stack_tab5.h"
#endif
#include "sdkconfig.h"

#if defined(CONFIG_ESP_HOSTED_ENABLED)
#include "esp_hosted.h"
#include "esp_hosted_event.h"
#include "esp_event.h"
#include <freertos/event_groups.h>
#endif

#include "app_config.h"
#include "display.h"
#include "lvgl_port.h"
#include "sd_card.h"
#include "ble_keyboard.h"
#include "editor.h"
#include "editor_ui.h"
#include "wifi_manager.h"
#include "git_sync.h"
#include "standby.h"
#include "battery.h"
#include "touchscreen.h"
#include "power.h"
#if defined(CONFIG_DRAFTLING_HAS_USB_HOST)
#include "usb_kbd.h"
#endif
#if defined(CONFIG_DRAFTLING_HAS_TAB5_KBD)
#include "tab5_kbd.h"
#endif

static const char *TAG = "Draftling";

#if defined(CONFIG_DRAFTLING_DEBUG_POWER) || \
    defined(CONFIG_DRAFTLING_CHARGER_WATCHDOG_REASSERT)
/* esp_timer handle for the periodic charger maintenance task. Created
 * once after battery_init_bq25896() succeeds; runs every 30 s and
 * does two things:
 *
 *   - Always re-asserts the safety-critical BQ25896 registers
 *     (ICO_EN=0 in REG02, WATCHDOG=00 in REG07) when
 *     CONFIG_DRAFTLING_CHARGER_WATCHDOG_REASSERT is set. Idempotent
 *     and cheap (two I2C RMW + write transactions). Catches the
 *     edge case where a one-off bus glitch or a missed write at
 *     boot would otherwise leave the charger in its 500 mA USB-SDP
 *     default until reboot.
 *
 *   - When CONFIG_DRAFTLING_DEBUG_POWER is also set AND the chip
 *     currently sees VBUS, logs the full diagnostic register set
 *     (REG00 / REG0B / REG0C / REG0E / REG0F / REG10 / REG11 /
 *     REG12 / REG13). This is enough to localise a slow-charge
 *     regression to a specific status bit (THERM_STAT, VDPM_STAT,
 *     NTC_FAULT, IDPM_STAT, etc.) without a bench meter.
 */
static esp_timer_handle_t s_charger_maint_timer = NULL;

/* 30 s cadence for the BQ25896 maintenance / debug timer: short
 * enough to recover from a 40 s POR I2C watchdog timeout before it
 * reverts our register writes, long enough that the periodic I2C
 * traffic is negligible. */
#define CHARGER_MAINT_PERIOD_US ((uint64_t)30 * 1000 * 1000)

static void charger_maint_cb(void *arg)
{
    (void)arg;
#if defined(CONFIG_DRAFTLING_CHARGER_WATCHDOG_REASSERT)
    battery_bq25896_reassert_config();
#endif
#if defined(CONFIG_DRAFTLING_DEBUG_POWER)
    /* Only dump status while charging is plausible. Avoids spamming
     * the log every 30 s when the device is running on battery and
     * there is no useful charger telemetry to report. */
    if (battery_bq25896_vbus_present() == 1) {
        battery_bq25896_dump_status();
    }
#endif
}

static void charger_maint_start(void)
{
    if (s_charger_maint_timer) return;
    esp_timer_create_args_t args = {};
    args.callback = charger_maint_cb;
    args.name     = "charger_maint";
    if (esp_timer_create(&args, &s_charger_maint_timer) != ESP_OK) {
        ESP_LOGW(TAG, "charger_maint: esp_timer_create failed");
        s_charger_maint_timer = NULL;
        return;
    }
    /* 30 s cadence: see CHARGER_MAINT_PERIOD_US comment above. */
    if (esp_timer_start_periodic(s_charger_maint_timer,
                                 CHARGER_MAINT_PERIOD_US) != ESP_OK) {
        ESP_LOGW(TAG, "charger_maint: esp_timer_start_periodic failed");
    }
}
#endif /* DEBUG_POWER || CHARGER_WATCHDOG_REASSERT */

#if defined(CONFIG_DRAFTLING_DEBUG_POWER)
/* NVS keys for the deep-sleep drain estimator. Written by
 * pre_sleep_t5_deinit on the way into deep sleep; read back here on
 * the next cold boot to compute "uA average" across the sleep
 * interval. esp_sleep_get_wakeup_cause() distinguishes wake-from-
 * deep-sleep (we have a meaningful delta) from a real cold boot
 * (we don't -- the SOC drop, if any, came from active use, not
 * sleep). The wall-clock anchor is uptime since wake; combined with
 * the BQ27220's reported SOC step it is enough for a rough but
 * useful "device drew X uA average for Y minutes" estimate. */
static void log_boot_power_telemetry(void)
{
    int mv  = battery_read_mv();
    int soc = battery_read_percent();
    int cur = battery_read_current_ma();
    ESP_LOGI(TAG, "Boot telemetry: %dmV %d%% %dmA (signed: +charge / -load)",
             mv, soc, cur);

    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    if (cause == ESP_SLEEP_WAKEUP_UNDEFINED) {
        /* Power-on / RESET / brown-out: no prior sleep entry to
         * compare against, so the persisted entry SOC is stale. */
        return;
    }

    nvs_handle_t h;
    if (nvs_open("draftling_pwr", NVS_READONLY, &h) != ESP_OK) return;
    uint8_t entry_soc = 0xFF;
    int32_t entry_mv  = 0;
    int32_t entry_cur = 0;
    esp_err_t e1 = nvs_get_u8(h,  "entry_soc",  &entry_soc);
    (void)nvs_get_i32(h, "entry_mv",   &entry_mv);
    (void)nvs_get_i32(h, "entry_curr", &entry_cur);
    nvs_close(h);
    if (e1 != ESP_OK || entry_soc > 100 || soc < 0) return;

    int delta_soc = (int)entry_soc - soc;
    /* esp_sleep_get_wakeup_cause is set on the wake that just
     * happened; uptime since wake is a good proxy for "time spent
     * post-wake" but the *sleep duration* is what we actually want.
     * ESP-IDF exposes that only on targets with RTC slow clock
     * retention; with RTC_SLOW_MEM powered off (our pre-sleep
     * hook) it is unavailable. As a fallback we report the SOC
     * delta and let the operator divide by their observed wall
     * clock. Without a wall-clock the uA estimate is meaningless,
     * so we skip it. */
    ESP_LOGI(TAG,
        "Sleep delta vs persisted entry: dSOC=%d%% (entry %u%% @ %ldmV, %ldmA) "
        "-> woke at %d%% @ %dmV, %dmA",
        delta_soc,
        (unsigned)entry_soc, (long)entry_mv, (long)entry_cur,
        soc, mv, cur);
}
#endif /* CONFIG_DRAFTLING_DEBUG_POWER */

/* Save one open document (called for every in-use doc via
 * editor_doc_foreach) before deep sleep. Persists the body if it has
 * unsaved changes and a path, and always persists the cursor/scroll
 * sidecar so reopening resumes at the last position. */
static void pre_sleep_save_doc_cb(editor_doc_t *doc, void *ctx)
{
    (void)ctx;
    if (editor_doc_is_modified(doc) && editor_doc_get_path(doc)) {
        ESP_LOGI(TAG, "Auto-saving %s before deep sleep...",
                 editor_doc_get_path(doc));
        esp_err_t err = editor_doc_save(doc);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Auto-save failed: %s", esp_err_to_name(err));
        }
    }
    editor_doc_save_meta(doc);
}

/* Called by standby just before entering deep sleep */
static void pre_sleep_autosave(void)
{
    ESP_LOGI(TAG, "Pre-sleep: autosave + EPD wipe (generic path)");
    /* Persist every open document (both panes when split), not just the
     * active one. Each callback saves the body if modified and always
     * writes the cursor/scroll sidecar so reopening resumes in place. */
    editor_doc_foreach(pre_sleep_save_doc_cb, NULL);
#if defined(CONFIG_DRAFTLING_DISPLAY_EPD)
    /* E-paper retains its image without power. Wipe the panel to a
     * clean white frame so the user does not see the editor frozen on
     * the display while the MCU is in deep sleep. We do this even when
     * CONFIG_DRAFTLING_EPD_BLACK_BACKGROUND is enabled -- the panel is
     * easier to read at a glance when blank, and a black frame held
     * indefinitely is worse for the e-paper than a white one.
     *
     * Take the LVGL mutex first so this does not race with the LVGL
     * task's flush_cb. */
    /* Take the LVGL mutex if we can so the wipe does not race with
     * the LVGL task's flush_cb. The mutex is recursive
     * (draftling_lvgl_port_init), so this also works when pre_sleep_autosave
     * runs inside the LVGL task itself (the "Sleep now" menu path).
     * If for any reason the lock cannot be obtained quickly, wipe
     * anyway -- a clean white frame on the panel matters more than
     * the slim chance of a flush_cb collision right before deep
     * sleep. */
    bool locked = draftling_lvgl_port_lock(200);
    display_clear(0xFF);
    display_full_refresh();
    if (locked) {
        draftling_lvgl_port_unlock();
    } else {
        ESP_LOGW(TAG, "pre_sleep wipe: LVGL lock not acquired, proceeded anyway");
    }
#endif
}

#if defined(CONFIG_DRAFTLING_MODEL_LILYGO_T5_EPD_S3_PRO) || \
    defined(CONFIG_DRAFTLING_MODEL_LILYGO_T5_EPD_S3_PRO_H752)
/* LilyGO T5 E-Paper S3 Pro / Pro Lite: deep-sleep current budget.
 *
 * Without this hook the board pulls ~30 mA in deep sleep (a 1500 mAh
 * NP-F battery dies in ~2 days). Target after this hook is < 100 uA,
 * giving > 2 weeks idle. Each step plugs one peripheral:
 *
 *   Peripheral             Sleep state we drive it to            Source
 *   --------------------   ----------------------------------    ----------
 *   GT911 touch ctrlr      cmd 0x05 -> "sleep" (~8 uA)           GT911 DS sec.6
 *   SX1262 LoRa (Pro)      SetSleep(0x00) (~600 nA cold)         SX126x DS sec.13.1.7
 *   SD card (SPI)          SPI3 bus freed; card to standby        sd_card_deinit()
 *   EPDIY (TPS65185+exp)   epd_poweroff() -> WAKEUP low (<1 uA)   epdiy README
 *   Front-light LED (G11)  driven LOW + RTC IO hold                display_deep_sleep_prepare()
 *   ESP32-S3 RTC_PERIPH    powered down                            esp_sleep_pd_config()
 *
 * The display + touch deinit must happen with the I2C bus still
 * alive, so the sequence is:
 *   touchscreen_sleep()              (uses I2C)
 *   display_deep_sleep_prepare()     (uses I2C, then tears it down)
 *   sd_card_deinit()                 (releases SPI3)
 *   SX1262 SetSleep over SPI3        (must run BEFORE spi_bus_free,
 *                                     but sd_card_deinit closes the
 *                                     bus, so we do LoRa first)
 *   esp_sleep_pd_config(RTC_PERIPH)  (final hint to PMU)
 *   gpio_deep_sleep_hold_en()        (latch all gpio_hold_en pads)
 *
 * `standby_enter_sleep()` does the actual `esp_deep_sleep_start()`
 * after this callback returns.
 */

/* SX1262 SetSleep opcode (sx126x datasheet sec.13.1.7). We issue a
 * cold-start sleep (no register retention -- we don't have any
 * persistent radio state to keep). 1-byte opcode + 1-byte param.
 *
 * Before issuing SetSleep we read a GetStatus byte. If the chip is
 * depopulated (Lite variant) the MISO line will read 0xFF or 0x00
 * idle and we skip the sleep write; on the Pro variant a healthy
 * SX1262 returns a non-trivial status byte and we proceed. This
 * prevents two failure modes:
 *   - on Lite, a stale "SX1262 entered sleep" log when no radio
 *     actually responded;
 *   - on Pro, a SetSleep being NAK'd because the radio is mid-FSK
 *     RX after a stray init -- a state we cannot recover from
 *     blind. */
static void t5_lora_sleep(void)
{
#if defined(BOARD_LORA_CS_PIN) && (BOARD_LORA_CS_PIN >= 0)
    /* The SX1262 sits on SPI3 (shared with the SD card). Talk to it
     * directly via spi_bus_add_device using the LoRa CS, then remove
     * the device when done. Failure is non-fatal -- the radio chip
     * is depopulated on the Lite variant, so a NACK / no-response is
     * the expected outcome there. */
    spi_device_handle_t lora = NULL;
    spi_device_interface_config_t devcfg = {};
    devcfg.clock_speed_hz = 1 * 1000 * 1000;  /* 1 MHz is safe for SetSleep */
    devcfg.mode           = 0;
    devcfg.spics_io_num   = BOARD_LORA_CS_PIN;
    devcfg.queue_size     = 1;
    esp_err_t err = spi_bus_add_device(SPI3_HOST, &devcfg, &lora);
    if (err != ESP_OK) {
        ESP_LOGD(TAG, "SX1262 spi_bus_add_device failed: %s",
                 esp_err_to_name(err));
        goto repark_cs;
    }

    /* GetStatus (opcode 0xC0) returns the chip mode + command status
     * in the byte clocked back as we send a 0x00 dummy. SX1262
     * datasheet sec. 13.5.1: bits[6:4] = ChipMode (1..6 == valid;
     * 0/7 == invalid/reserved). On a depopulated bus we'll read 0x00
     * or 0xFF here -- both invalid -- and bail out without spamming
     * the log. */
    {
        uint8_t gs_tx[2] = { 0xC0, 0x00 };
        uint8_t gs_rx[2] = { 0xFF, 0xFF };
        spi_transaction_t gs = {};
        gs.length    = sizeof(gs_tx) * 8;
        gs.tx_buffer = gs_tx;
        gs.rx_buffer = gs_rx;
        err = spi_device_polling_transmit(lora, &gs);
        if (err != ESP_OK) {
            ESP_LOGD(TAG, "SX1262 GetStatus transmit failed: %s",
                     esp_err_to_name(err));
            goto remove_device;
        }
        uint8_t chip_mode = (gs_rx[1] >> 4) & 0x07;
        if (chip_mode == 0 || chip_mode == 7) {
            ESP_LOGD(TAG, "SX1262 GetStatus=0x%02X (no chip response) -- "
                     "skipping SetSleep", gs_rx[1]);
            goto remove_device;
        }

        uint8_t tx[2] = { 0x84 /* SetSleep */, 0x00 /* cold start */ };
        spi_transaction_t t = {};
        t.length    = sizeof(tx) * 8;
        t.tx_buffer = tx;
        err = spi_device_polling_transmit(lora, &t);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "SX1262 entered sleep mode (was ChipMode=%u)",
                     (unsigned)chip_mode);
        } else {
            ESP_LOGD(TAG, "SX1262 SetSleep transmit failed: %s",
                     esp_err_to_name(err));
        }
    }

remove_device:
    spi_bus_remove_device(lora);

repark_cs:
    /* spi_bus_remove_device() releases the CS GPIO back to the GPIO
     * matrix with no defined level. If the SX1262 CS is then left
     * floating again the radio resumes snooping on subsequent SD
     * traffic over the shared SPI3 bus -- exactly the
     * "LilyGO T5S3-4.7-e-paper-PRO issue #3" we work around at SD
     * mount time. Observed symptom: SD card mounts, multi-sector
     * data-phase probe passes, but FatFs's first cluster-chain read
     * a few hundred ms later returns 0x107 ESP_ERR_TIMEOUT or
     * 0x108 ESP_ERR_INVALID_RESPONSE from sdmmc_read_sectors_dma.
     * Re-pin LoRa CS HIGH as a plain GPIO output on every exit path
     * (including the early bail-outs above for a depopulated /
     * unresponsive radio) so the radio stays deselected for the rest
     * of the active session. */
    {
        gpio_config_t lora_cs = {};
        lora_cs.intr_type    = GPIO_INTR_DISABLE;
        lora_cs.mode         = GPIO_MODE_OUTPUT;
        lora_cs.pin_bit_mask = (1ULL << BOARD_LORA_CS_PIN);
        gpio_config(&lora_cs);
        gpio_set_level((gpio_num_t)BOARD_LORA_CS_PIN, 1);
    }
#endif
}

/* MIA-M10Q GPS (Pro variant) -> UBX-RXM-PMREQ "backup" mode.
 *
 * The T5 E-Paper S3 Pro / Pro Lite carries the MIA-M10Q on UART2
 * (TX=43, RX=44, default baud 38400 -- see the factory firmware
 * peri_gps.cpp comment "Set u-blox m10q gps baudrate 38400"). The
 * chip has *no* dedicated power-enable GPIO -- it is hard-wired to
 * the 3V3 rail -- so the only software lever for its ~25 mA run
 * draw is the receiver's own RXM-PMREQ command (u-blox M10 interface
 * description, sec.3.13.6.4). In backup the chip idles at ~15 uA
 * until a level transition on its RX pin or a hardware reset wakes
 * it up again. Draftling never uses GPS during normal operation, so
 * we issue PMREQ once at boot and again from pre_sleep_t5_deinit().
 *
 * The Lite variant has the GPS chip depopulated; we still drive
 * the UART pins, the bytes clock into a floating line, and the
 * gpio_reset_pin() at the end releases them. No harm done. */
static void t5_gps_sleep(void)
{
#if defined(BOARD_GPS_TX_PIN) && defined(BOARD_GPS_RX_PIN) && \
    (BOARD_GPS_TX_PIN >= 0) && (BOARD_GPS_RX_PIN >= 0)
    /* UBX-RXM-PMREQ v0 (CLASS=0x02, ID=0x41, len=8):
     *   duration[4] = 0   (infinite)
     *   flags[4]    = 0x06 (bit1=backup, bit2=force)
     * Checksum is 8-bit Fletcher over class..payload (sec.3.4 of
     * the M10 interface description). Pre-computed and frozen here
     * so we don't have to drag a checksum helper into main.cpp. */
    static const uint8_t kPmreqBackup[] = {
        0xB5, 0x62,                         /* sync chars                  */
        0x02, 0x41,                         /* class=RXM, id=PMREQ         */
        0x08, 0x00,                         /* payload length = 8 (LE)     */
        0x00, 0x00, 0x00, 0x00,             /* duration = 0 (infinite)     */
        0x06, 0x00, 0x00, 0x00,             /* flags = backup | force      */
        0x51, 0x4B                          /* CK_A, CK_B                  */
    };

    static const uart_port_t kPort = UART_NUM_1;
    uart_config_t cfg = {};
    cfg.baud_rate = 38400;
    cfg.data_bits = UART_DATA_8_BITS;
    cfg.parity    = UART_PARITY_DISABLE;
    cfg.stop_bits = UART_STOP_BITS_1;
    cfg.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    cfg.source_clk = UART_SCLK_DEFAULT;

    esp_err_t err = uart_driver_install(kPort, 256, 0, 0, NULL, 0);
    bool installed_here = (err == ESP_OK);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGD(TAG, "GPS UART install failed: %s", esp_err_to_name(err));
        return;
    }
    (void)uart_param_config(kPort, &cfg);
    (void)uart_set_pin(kPort, BOARD_GPS_TX_PIN, BOARD_GPS_RX_PIN,
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    int n = uart_write_bytes(kPort, (const char *)kPmreqBackup,
                             sizeof(kPmreqBackup));
    if (n == (int)sizeof(kPmreqBackup)) {
        (void)uart_wait_tx_done(kPort, pdMS_TO_TICKS(100));
        ESP_LOGI(TAG, "MIA-M10Q GPS: sent UBX-RXM-PMREQ backup (%d bytes)", n);
    } else {
        ESP_LOGD(TAG, "GPS UART write returned %d", n);
    }

    if (installed_here) {
        (void)uart_driver_delete(kPort);
    }
    /* Release the UART pins so they go to high-Z and don't drive
     * against the GPS once it has entered backup. The pads are not
     * RTC-IO (GPIO 43/44 sit outside the 0..21 RTC range) so they
     * default to high-Z in deep sleep automatically; this just
     * detaches the UART matrix early. */
    gpio_reset_pin((gpio_num_t)BOARD_GPS_TX_PIN);
    gpio_reset_pin((gpio_num_t)BOARD_GPS_RX_PIN);
#endif
}

/* One-shot "shut every Pro-only radio up" helper, called from
 * app_main() right after SD init (so SPI3 is available to the LoRa
 * sleep path). Idempotent and safe to re-invoke later. */
static void t5_disable_radios_at_boot(void)
{
    ESP_LOGI(TAG, "Pro: disabling SX1262 LoRa and MIA-M10Q GPS at boot");
    t5_lora_sleep();
    t5_gps_sleep();
}

#if defined(CONFIG_DRAFTLING_MODEL_LILYGO_T5_EPD_S3_PRO)
/* Sweep a hard-coded list of GPIOs that are not wired to any
 * peripheral on the T5 (per the LilyGO factory schematic) and
 * isolate them so they don't leak through floating inputs in deep
 * sleep. We restrict the list to RTC-capable pins because
 * rtc_gpio_isolate() is the documented low-power state for those.
 * Non-RTC pins are released to high-Z via gpio_reset_pin -- the
 * pad's analog input is already off in deep sleep, so a high-Z
 * output is the lowest-leakage state available.
 *
 * Only the original Pro variant uses the deep-sleep path that
 * calls into these helpers; the H752 build has its own teardown
 * and would warn-as-unused if these were compiled in. */
static void t5_isolate_unused_gpios(void)
{
    /* ESP32-S3 RTC-IO range is GPIO0..GPIO21. Skip pins that are
     * actively used in deep sleep OR are otherwise NOT spare:
     *   GPIO0           -- BOOT button, EXT0 wake (handled by standby.cpp)
     *   GPIO5..GPIO10   -- EPD 8-bit parallel data bus (epd_board_v7
     *                      pins D0..D5; released by epd_lcd_deinit()
     *                      inside display_deep_sleep_prepare(), so
     *                      isolating them again here is redundant and
     *                      historically mis-described as "unused").
     *   GPIO11          -- front-light LED, held LOW with gpio_hold_en
     *                      (display_deep_sleep_prepare); must NOT be
     *                      reclaimed by an isolate call.
     *   GPIO12          -- SD CS (released by sd_card_deinit).
     *   GPIO13          -- SD MOSI / shared SPI3 (released by sd_card_deinit).
     *   GPIO14          -- SD SCK  / shared SPI3 (released by sd_card_deinit).
     *   GPIO15..GPIO18  -- EPD 8-bit parallel data bus (epd_board_v7
     *                      pins D3, D4, D5, D6; same rationale as
     *                      GPIO5..GPIO10).
     *   GPIO19, GPIO20  -- ESP32-S3 USB-Serial-JTAG (USB CDC console).
     *                      Isolating these silently kills the last
     *                      few lines of log output before
     *                      esp_deep_sleep_start(), which makes any
     *                      pre-sleep crash invisible to `idf.py
     *                      monitor`. Leave them alone -- the
     *                      USB-Serial-JTAG controller is powered
     *                      down by the digital domain in deep sleep
     *                      regardless.
     *   GPIO21          -- SD MISO / shared SPI3 (released by sd_card_deinit).
     *
     * That leaves GPIO1, GPIO2, GPIO3, GPIO4 as the genuinely spare
     * RTC-IO pins on this board. GPIO3 (TOUCH_INT on the T5) is
     * normally driven by the GT911; once display_deep_sleep_prepare()
     * has collapsed the EPD rail the GT911 is unpowered and its INT
     * line floats, so isolating GPIO3 here is appropriate. */
    static const int rtc_unused[] = {
        1, 2, 3, 4
    };
    for (size_t i = 0; i < sizeof(rtc_unused) / sizeof(rtc_unused[0]); ++i) {
        gpio_num_t g = (gpio_num_t)rtc_unused[i];
        if (rtc_gpio_is_valid_gpio(g)) {
            (void)rtc_gpio_isolate(g);
        }
    }
}

/* Undo, at boot, every IO state we forced into deep sleep:
 *
 *   * gpio_deep_sleep_hold_en() -- chip-wide latch enable. On
 *     ESP32-S3 the per-pin hold registers survive the wake reset
 *     and keep latched pads driven (or, for inputs, isolated)
 *     until explicitly released. Without this, the front-light
 *     stays dark and the touchscreen INT line never goes back to
 *     a normal digital input.
 *
 *   * gpio_hold_en() on the front-light (GPIO11). Pinned LOW by
 *     display_deep_sleep_prepare() so the LED does not glow on
 *     leakage during sleep. Released here so display_init() ->
 *     backlight_pwm_init() can route LEDC back to the pin.
 *
 *   * rtc_gpio_isolate() on the spare RTC-IO pads (GPIO1..GPIO4)
 *     from t5_isolate_unused_gpios(). Isolation switches the pad
 *     to the RTC GPIO mux with both input and output buffers
 *     disabled, *and engages an RTC-domain pad hold* via
 *     rtc_gpio_hold_en(). That selection AND the hold survive the
 *     wake reset on ESP32-S3. The chip-wide gpio_deep_sleep_hold_dis()
 *     above only releases *digital* pad holds (the ones armed by
 *     gpio_hold_en()), and rtc_gpio_deinit() only flips the IO
 *     mux back to the digital matrix -- neither of them clears the
 *     RTC hold register. So we have to call rtc_gpio_hold_dis()
 *     explicitly first, before rtc_gpio_deinit(), or the pad stays
 *     clamped at its isolated level. GPIO3 (the GT911 TOUCH_INT)
 *     in particular has to come back to a normal digital input
 *     before touchscreen_init() configures it as an input with
 *     pull-up, otherwise the INT line never reads a valid level
 *     and the touchscreen appears dead after wake even though the
 *     I2C side of the controller responds.
 *
 * Safe to call unconditionally on every boot: gpio_hold_dis /
 * rtc_gpio_hold_dis on a pad that was never latched is a no-op,
 * and rtc_gpio_deinit on a pad already in the digital mux just
 * rewrites the (same) mux selection. */
static void t5_release_held_gpios_after_wake(void)
{
    gpio_deep_sleep_hold_dis();

    /* Front-light pin must match EPDIY_BL_PIN in display_epdiy.cpp. */
    gpio_hold_dis((gpio_num_t)11);

    static const int rtc_isolated[] = {
        1, 2, 3, 4
    };
    for (size_t i = 0; i < sizeof(rtc_isolated) / sizeof(rtc_isolated[0]); ++i) {
        gpio_num_t g = (gpio_num_t)rtc_isolated[i];
        if (rtc_gpio_is_valid_gpio(g)) {
            /* Release the RTC pad hold engaged by rtc_gpio_isolate()
             * before flipping the mux back to the digital GPIO
             * matrix; otherwise the pad stays clamped at its
             * isolated level. */
            (void)rtc_gpio_hold_dis(g);
            (void)rtc_gpio_deinit(g);
        }
    }
}
#endif /* CONFIG_DRAFTLING_MODEL_LILYGO_T5_EPD_S3_PRO */

#if defined(CONFIG_DRAFTLING_MODEL_LILYGO_T5_EPD_S3_PRO)
static void pre_sleep_t5_deinit(void)
{
    ESP_LOGI(TAG, "Pre-sleep: LilyGO T5 peripheral teardown");
    /* Run the editor autosave + EPD wipe first; that path uses the
     * touch / display / I2C / SD subsystems while they are still up. */
    pre_sleep_autosave();

    /* GT911 touch controller -> sleep (uses the shared I2C bus).
     * Issued before EPD power-down because the GT911 lives on the
     * same bus that epd_deinit() will tear down. */
    touchscreen_sleep();

#if defined(CONFIG_DRAFTLING_DEBUG_POWER)
    /* Snapshot the fuel gauge's current draw and SOC immediately
     * before we start cutting peripherals. The next cold boot will
     * read the persisted SOC delta and estimate average sleep draw
     * in uA without needing a bench meter. */
    {
        int mv_now   = battery_read_mv();
        int soc_now  = battery_read_percent();
        int cur_now  = battery_read_current_ma();
        ESP_LOGI(TAG, "Pre-sleep telemetry: %dmV %d%% %dmA (active draw)",
                 mv_now, soc_now, cur_now);
        if (soc_now >= 0) {
            nvs_handle_t h;
            if (nvs_open("draftling_pwr", NVS_READWRITE, &h) == ESP_OK) {
                /* Persist the entry SOC and a wall-clock anchor. We
                 * use uptime in microseconds (esp_timer_get_time)
                 * because no RTC time-of-day is guaranteed to be
                 * synced; on the next cold boot we can read RTC
                 * residual time via esp_sleep_get_wakeup_cause +
                 * gpio_dump_io_configuration, but the simplest
                 * portable proxy is "store wall time if SNTP got us
                 * one, else nothing". Always store SOC. */
                (void)nvs_set_u8(h,  "entry_soc",  (uint8_t)soc_now);
                (void)nvs_set_i32(h, "entry_curr", (int32_t)cur_now);
                (void)nvs_set_i32(h, "entry_mv",   (int32_t)mv_now);
                (void)nvs_commit(h);
                nvs_close(h);
            }
        }
    }
#endif

    /* SX1262 LoRa -> sleep (Pro variant only; harmless NACK on Lite). */
    t5_lora_sleep();

    /* MIA-M10Q GPS -> backup mode (Pro variant only; bytes clock into
     * an unconnected line on Lite, which is harmless). Done before the
     * RTC-IO isolation sweep so the UART pins are released cleanly. */
    t5_gps_sleep();

    /* If we are about to enter deep sleep on battery (no USB plugged
     * in), put the BQ25896 charger into HIZ so it stops drawing its
     * ~1.5 mA idle bias current from the cell. No-op when VBUS is
     * present (user wants to keep charging through sleep) or when
     * the chip was never initialised. Cold-boot re-init clears HIZ.
     *
     * Done here -- BEFORE display_deep_sleep_prepare() tears down the
     * shared I2C bus inside epd_deinit() -- so all the I2C-attached
     * peripherals on this board (GT911 touch, BQ27220 fuel gauge,
     * BQ25896 charger, TPS65185 EPD PMIC, PCA9535 expander) are
     * touched in a single linear sweep with the bus still alive. */
    battery_bq25896_prepare_sleep();

    /* Release the SD card + SPI3 bus so the card itself drops to
     * standby and the bus pins float free. After this point any SPI3
     * transaction (including the LoRa one above) is illegal -- which
     * is why we do LoRa first. */
    (void)sd_card_deinit();

    /* Tear down the EPD power IC + epdiy I2C state. After this call
     * the shared I2C bus may have been released by epd_deinit(),
     * so any further I2C work (touchscreen, BQ27220, BQ25896) must
     * have already happened above.
     *
     * We deliberately do NOT re-issue touchscreen_sleep() afterwards.
     * The GT911's RST line is wired through the TPS65185 / PCA9535
     * expander, so epd_poweroff() can yank reset and the controller
     * may come back at its backup I2C address 0x14 instead of 0x5D;
     * blindly retrying a sleep command into that state is racy.
     * The authoritative GT911 sleep already happened above (before
     * any EPD teardown), with the bus still in a known-good state. */
    display_deep_sleep_prepare();

    /* Isolate every unused RTC-IO pin so floating inputs don't leak. */
    t5_isolate_unused_gpios();

    /* Power down the RTC peripherals domain. We have no ULP and no
     * RTC SLOW memory data to retain across wake (the SoC cold-boots
     * on wake and the editor restores from autosave), so dropping
     * this domain saves ~40 uA on ESP32-S3 over the default. The
     * ESP32-S3 does not expose separate ESP_PD_DOMAIN_RTC_SLOW_MEM /
     * RTC_FAST_MEM controls in current ESP-IDF -- those memories
     * follow the RTC_PERIPH / VDD_SDIO domains automatically. */
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH,   ESP_PD_OPTION_OFF);

    /* Latch every pad that called gpio_hold_en() (the front-light,
     * specifically) so the level stays driven through deep sleep. */
    gpio_deep_sleep_hold_en();
}
#endif /* CONFIG_DRAFTLING_MODEL_LILYGO_T5_EPD_S3_PRO */

#if defined(CONFIG_DRAFTLING_MODEL_LILYGO_T5_EPD_S3_PRO_H752)
static void pre_sleep_h752_deinit(void)
{
    ESP_LOGI(TAG, "Pre-sleep: LilyGO original H752 peripheral teardown");

    pre_sleep_autosave();

    /* H752 sleeps via esp_light_sleep_start() (GPIO48 is not an RTC
     * IO, see standby.cpp), which freezes the CPU while the BT
     * controller would otherwise keep scanning; stop BLE first so
     * the controller is quiescent when the clocks gate. */
    ble_keyboard_disable();
    touchscreen_sleep();

    t5_lora_sleep();
    t5_gps_sleep();

    battery_bq25896_prepare_sleep();
    (void)sd_card_deinit();

    display_deep_sleep_prepare();
    gpio_deep_sleep_hold_en();
}
#endif /* CONFIG_DRAFTLING_MODEL_LILYGO_T5_EPD_S3_PRO_H752 */
#endif /* LilyGO T5 EPD S3 family */

#if defined(CONFIG_DRAFTLING_MODEL_LILYGO_T5_EPD_S3_PRO_H752)
/* ---- H752 hardware shortcut keys ----
 *
 * The side key (GPIO48, also the light-sleep wake source) doubles as
 * a Menu key during normal operation: a press injects F1 through the
 * same editor_ui_handle_key() path the BLE/USB keyboards use, so it
 * opens or closes the menu exactly like F1 on a keyboard. Polled at
 * 30 ms by an esp_timer with a two-sample debounce; the synthetic
 * event is injected as press+release in one go so the key-repeat
 * tracker never fires a second toggle while the button is held.
 *
 * The capacitive touch key below the panel (GT911 status bit 0x10,
 * surfaced by touchscreen_set_button_callback) acts as Back: it
 * injects Esc, which closes the menu / settings / prompts, and in
 * the editor returns to the file browser (auto-saving, see
 * editor_ui.cpp). */

static void h752_inject_key(uint8_t keycode)
{
    ESP_LOGD(TAG, "H752 key inject: keycode=0x%02X", keycode);
    kb_event_t ev = {};
    ev.keycode = keycode;
    ev.pressed = true;
    editor_ui_handle_key(&ev);
    ev.pressed = false;
    editor_ui_handle_key(&ev);
}

static void h752_touch_button_cb(void)
{
    h752_inject_key(KB_KEY_ESCAPE);
}

static void h752_user_key_poll_cb(void *arg)
{
    (void)arg;
    static bool down = false;
    static int  stable = 0;
    bool raw_down = gpio_get_level((gpio_num_t)WAKEUP_GPIO_NUM) == 0;
    if (raw_down == down) {
        stable = 0;
        return;
    }
    if (++stable < 2) {
        return;
    }
    stable = 0;
    down = raw_down;
    ESP_LOGD(TAG, "H752 side key %s (GPIO%d)",
             down ? "PRESSED" : "released", WAKEUP_GPIO_NUM);
    if (down) {
        h752_inject_key(KB_KEY_F1);
    }
}

static void h752_user_key_init(void)
{
    gpio_config_t g = {};
    g.intr_type    = GPIO_INTR_DISABLE;
    g.mode         = GPIO_MODE_INPUT;
    g.pin_bit_mask = 1ULL << WAKEUP_GPIO_NUM;
    g.pull_up_en   = GPIO_PULLUP_ENABLE;
    g.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&g);

    esp_timer_create_args_t targs = {};
    targs.callback = h752_user_key_poll_cb;
    targs.name     = "h752_key";
    esp_timer_handle_t t = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&targs, &t));
    ESP_ERROR_CHECK(esp_timer_start_periodic(t, 30 * 1000));
    ESP_LOGI(TAG, "H752 side key poller started (GPIO%d, level now %d)",
             WAKEUP_GPIO_NUM, gpio_get_level((gpio_num_t)WAKEUP_GPIO_NUM));
}
#endif /* CONFIG_DRAFTLING_MODEL_LILYGO_T5_EPD_S3_PRO_H752 */

#if defined(CONFIG_DRAFTLING_MODEL_M5STACK_TAB5)
/* M5Stack Tab5 (ESP32-P4): pre-sleep peripheral teardown.
 *
 * The Tab5 has no LP_IO-capable user input, so deep sleep can only
 * exit via the hardware RESET button (the chip cold-boots; editor
 * state is restored from autosave). With the BSP's default state
 * left untouched the board still draws far more than it should in
 * deep sleep because:
 *
 *   - the ESP32-C6 co-processor stays fully powered through P4 deep
 *     sleep (WLAN_PWR_EN = PI4IOE5V6408 #2 P0 is latched HIGH at
 *     boot and never released; the C6 LDO keeps its CPU, BLE
 *     controller and SDIO link armed),
 *   - the MIPI-DSI panel and its backlight stay on,
 *   - the GT911 touch controller keeps polling at ~3 mA,
 *   - the RTC peripherals domain is left powered.
 *
 * This hook drives each of those into the lowest state we can reach
 * from software, in the order dictated by their dependencies (the
 * I/O expander writes must happen while bsp_i2c is still up; the
 * panel disable is safe at any time because the BSP owns it).
 *
 * Wake path is unchanged: the chip cold-boots on RESET and re-runs
 * app_main(), which re-asserts WLAN_PWR_EN and CHG_EN through the
 * same BSP / expander code used at first boot. */
static void pre_sleep_tab5_deinit(void)
{
    ESP_LOGI(TAG, "Pre-sleep: M5Stack Tab5 peripheral teardown");

    /* Editor autosave first while every bus is still up. */
    pre_sleep_autosave();

    /* GT911 -> sleep (uses the BSP-owned I2C bus, which is still up). */
    touchscreen_sleep();

    /* MIPI-DSI panel + backlight off via the BSP-managed handles. */
    display_deep_sleep_prepare();

    /* Drop the ESP32-C6 power rail. The PI4IOE5V6408 latches the
     * pin state across P4 deep sleep, so the rail stays off until
     * the next cold boot re-runs bsp_feature_enable(BSP_FEATURE_WIFI,
     * true) in app_main(). Use the BSP API rather than poking the
     * expander directly so the driver's shadow register stays in
     * sync with the chip; otherwise a later RMW on any other pin
     * on the same expander would un-clear our bit. */
    {
        esp_err_t err = bsp_feature_enable(BSP_FEATURE_WIFI, false);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Tab5: WLAN_PWR_EN dropped (ESP32-C6 off)");
        } else {
            ESP_LOGW(TAG, "Tab5: bsp_feature_enable(WIFI, false) failed: %s",
                     esp_err_to_name(err));
        }
    }

    /* Power down the RTC peripherals domain. Draftling has no ULP
     * and no RTC SLOW data to retain across wake, so dropping this
     * domain shaves a few mA on ESP32-P4 over the default. */
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
}
#endif /* CONFIG_DRAFTLING_MODEL_M5STACK_TAB5 */

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Draftling - %s", BOARD_NAME);

    /* Boot diagnostics: reset reason + wake-up cause.
     *
     * Logged as the very first thing in app_main so a "screen comes
     * back on right after sleep" symptom can be triaged from a
     * single boot line:
     *   ESP_RST_DEEPSLEEP + EXT0  -> wake source fired immediately on
     *                                entry (e.g. BOOT held, or pull-
     *                                up not yet settled when EXT0 was
     *                                armed).
     *   ESP_RST_PANIC             -> something in pre_sleep_t5_deinit
     *                                aborted; the panic backtrace
     *                                will be in the lines above.
     *   ESP_RST_BROWNOUT          -> rail dipped below the BOD
     *                                threshold during sleep entry
     *                                (most likely candidate: EPD
     *                                rail collapse during
     *                                epd_poweroff on a tired cell).
     *   ESP_RST_INT_WDT/TASK_WDT  -> a watchdog tripped while we
     *                                were holding off in
     *                                pre_sleep_t5_deinit.
     *   ESP_RST_POWERON           -> genuine cold boot (USB plug,
     *                                latch released, full battery
     *                                disconnect). Wake cause will
     *                                read UNDEFINED. */
    {
        esp_reset_reason_t        rst  = esp_reset_reason();
        esp_sleep_wakeup_cause_t  wake = esp_sleep_get_wakeup_cause();
        ESP_LOGI(TAG, "Boot: reset_reason=%d wakeup_cause=%d",
                 (int)rst, (int)wake);
    }

    /* Initialize NVS - required for WiFi and BT */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

#if defined(CONFIG_DRAFTLING_HAS_POWER_LATCH)
    /* Close the hardware power latch first thing after NVS so the
     * battery rail stays alive when the user releases the boot-time
     * PWR button press. power_init() also arms a polling timer that
     * watches the PWR button for a long-press (= power off). The
     * pre-off callback is wired up after the editor is created so
     * the long-press save can flush dirty document state. */
    ESP_LOGI(TAG, "Initializing power latch...");
    {
        power_config_t pcfg = {};
        pcfg.pwr_button_gpio = PWR_BUTTON_GPIO;
        pcfg.i2c_sda         = PWR_I2C_SDA_PIN;
        pcfg.i2c_scl         = PWR_I2C_SCL_PIN;
        pcfg.tca9554_addr    = PWR_TCA9554_ADDR;
        pcfg.latch_bit       = PWR_TCA9554_LATCH_BIT;
        power_init(&pcfg);
    }
#endif

    /* Initialize display */
    ESP_LOGI(TAG, "Initializing display...");
#if defined(CONFIG_DRAFTLING_MODEL_LILYGO_T5_EPD_S3_PRO)
    /* If this boot is a wake from deep sleep, undo every IO latch
     * pre_sleep_t5_deinit() applied before esp_deep_sleep_start().
     * Front-light GPIO11 was held LOW (so it stayed dark in sleep)
     * and the spare RTC-IO pads (GPIO1..GPIO4, including the GT911
     * TOUCH_INT on GPIO3) were rtc_gpio_isolate()'d. Both states
     * survive the wake reset on ESP32-S3 and would otherwise leave
     * the front-light dark and the touchscreen unresponsive after
     * wake. Called before display_init() / touchscreen_init() so
     * those bring-up paths see fresh, un-latched pads. */
    t5_release_held_gpios_after_wake();
#endif
#if defined(CONFIG_DRAFTLING_DISPLAY_EPDIY) || defined(CONFIG_DRAFTLING_DISPLAY_H752_EPD)
    /* The LilyGO T5 E-Paper S3 Pro / Pro Lite shares its on-board
     * I2C bus between epdiy (TPS65185 EPD power IC + PCA9535 IO
     * expander), the GT911 capacitive touch controller and the
     * BQ27220 battery fuel gauge (battery_init_bq27220 further
     * below). All consumers use driver-NG (driver/i2c_master.h), and
     * ESP-IDF only allows one i2c_new_master_bus() per port. We
     * therefore create the bus here, ahead of any consumer, and hand
     * the handle to each: display_set_shared_i2c_bus() before
     * display_init() (epdiy routes through epd_init_with_config() +
     * EpdInitConfig.i2c.bus_handle), battery_init_bq27220() right
     * after, and touchscreen_config_t.i2c_bus before
     * touchscreen_init() further below. This is the resolution for
     * the legacy/driver-NG conflict that previously forced touch off
     * on these boards; see components/display/idf_component.yml for
     * why epdiy is pinned to a git commit. */
    i2c_master_bus_handle_t shared_i2c_bus = NULL;
    {
        i2c_master_bus_config_t bus_cfg = {};
        bus_cfg.i2c_port          = 0;
        bus_cfg.sda_io_num        = (gpio_num_t)I2C_SDA_PIN;
        bus_cfg.scl_io_num        = (gpio_num_t)I2C_SCL_PIN;
        bus_cfg.clk_source        = I2C_CLK_SRC_DEFAULT;
        bus_cfg.glitch_ignore_cnt = 7;
        bus_cfg.flags.enable_internal_pullup = true;
        esp_err_t bus_err = i2c_new_master_bus(&bus_cfg, &shared_i2c_bus);
        if (bus_err != ESP_OK) {
            ESP_LOGE(TAG, "Shared I2C bus init failed: %s -- "
                          "falling back to per-component buses (touch "
                          "will be disabled to avoid conflict)",
                     esp_err_to_name(bus_err));
            shared_i2c_bus = NULL;
        } else {
            ESP_LOGI(TAG, "Shared I2C bus created (port 0, SDA=%d SCL=%d)",
                     I2C_SDA_PIN, I2C_SCL_PIN);
        }
    }
    display_set_shared_i2c_bus(shared_i2c_bus);
#endif
#if defined(CONFIG_DRAFTLING_DISPLAY_MIPI_DSI)
    /* M5Stack Tab5 shares its on-board I2C bus (SDA=31, SCL=32)
     * between the PI4IOE5V6408 I/O expander (LCD_EN, TOUCH_EN, ...),
     * the GT911 / ST7123 touch controller, the BMI270 IMU and the
     * audio codec. The espressif/m5stack_tab5 BSP creates and owns
     * the driver-NG bus inside bsp_i2c_init(); the touchscreen
     * component picks it up further below via bsp_i2c_get_handle()
     * + touchscreen_config_t.i2c_bus. Call bsp_i2c_init() once
     * here, before display_init(), so the panel auto-detect probe
     * inside the BSP can issue the read on a ready bus. */
    {
        esp_err_t bus_err = bsp_i2c_init();
        if (bus_err != ESP_OK) {
            ESP_LOGE(TAG, "bsp_i2c_init failed: %s", esp_err_to_name(bus_err));
        }
    }

    /* M5Stack Tab5 GT911 I2C address selection.
     *
     * The GT911 latches its 7-bit I2C address from the level on its
     * INT pin at the *rising edge* of its internal power-on reset:
     *   INT low  during reset -> backup  address 0x14
     *   INT high during reset -> primary address 0x5D
     *
     * The Tab5 has a hardware pull-up to 3V3 on the GT911 INT
     * (ESP32-P4 GPIO23). If we leave that pull-up to win, the
     * GT911 latches 0x5D, but the BSP only ever probes the backup
     * 0x14 (and our own TOUCH_I2C_ADDR is 0x14 to match) so touch
     * is effectively dead.
     *
     * Crucially, on this board the GT911 has NO dedicated RST line
     * exposed to the SoC -- the BSP sets `rst_gpio_num = GPIO_NUM_NC`
     * and `BSP_LCD_RST = GPIO_NUM_NC`. The only way to force the
     * GT911 to re-run its internal power-on reset (and re-latch
     * its I2C address) is to power-cycle the TOUCH_EN rail driven
     * by the first PI4IOE5V6408 I/O expander (I2C 0x43, pin P5 =
     * BSP_TOUCH_EN). On a cold boot the PI4IOE5V6408 starts with
     * every pin in high-impedance, so TOUCH_EN is effectively low
     * and the GT911 is unpowered until `bsp_feature_enable(TOUCH)`
     * brings it up inside `bsp_get_board_version()`. But on a warm
     * reboot (esp_restart, panic, watchdog, wake-from-reset) the
     * PI4IOE5V6408 retains its previous register state -- TOUCH_EN
     * is already high, the GT911 already latched 0x5D against the
     * INT pull-up before we ever got control, and any amount of
     * INT-low driving afterwards has no effect.
     *
     * So do both, in this order:
     *   (a) drive GPIO23 LOW as a push-pull output (override the
     *       3V3 pull-up at the SoC pin),
     *   (b) ask the BSP for the first PI4IOE5V6408 handle, switch
     *       BSP_TOUCH_EN (P5) to push-pull output, then drive it
     *       low for >=10 ms and high again, holding INT low across
     *       the whole edge so the GT911 latches 0x14.
     *
     * `touchscreen_init()` further down reconfigures GPIO23 to
     * INPUT + pull-up for the normal "data ready" signalling.
     * By then the GT911 has already latched its address, so the
     * 3V3 idle level on INT no longer matters.
     *
     * The subsequent `bsp_feature_enable(BSP_FEATURE_TOUCH, true)`
     * call inside `bsp_get_board_version()` (which is inside
     * `display_init()` below) is idempotent: it just rewrites P5
     * to the same `high` we already set here. */
#if defined(CONFIG_DRAFTLING_TOUCHSCREEN) && TOUCH_INT_PIN >= 0
    {
        /* (a) Hold INT low at the SoC pin. */
        gpio_config_t int_cfg = {};
        int_cfg.intr_type    = GPIO_INTR_DISABLE;
        int_cfg.mode         = GPIO_MODE_OUTPUT;
        int_cfg.pin_bit_mask = (1ULL << TOUCH_INT_PIN);
        int_cfg.pull_up_en   = GPIO_PULLUP_DISABLE;
        int_cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
        gpio_config(&int_cfg);
        gpio_set_level((gpio_num_t)TOUCH_INT_PIN, 0);

        /* (b) Power-cycle TOUCH_EN through the first PI4IOE5V6408
         * so the GT911 actually re-runs its internal POR and
         * latches the INT=low we just established. */
        esp_io_expander_handle_t pi4 = bsp_io_expander_init();
        if (pi4 == NULL) {
            ESP_LOGW(TAG, "Tab5 touch: bsp_io_expander_init returned NULL; "
                          "skipping GT911 power-cycle (touch may stay at 0x5D)");
        } else {
            esp_err_t err = esp_io_expander_set_dir(
                pi4, BSP_TOUCH_EN, IO_EXPANDER_OUTPUT);
            if (err == ESP_OK) {
                err = esp_io_expander_set_output_mode(
                    pi4, BSP_TOUCH_EN, IO_EXPANDER_OUTPUT_MODE_PUSH_PULL);
            }
            if (err == ESP_OK) {
                err = esp_io_expander_set_level(pi4, BSP_TOUCH_EN, 0);
            }
            if (err == ESP_OK) {
                /* GT911 datasheet: hold reset (here: VDD removed) for
                 * at least 10 ms to drain rail capacitance and ensure
                 * the internal POR actually fires on the next rise. */
                vTaskDelay(pdMS_TO_TICKS(20));
                err = esp_io_expander_set_level(pi4, BSP_TOUCH_EN, 1);
            }
            if (err == ESP_OK) {
                /* GT911 typical boot time is ~50 ms; the BSP itself
                 * waits 500 ms before probing 0x14. We just need the
                 * chip to have latched the address before we release
                 * the INT line as input later in touchscreen_init(),
                 * so a short delay here is enough -- the BSP's own
                 * 500 ms inside bsp_get_board_version() covers the
                 * rest. */
                vTaskDelay(pdMS_TO_TICKS(60));
                ESP_LOGI(TAG, "Tab5 touch: TOUCH_EN power-cycled with "
                              "INT held low (selects GT911 0x14 on v1; "
                              "v2 ST7123 at 0x55 ignores INT)");
            } else {
                ESP_LOGW(TAG, "Tab5 touch: TOUCH_EN power-cycle failed: %s",
                         esp_err_to_name(err));
            }
        }
    }
#endif

    /* M5Stack Tab5 battery charger enable (CHG_EN, P7 on the second
     * PI4IOE5V6408) is asserted later, AFTER `bsp_feature_enable()`
     * has had a chance to bring up the second I/O expander (its
     * driver's reset() routine wipes the output register and forces
     * every pin into high-impedance, which would clobber any CHG_EN
     * setting we did here). See the dedicated block further down,
     * immediately after the BSP_FEATURE_WIFI bring-up. */
#endif

#if defined(CONFIG_DRAFTLING_DISPLAY_RLCD)
    display_init(RLCD_MOSI_PIN, RLCD_SCK_PIN, RLCD_DC_PIN,
                 RLCD_CS_PIN, RLCD_RST_PIN, -1,
                 DISPLAY_WIDTH, DISPLAY_HEIGHT);
#elif defined(CONFIG_DRAFTLING_DISPLAY_EPDIY)
    /* vroland/epdiy backend. Owns all panel GPIOs via its board
     * definition (epdiy's built-in epd_board_v7 on the LilyGO T5
     * E-Paper S3 Pro / Pro Lite, the in-tree epd_board_papers3 on
     * the M5Stack PaperS3). Pin parameters are ignored. */
    display_init(-1, -1, -1, -1, -1, -1, DISPLAY_WIDTH, DISPLAY_HEIGHT);
#elif defined(CONFIG_DRAFTLING_DISPLAY_H752_EPD)
    /* Original LilyGO H752 EPD47 backend. Owns all panel GPIOs via
     * the vendored LilyGO shift-register driver. Pin parameters are
     * ignored. */
    display_init(-1, -1, -1, -1, -1, -1, DISPLAY_WIDTH, DISPLAY_HEIGHT);
#elif defined(CONFIG_DRAFTLING_DISPLAY_MIPI_DSI)
    /* M5Stack Tab5 MIPI-DSI panel. All panel GPIOs, the MIPI-DSI
     * PHY LDO, the I/O expander wiring and the LEDC backlight
     * channel are owned by the espressif/m5stack_tab5 BSP, which
     * also auto-detects board v1 (ILI9881C + GT911) vs board v2
     * (ST7123 + ST7123). display_init's pin parameters are ignored
     * by this backend. */
    display_init(-1, -1, -1, -1, -1, -1, DISPLAY_WIDTH, DISPLAY_HEIGHT);
#elif defined(CONFIG_DRAFTLING_DISPLAY_RGB)
    /* Parallel RGB565 color LCD (Sunton ESP32-8048S070C). All panel
     * data / control / backlight GPIOs are owned by the RGB backend
     * (components/display/display_rgb.cpp); pin parameters are
     * ignored, only width/height are used. */
    display_init(-1, -1, -1, -1, -1, -1, DISPLAY_WIDTH, DISPLAY_HEIGHT);
#elif defined(CONFIG_DRAFTLING_DISPLAY_AXS15231B)
    /* AXS15231B QSPI color LCD. Needs 9 GPIOs (CS/SCK/D0..D3/RST/TE/BL),
     * which do not fit in display_init()'s 6 pin slots, so the backend
     * exposes a dedicated struct-based init. */
    {
        display_axs15231b_config_t cfg = {};
        cfg.cs     = LCD_QSPI_CS_PIN;
        cfg.sck    = LCD_QSPI_SCK_PIN;
        cfg.d0     = LCD_QSPI_D0_PIN;
        cfg.d1     = LCD_QSPI_D1_PIN;
        cfg.d2     = LCD_QSPI_D2_PIN;
        cfg.d3     = LCD_QSPI_D3_PIN;
        cfg.rst    = LCD_RST_PIN;
        cfg.te     = LCD_TE_PIN;
        cfg.bl     = LCD_BL_PIN;
        cfg.width  = DISPLAY_WIDTH;
        cfg.height = DISPLAY_HEIGHT;
        /* The Guition JC3248W535's AXS15231B silicon ignores the
         * MADCTL MV (swap-XY) bit (same observation as Tactility's
         * driver for this board), so the backend has to software-
         * rotate at flush time to present the 320x480 portrait
         * panel as 480x320 landscape. Waveshare's Touch-LCD-3.49
         * is mounted with the same AXS15231B silicon but a 172x640
         * portrait panel; its reference firmware also software-
         * rotates to 640x172 landscape (USER_DISP_ROT_90), so we
         * take the same path. */
#if defined(CONFIG_DRAFTLING_MODEL_JC3248W535) || \
    defined(CONFIG_DRAFTLING_MODEL_WAVESHARE_TOUCH_LCD_349)
        cfg.swap_xy = true;
#else
        cfg.swap_xy = false;
#endif
        /* The hand-coded vendor-register block in display_axs15231b.cpp
         * was adapted from the Guition JC3248W535 reference and is
         * required to drive that 480x320 panel out of its weak factory
         * defaults. The Waveshare Touch-LCD-3.49's 172x640 panel
         * already ships with correct factory POR values -- the
         * upstream Espressif esp_lcd_axs15231b driver lets users
         * supply only {SLPOUT, DISPON} for that board, which is what
         * Waveshare's reference firmware does. Sending the JC3248W535
         * recipe to that panel clobbers its correct defaults and
         * leaves the display black on cold boot until the user
         * presses RESET. Skip the vendor block on Touch-LCD-3.49. */
#if defined(CONFIG_DRAFTLING_MODEL_WAVESHARE_TOUCH_LCD_349)
        cfg.skip_vendor_init = true;
#else
        cfg.skip_vendor_init = false;
#endif
        cfg.bl_deep_sleep_cut = LCD_BL_DEEP_SLEEP_CUT_PIN;
        cfg.bl_active_low     = (LCD_BL_ACTIVE_LOW != 0);
        display_axs15231b_init(&cfg);
    }
#endif

    /* Initialize LVGL.
     *
     * LVGL renders in *logical* pixels (panel size / DISPLAY_SCALE).
     * The display backend is responsible for scaling each logical
     * pixel to a SCALE x SCALE block of physical panel pixels. With
     * SCALE = 1 the logical and panel dimensions are identical. */
    ESP_LOGI(TAG, "Initializing LVGL...");
    draftling_lvgl_port_init(DISPLAY_LOGICAL_WIDTH, DISPLAY_LOGICAL_HEIGHT, DISPLAY_ROTATE);

    /* Initialize battery voltage monitor before the UI so the editor
     * status bar can show the battery level immediately. battery_init
     * is a no-op when BATT_ADC_PIN is < 0 (HAT case). */
    ESP_LOGI(TAG, "Initializing battery monitor...");
#if defined(CONFIG_DRAFTLING_BATTERY_BQ27220)
    /* LilyGO T5 E-Paper S3 Pro / Pro Lite: BQ27220YZFR fuel gauge
     * lives on the same I2C bus we already created above for epdiy
     * + GT911. The ADC backend has no pin (BATT_ADC_PIN == -1) on
     * these boards; battery_init() would early-return, so we drive
     * the BQ27220 backend directly. */
    if (battery_init_bq27220(shared_i2c_bus) != 0) {
        ESP_LOGW(TAG, "BQ27220 fuel gauge init failed; "
                      "falling back to ADC backend");
        battery_init(BATT_ADC_PIN, BATT_EN_PIN, BATT_DIVIDER);
    }
#elif defined(CONFIG_DRAFTLING_BATTERY_INA226)
    /* M5Stack Tab5: TI INA226 current/voltage monitor at I2C 0x41 on
     * the shared system bus owned by the espressif/m5stack_tab5 BSP
     * (bsp_i2c_init() is run earlier by display_init() via the MIPI
     * DSI backend). Battery pack is a 2S NP-F550 (~6.0-8.4 V); the
     * INA226 backend divides bus voltage by cell count before
     * looking up the per-cell SoC table. */
    if (battery_init_ina226(bsp_i2c_get_handle(), 0x41, 2) != 0) {
        ESP_LOGW(TAG, "INA226 init failed; battery indicator disabled");
    }
#else
    battery_init(BATT_ADC_PIN, BATT_EN_PIN, BATT_DIVIDER);
#endif

#if defined(CONFIG_DRAFTLING_CHARGER_BQ25896)
    /* LilyGO T5 E-Paper S3 Pro / Pro Lite: the on-board BQ25896
     * charger powers up with USB-SDP-class settings (500 mA IINLIM,
     * AUTO_DPDM re-detecting on every plug-in, 40 s I2C watchdog
     * that reverts host writes) and would otherwise charge the cell
     * at ~500 mA regardless of the wall adapter rating. Lift the
     * input current limit, raise the fast-charge current and
     * disable the watchdog so the settings persist. Uses the same
     * shared I2C bus as the BQ27220 fuel gauge (chip at 0x6B). */
    if (battery_init_bq25896(shared_i2c_bus) != 0) {
        ESP_LOGW(TAG, "BQ25896 charger init failed; "
                      "device will charge at reduced rate");
    }
#endif

#if defined(CONFIG_DRAFTLING_DEBUG_POWER)
    /* Boot-time fuel-gauge dump + (if this boot was a wake from
     * deep sleep) drain-rate estimate vs the SOC we persisted on
     * the way into sleep. Cheap (two I2C reads + one NVS read);
     * Kconfig-gated so production builds stay quiet. */
    log_boot_power_telemetry();
#endif
#if defined(CONFIG_DRAFTLING_DEBUG_POWER) || \
    defined(CONFIG_DRAFTLING_CHARGER_WATCHDOG_REASSERT)
    /* Periodic BQ25896 maintenance task (30 s):
     *   - re-asserts ICO_EN=0 + watchdog disabled (tamper-resistance)
     *   - logs full charger status when VBUS is present (DEBUG_POWER)
     * No-op when the chip is not present (the helper API early-exits). */
    charger_maint_start();
#endif

    /* Create editor UI */
    ESP_LOGI(TAG, "Creating editor UI...");
    if (draftling_lvgl_port_lock(-1)) {
        editor_ui_init();
        draftling_lvgl_port_unlock();
    }

    /* Initialize SD card */
    ESP_LOGI(TAG, "Initializing SD card...");
    esp_err_t sd_ret = ESP_FAIL;
#if defined(CONFIG_DRAFTLING_DISPLAY_EPDIY)
    /* Hold the LVGL port lock across the whole SD mount sequence on
     * epdiy e-paper boards (M5Stack PaperS3, LilyGO T5 E-Paper S3 Pro
     * / Pro Lite).
     *
     * When NO card is inserted, sd_card_init_spi()'s retry ladder
     * spends up to ~800 ms in its slowest attempt (1 MHz), keeping
     * core 0 busy servicing the SPI3 ISR and the sdspi command
     * timeouts. The epdiy LCD render path feeds pixel lines through
     * the ESP32-S3 LCD peripheral on a tight ~25 us-per-line schedule
     * driven by the LCD VSYNC and GDMA end-of-frame interrupts (also
     * on core 0). If an EPD flush starts while that slow probing is
     * hammering core 0, the frame timing is disrupted, the LCD state
     * machine desyncs, and both epdiy "epd_prep" feeder tasks
     * busy-spin forever at top priority -- starving IDLE0 and tripping
     * the task watchdog. The symptom appears only with no card,
     * because a present card mounts on the first 20 MHz attempt and an
     * EPD flush never overlaps a slow attempt.
     *
     * EPD flushes run from the LVGL task inside lv_timer_handler(),
     * which holds this same lock (see lvgl_port.cpp). Taking the lock
     * here lets any in-progress flush finish, then blocks new flushes
     * for the duration of the mount, making SD probing and EPD
     * refreshes mutually exclusive. (NOTE: the SPI master allocates a
     * dedicated, non-shared interrupt, so the earlier "SD mount frees
     * epdiy's shared SPI IRQ" theory does not hold; this serialization
     * is the actual fix.) */
    bool sd_lvgl_held = draftling_lvgl_port_lock(-1);
#endif
#if defined(CONFIG_DRAFTLING_SD_SDMMC)
    /* Boards with the SD slot wired to the on-chip SDMMC peripheral
     * (Waveshare RLCD-4.2). Uses 1-bit mode to keep the pin count
     * down. */
    sd_ret = sd_card_init(SD_CLK_PIN, SD_CMD_PIN, SD_D0_PIN, SD_MOUNT_POINT);
#else
#if defined(BOARD_LORA_CS_PIN) && (BOARD_LORA_CS_PIN >= 0)
    /* LilyGO T5 E-Paper S3 Pro: the on-board MicroSD slot shares its
     * SPI bus with the SX1262 LoRa radio. If the LoRa CS line is
     * left floating, SD init can fail intermittently because the
     * LoRa chip latches onto SPI traffic intended for the SD card
     * (LilyGO T5S3-4.7-e-paper-PRO issue #3). Drive LoRa CS HIGH
     * here -- on the Pro Lite variant the LoRa silicon is
     * depopulated, so this is just a harmless drive on an
     * unconnected output. */
    {
        gpio_config_t lora_cs = {};
        lora_cs.intr_type    = GPIO_INTR_DISABLE;
        lora_cs.mode         = GPIO_MODE_OUTPUT;
        lora_cs.pin_bit_mask = (1ULL << BOARD_LORA_CS_PIN);
        gpio_config(&lora_cs);
        gpio_set_level((gpio_num_t)BOARD_LORA_CS_PIN, 1);
    }
#endif
    /* Every other supported board carries the SD card on a generic
     * SPI bus separate from the display:
     *   - PaperS3:                on-board MicroSD on SPI3
     *   - AXS15231B color LCDs:   SPI3 (display owns SPI2 QSPI)
     *   - LilyGO T5 E-Paper S3 Pro: on-board MicroSD on SPI3 (shared
     *     with the SX1262 LoRa radio CS; we drive LoRa CS HIGH above
     *     so the radio does not snoop the SD traffic).
     * sd_card_init_spi() returns gracefully if no card is present. */
    sd_ret = sd_card_init_spi(SPI3_HOST,
                              SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_SCK_PIN,
                              SD_SPI_CS_PIN, SD_EN_PIN,
                              SD_MOUNT_POINT);
#endif
    if (sd_ret != ESP_OK) {
        ESP_LOGE(TAG, "SD card init failed: %s", esp_err_to_name(sd_ret));
        if (draftling_lvgl_port_lock(-1)) {
            editor_ui_set_status("ERROR: SD card not ready");
            draftling_lvgl_port_unlock();
        }
    }
#if defined(CONFIG_DRAFTLING_DISPLAY_EPDIY)
    /* Release the LVGL port lock taken before the mount sequence. The
     * pending "ERROR: SD card not ready" status (or the editor's
     * normal first refresh) is flushed by the LVGL task only now that
     * SD probing has finished, so the flush can never overlap it. */
    if (sd_lvgl_held) {
        draftling_lvgl_port_unlock();
    }
#endif

#if defined(CONFIG_DRAFTLING_MODEL_LILYGO_T5_EPD_S3_PRO) || \
    defined(CONFIG_DRAFTLING_MODEL_LILYGO_T5_EPD_S3_PRO_H752)
    /* LilyGO T5 E-Paper S3 Pro / Pro Lite: this firmware does not use
     * the SX1262 LoRa radio or the MIA-M10Q GPS receiver. Both come
     * up live after POR (SX1262 in STBY_RC ~600 uA, MIA-M10Q in full
     * acquisition mode ~25 mA), which would burn ~25 mA on the
     * battery for the entire active session before standby kicks in.
     * Park them in their lowest-power state right now -- SPI3 has
     * just been initialised by sd_card_init_spi() above, which is
     * the bus the LoRa sleep helper needs. pre_sleep_t5_deinit()
     * re-issues the same commands before deep sleep so a peripheral
     * that was somehow re-armed in between still gets parked. */
    t5_disable_radios_at_boot();
#endif

    /* Bring up the ESP-Hosted SDIO link to the on-board ESP32-C6
     * co-processor before Bluetooth or Wi-Fi. The C6 provides the
     * BLE controller (attached to Bluedroid via VHCI in
     * ble_keyboard.cpp) and the 2.4 GHz Wi-Fi radio (used through
     * esp_wifi_remote by wifi_manager.cpp).
     *
     * esp_hosted_init() / esp_hosted_connect_to_slave() return
     * success even when the SDIO slave does not respond (the
     * underlying transport_drv_reconfigure() failure is logged via
     * ESP_ERROR_CHECK_WITHOUT_ABORT and swallowed). We therefore
     * subscribe to the ESP_HOSTED_EVENT_TRANSPORT_UP event and
     * require it to fire within a short timeout before continuing.
     * If it doesn't, the most common cause is that the on-board
     * ESP32-C6 is not flashed with the matching ESP-Hosted slave
     * firmware (see docs/tab5-esp-hosted.md). In that case we show a
     * persistent on-screen message and halt instead of letting later
     * BLE/Wi-Fi initialisation abort the firmware. */
#if defined(CONFIG_ESP_HOSTED_ENABLED)
    {
#if defined(CONFIG_DRAFTLING_DISPLAY_MIPI_DSI)
        /* On the M5Stack Tab5 the ESP32-C6 co-processor's 3V3 rail is
         * gated by WLAN_PWR_EN (PI4IOE5V6408 #2, address 0x44, P0).
         * Until this pin is driven HIGH the C6 has no power, so the
         * host's reset pulse on GPIO15 below targets a dead chip and
         * the SDIO handshake never completes. Drive the rail high via
         * the espressif/m5stack_tab5 BSP and wait briefly for the C6
         * LDO to settle and the ROM bootloader to come up. bsp_i2c_init()
         * has already been called above (display path), and
         * bsp_feature_enable() is idempotent.
         *
         * A 50 ms post-enable delay was empirically insufficient on cold
         * boot (SDIO CMD5 then times out with sdmmc_init_ocr: send_op_cond
         * returned 0x107 even though the host pulses Slave_Reset on GPIO15
         * afterwards). 200 ms gives the C6 LDO + CHIP_PU + ROM bootloader
         * comfortable headroom before SDIO probing starts. */
        esp_err_t pwr_err = bsp_feature_enable(BSP_FEATURE_WIFI, true);
        if (pwr_err != ESP_OK) {
            ESP_LOGE(TAG, "bsp_feature_enable(BSP_FEATURE_WIFI) failed: %s",
                     esp_err_to_name(pwr_err));
        } else {
            ESP_LOGI(TAG, "ESP32-C6 power rail (WLAN_PWR_EN) enabled");
        }
        vTaskDelay(pdMS_TO_TICKS(200));

#if defined(CONFIG_DRAFTLING_MODEL_M5STACK_TAB5)
        /* M5Stack Tab5: enable battery charging from the USB-C input.
         *
         * The Tab5 carries an IP2326 autonomous Li-ion charger gated
         * by the CHG_EN signal, which is wired to P7 of the second
         * PI4IOE5V6408 I/O expander (I2C address 0x44). The
         * espressif/m5stack_tab5 BSP does not expose the charger pin
         * in `bsp_feature_enable()`, and M5Stack's reference
         * `bsp_io_expander_pi4ioe_init()` deliberately boots with
         * CHG_EN = 0, which is why the battery never charges when
         * USB is plugged in.
         *
         * This block must run AFTER `bsp_feature_enable(BSP_FEATURE_WIFI)`
         * above. That call goes through `bsp_io_expander1_init()`,
         * which constructs the espressif/esp_io_expander_pi4ioe5v6408
         * driver; that driver's reset() routine issues a chip-reset
         * and then re-writes the default register values
         * (OUT_SET = 0x00, OUT_H_IM = 0xFF, IO_DIR = 0xFF), which
         * would clobber any direct I2C write we did before the BSP
         * initialised the expander.
         *
         * We deliberately use the BSP-managed `esp_io_expander_handle_t`
         * (rather than raw I2C writes) so the driver's cached shadow
         * registers stay in sync with the chip. Otherwise a later
         * BSP `esp_io_expander_set_level()` call on any OTHER pin
         * (the BSP performs read-modify-write on its own shadow,
         * never reading back the chip) would overwrite CHG_EN
         * because the shadow still believes P7 is 0.
         *
         * Switching the pin to PUSH_PULL is required: the
         * PI4IOE5V6408 powers up with every pin in high-impedance
         * (OUT_H_IM = 0xFF), and the driver's reset() restores that
         * default. Even with IO_DIR = output and OUT_SET = 1 the
         * pin would otherwise stay floating and the IP2326 would
         * never see CHG_EN go high.
         */
        {
            esp_io_expander_handle_t pi4 = bsp_io_expander1_init();
            if (pi4 == NULL) {
                ESP_LOGW(TAG, "Tab5 CHG_EN: bsp_io_expander1_init returned NULL");
            } else {
                esp_err_t err = esp_io_expander_set_dir(
                    pi4, IO_EXPANDER_PIN_NUM_7, IO_EXPANDER_OUTPUT);
                if (err == ESP_OK) {
                    err = esp_io_expander_set_output_mode(
                        pi4, IO_EXPANDER_PIN_NUM_7,
                        IO_EXPANDER_OUTPUT_MODE_PUSH_PULL);
                }
                if (err == ESP_OK) {
                    err = esp_io_expander_set_level(
                        pi4, IO_EXPANDER_PIN_NUM_7, 1);
                }
                if (err == ESP_OK) {
                    ESP_LOGI(TAG, "Tab5 CHG_EN asserted (PI4IOE5V6408 #2 P7 = 1)");
                } else {
                    ESP_LOGW(TAG, "Tab5 CHG_EN setup failed: %s",
                             esp_err_to_name(err));
                }
            }
        }
#endif /* CONFIG_DRAFTLING_MODEL_M5STACK_TAB5 */
#endif
        ESP_LOGI(TAG, "Initializing ESP-Hosted link to ESP32-C6...");

        /* esp_hosted_init() posts ESP_HOSTED_EVENT_TRANSPORT_UP via
         * the default event loop, so it must exist before we call
         * into ESP-Hosted. wifi_manager_init() creates it later; here
         * we just ensure it's already up. ESP_ERR_INVALID_STATE means
         * "already created", which is fine. */
        esp_err_t loop_err = esp_event_loop_create_default();
        if (loop_err != ESP_OK && loop_err != ESP_ERR_INVALID_STATE) {
            ESP_LOGE(TAG, "esp_event_loop_create_default failed: %s",
                     esp_err_to_name(loop_err));
        }

        static EventGroupHandle_t s_hosted_evt = NULL;
        s_hosted_evt = xEventGroupCreate();
        constexpr EventBits_t HOSTED_UP_BIT = BIT0;

        auto hosted_event_cb = [](void *arg, esp_event_base_t /*base*/,
                                  int32_t id, void * /*data*/) {
            if (id == ESP_HOSTED_EVENT_TRANSPORT_UP) {
                xEventGroupSetBits(reinterpret_cast<EventGroupHandle_t>(arg),
                                   HOSTED_UP_BIT);
            }
        };

        esp_err_t reg_err = esp_event_handler_register(
            ESP_HOSTED_EVENT, ESP_EVENT_ANY_ID,
            hosted_event_cb, s_hosted_evt);
        if (reg_err != ESP_OK) {
            ESP_LOGE(TAG, "esp_event_handler_register(ESP_HOSTED_EVENT) "
                          "failed: %s", esp_err_to_name(reg_err));
        }

        int hosted_err = esp_hosted_init();
        if (hosted_err == 0) {
            hosted_err = esp_hosted_connect_to_slave();
        }

        /* Wait up to 5 seconds for the SDIO transport to come up. The
         * normal handshake completes in well under a second on a
         * healthy Tab5; 5 s leaves generous margin for slower retries
         * without keeping the user staring at an empty screen for
         * long. */
        const TickType_t timeout = pdMS_TO_TICKS(5000);
        EventBits_t bits = xEventGroupWaitBits(
            s_hosted_evt, HOSTED_UP_BIT,
            pdFALSE /* clear */, pdTRUE /* wait all */, timeout);

        esp_event_handler_unregister(ESP_HOSTED_EVENT, ESP_EVENT_ANY_ID,
                                     hosted_event_cb);

        if ((bits & HOSTED_UP_BIT) == 0) {
            ESP_LOGE(TAG, "ESP-Hosted link did not come up "
                          "(hosted_err=%d). The on-board ESP32-C6 is "
                          "probably not flashed with the matching "
                          "ESP-Hosted slave firmware. Halting.",
                     hosted_err);
            const char *fatal_msg =
                "ESP32-C6 not responding.\n\n"
                "The on-board ESP32-C6 co-processor is not "
                "responding over the ESP-Hosted SDIO link. "
                "This usually means it has not been flashed "
                "with the matching ESP-Hosted slave firmware.\n\n"
                "Please flash the ESP32-C6 with the ESP-Hosted "
                "slave firmware, then power-cycle the device.\n\n"
                "See docs/tab5-esp-hosted.md for instructions.";
            if (draftling_lvgl_port_lock(-1)) {
                editor_ui_show_fatal(fatal_msg);
                draftling_lvgl_port_unlock();
            }
            /* Stop here. Do not proceed to ble_keyboard_init() /
             * wifi_manager / etc.: those abort the firmware when the
             * hosted link is down, which would wipe the message we
             * just put on screen. */
            while (true) {
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        }
        ESP_LOGI(TAG, "ESP-Hosted link up");
    }
#endif

    /* Bring up the M5Stack Tab5 attachable keyboard (I2C + GPIO INT).
     * Probed once here: if the keyboard is attached it is initialised
     * in HID interrupt mode and feeds the editor alongside any USB
     * keyboard; if it is absent the driver goes idle and issues no
     * further I2C traffic this boot. Runs before the USB probe so the
     * combined "is any wired keyboard present?" decision below can
     * suppress the automatic BLE scan when local input already
     * exists. */
    bool tab5_kbd_present = false;
#if defined(CONFIG_DRAFTLING_HAS_TAB5_KBD)
    ESP_LOGI(TAG, "Probing for Tab5 keyboard...");
    /* The Tab5 keyboard sits on its own I2C bus (SDA/SCL configured
     * via CONFIG_DRAFTLING_TAB5_KBD_I2C_SDA_GPIO / _SCL_GPIO), not on
     * the shared system bus (touch / IMU / codec). Create a dedicated
     * master bus for it here. The system bus owned by bsp_i2c_init()
     * already holds one controller, so let ESP-IDF auto-pick a free
     * port (i2c_port = -1) instead of hard-coding one and colliding. */
    i2c_master_bus_handle_t tab5_kbd_bus = NULL;
    {
        i2c_master_bus_config_t kbd_bus_cfg = {};
        kbd_bus_cfg.i2c_port          = -1;
        kbd_bus_cfg.sda_io_num        = (gpio_num_t)CONFIG_DRAFTLING_TAB5_KBD_I2C_SDA_GPIO;
        kbd_bus_cfg.scl_io_num        = (gpio_num_t)CONFIG_DRAFTLING_TAB5_KBD_I2C_SCL_GPIO;
        kbd_bus_cfg.clk_source        = I2C_CLK_SRC_DEFAULT;
        kbd_bus_cfg.glitch_ignore_cnt = 7;
        kbd_bus_cfg.flags.enable_internal_pullup = true;
        esp_err_t kbd_bus_err = i2c_new_master_bus(&kbd_bus_cfg, &tab5_kbd_bus);
        if (kbd_bus_err != ESP_OK) {
            ESP_LOGE(TAG, "Tab5 keyboard I2C bus init failed: %s",
                     esp_err_to_name(kbd_bus_err));
            tab5_kbd_bus = NULL;
        }
    }
    if (tab5_kbd_bus &&
        tab5_kbd_init(tab5_kbd_bus,
                      CONFIG_DRAFTLING_TAB5_KBD_INT_GPIO) == 0) {
        tab5_kbd_present = true;
    }
#endif

    /* Bring up USB Host + HID keyboard driver (Tab5: USB-A port).
     * If a USB keyboard is enumerated within the probe window we
     * disable BLE so the wired keyboard is the sole input source;
     * if the user later unplugs the USB keyboard, the disconnect
     * handler in usb_kbd.cpp calls ble_keyboard_enable() and BLE
     * resumes scanning. BLE is brought up unconditionally below
     * so this hand-off works in both directions.
     */
    bool usb_kbd_present = false;
#if defined(CONFIG_DRAFTLING_HAS_USB_HOST)
    ESP_LOGI(TAG, "Starting USB host...");
    esp_err_t usb_err = bsp_usb_host_start(BSP_USB_HOST_POWER_MODE_USB_DEV,
                                            true /* limit to 500 mA */);
    if (usb_err != ESP_OK) {
        ESP_LOGW(TAG, "bsp_usb_host_start failed: %s",
                 esp_err_to_name(usb_err));
    } else if (usb_kbd_init() == 0) {
        const int probe_ms = CONFIG_DRAFTLING_USB_KBD_PROBE_MS;
        ESP_LOGI(TAG, "Probing for USB keyboard (%d ms)...", probe_ms);
        /* Poll every 50 ms so we exit as soon as enumeration
         * completes rather than always waiting the full window. */
        for (int waited = 0; waited < probe_ms; waited += 50) {
            if (usb_kbd_is_connected()) break;
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        usb_kbd_present = usb_kbd_is_connected();
    }
#endif

    /* Initialize Bluetooth keyboard.
     *
     * We always bring BLE up so it can take over as soon as the
     * user unplugs the USB keyboard (the USB disconnect handler in
     * usb_kbd.cpp calls ble_keyboard_enable() which restarts
     * scanning) or chooses "BLE: Start scan" from the menu. When a
     * wired keyboard (USB or the Tab5 attachable keyboard) is
     * already present at boot we initialise BLE and immediately
     * disable it; the BT controller stays up but the host stops
     * scanning and stops dispatching key events / status text until
     * it is re-enabled. */
    bool wired_kbd_present = usb_kbd_present || tab5_kbd_present;
    if (wired_kbd_present) {
        if (usb_kbd_present) {
            ESP_LOGI(TAG, "USB keyboard detected; BLE will stay idle "
                          "until the USB keyboard is unplugged");
        } else {
            ESP_LOGI(TAG, "Tab5 keyboard detected; BLE stays idle "
                          "(connect on demand via the menu)");
        }
        /* editor_ui_init() left the "Searching for BLE keyboard..."
         * prompt screen active because at that point we did not yet
         * know whether a local keyboard would be present. Jump
         * straight to the file browser; the BLE connect callback will
         * not fire while BLE is disabled. */
        if (draftling_lvgl_port_lock(-1)) {
            editor_ui_show_file_browser();
            draftling_lvgl_port_unlock();
        }
        ble_keyboard_init();
        ble_keyboard_disable();
    } else {
        ESP_LOGI(TAG, "Initializing Bluetooth keyboard...");
        /* Now that we know we are going down the BLE path, replace
         * the neutral "Initializing..." caption editor_ui_init()
         * put on the boot screen with the actual "Searching for BLE
         * keyboard..." prompt. Doing it here (rather than in
         * editor_ui_init) keeps that message off the screen on the
         * USB-keyboard path. */
        if (draftling_lvgl_port_lock(-1)) {
            editor_ui_set_ble_prompt_text(
                "Searching for BLE keyboard...\nPlease turn on your keyboard");
            draftling_lvgl_port_unlock();
        }
        ble_keyboard_init();
    }

#if defined(CONFIG_DRAFTLING_TOUCHSCREEN)
    /* Initialize the touchscreen and register the LVGL pointer
     * indev. Must run after the display (LVGL needs the default
     * display) and before standby_init (standby queries the touch
     * INT GPIO when CONFIG_DRAFTLING_STANDBY_WAKE_ON_TOUCH is set).
     *
     * Touch native orientation and mirror flags come from
     * app_config.h's per-board block (TOUCH_NATIVE_W / _H /
     * TOUCH_SWAP_XY / TOUCH_MIRROR_X / TOUCH_MIRROR_Y), so this
     * code stays board-agnostic. logical_width / logical_height
     * are the DISPLAY_SCALE-aware logical pixel counts that LVGL
     * itself uses.
     *
     * On the LilyGO T5 E-Paper S3 Pro / Pro Lite the GT911 sits on
     * the same physical I2C bus as epdiy's PCA9535 + TPS65185, and
     * ESP-IDF allows only one driver-NG i2c_new_master_bus() per
     * port. We therefore reuse the `shared_i2c_bus` handle created
     * above and hand it to the touchscreen component via
     * tcfg.i2c_bus; on every other board tcfg.i2c_bus stays NULL
     * and the component creates its own bus from sda/scl as before. */
    ESP_LOGI(TAG, "Initializing touchscreen...");
    {
        touchscreen_config_t tcfg = {};
        tcfg.sda      = I2C_SDA_PIN;
        tcfg.scl      = I2C_SCL_PIN;
        tcfg.rst      = TOUCH_RST_PIN;
        tcfg.intr     = TOUCH_INT_PIN;
        tcfg.i2c_addr = TOUCH_I2C_ADDR;
        tcfg.i2c_port = 0;
        tcfg.i2c_hz   = 400000;
        tcfg.native_width   = TOUCH_NATIVE_W;
        tcfg.native_height  = TOUCH_NATIVE_H;
        /* All boards (including the Tab5 BSP-delegated path) feed
         * LVGL indev coordinates in the *pre-rotation* logical pixel
         * frame -- i.e. (DISPLAY_LOGICAL_WIDTH, DISPLAY_LOGICAL_HEIGHT),
         * the same dimensions handed to lv_display_create() in
         * draftling_lvgl_port_init().
         *
         * LVGL v9.2 itself applies the display rotation to indev
         * points inside indev_pointer_proc() (using disp->hor_res /
         * disp->ver_res, which stay at their unrotated values even
         * after lv_display_set_rotation()). If we also rotated the
         * touch coords in native_to_logical(), the two transforms
         * would stack and the cursor would land at the wrong spot
         * (a middle tap on Tab5 -- DISPLAY_ROTATE=270 -- would map
         * to roughly 1/4 down from the top instead of the centre).
         * Pass user_rotate_deg=0 so the rotation happens exactly
         * once, inside LVGL. */
        tcfg.logical_width  = DISPLAY_LOGICAL_WIDTH;
        tcfg.logical_height = DISPLAY_LOGICAL_HEIGHT;
        tcfg.swap_xy  = TOUCH_SWAP_XY  ? true : false;
        tcfg.mirror_x = TOUCH_MIRROR_X ? true : false;
        tcfg.mirror_y = TOUCH_MIRROR_Y ? true : false;
        tcfg.user_rotate_deg = 0;
#if defined(CONFIG_DRAFTLING_DISPLAY_EPDIY) || defined(CONFIG_DRAFTLING_DISPLAY_H752_EPD)
        tcfg.i2c_bus = (void *)shared_i2c_bus;
#elif defined(CONFIG_DRAFTLING_DISPLAY_MIPI_DSI)
        /* The m5stack_tab5 BSP owns the I2C master bus (created
         * by bsp_i2c_init() before display_init() above); hand
         * its handle to the touchscreen component so we do not
         * collide on i2c_new_master_bus() for the same port. */
        tcfg.i2c_bus = (void *)bsp_i2c_get_handle();
#endif
        touchscreen_init(&tcfg);
#if defined(CONFIG_DRAFTLING_MODEL_LILYGO_T5_EPD_S3_PRO_H752)
        /* Front touch key below the panel = Back (Esc). */
        touchscreen_set_button_callback(h752_touch_button_cb);
#endif
    }
#endif

#if defined(CONFIG_DRAFTLING_MODEL_LILYGO_T5_EPD_S3_PRO_H752)
    /* Side key (GPIO48) = Menu (F1) while awake. */
    h752_user_key_init();
#endif

    /* WiFi is lazy-initialized on first wifi_manager_connect() call.
     * On boards with a tight internal heap (e.g. M5Stack PaperS3,
     * ~138 KB free) eagerly calling esp_wifi_init() here fails with
     * ESP_ERR_NO_MEM because the WiFi static RX/TX buffers (DMA-
     * capable, must live in internal RAM) cannot fit alongside the
     * Bluedroid stack that ble_keyboard_init() just brought up. */

    /* Initialize Git sync */
    ESP_LOGI(TAG, "Initializing Git sync...");
    git_sync_init();

    /* Initialize standby manager (deep sleep on inactivity) */
    ESP_LOGI(TAG, "Initializing standby manager...");
    standby_init();
#if defined(CONFIG_DRAFTLING_MODEL_LILYGO_T5_EPD_S3_PRO)
    standby_set_pre_sleep_cb(pre_sleep_t5_deinit);
#elif defined(CONFIG_DRAFTLING_MODEL_LILYGO_T5_EPD_S3_PRO_H752)
    standby_set_pre_sleep_cb(pre_sleep_h752_deinit);
#elif defined(CONFIG_DRAFTLING_MODEL_M5STACK_TAB5)
    standby_set_pre_sleep_cb(pre_sleep_tab5_deinit);
#else
    standby_set_pre_sleep_cb(pre_sleep_autosave);
#endif

#if defined(CONFIG_DRAFTLING_HAS_POWER_LATCH)
    /* On PWR long-press, run the full shutdown sequence via the
     * standby manager: auto-save (pre_sleep_cb above), backlight
     * cut, power-latch cut (battery: rail drops here; USB: no-op),
     * then esp_deep_sleep_start. This is the same sequence used on
     * inactivity timeout, so the behaviour is identical regardless
     * of which path triggers the shutdown. */
    power_set_long_press_cb(standby_enter_sleep);
#endif

    ESP_LOGI(TAG, "Draftling ready. Waiting for Bluetooth keyboard...");
}
