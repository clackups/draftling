#include "usb_msc.h"

/*
 * usb_msc.cpp is always compiled (see CMakeLists.txt for why), but is
 * only functional on the boards listed under CONFIG_DRAFTLING_HAS_USB_MSC
 * in Kconfig.projbuild -- those whose USB port wires the ESP32-S3's
 * native USB-OTG controller straight to the connector. Every other
 * board gets the no-op stubs at the bottom of this file instead.
 */
#if defined(CONFIG_DRAFTLING_HAS_USB_MSC)

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_timer.h>

#include "hal/usb_serial_jtag_hal.h"
#include "tinyusb_default_config.h"
#include "tinyusb_msc.h"

#include "sd_card.h"

static const char *TAG = "USBMsc";

static SemaphoreHandle_t s_lock = NULL;

/* volatile: usb_msc_get_mode() / tud_msc_is_writable_cb() read this
 * from other tasks (LVGL, TinyUSB) without taking s_lock -- writes
 * always happen with s_lock held, from usb_msc_set_mode() or the
 * idle timer, so a reader only ever observes a fully-written enum
 * value, never a torn one. */
static volatile usb_msc_mode_t    s_mode = USB_MSC_MODE_OFF;
static tinyusb_msc_storage_handle_t s_storage = NULL;
static volatile bool              s_host_connected = false;
/* Monotonic timestamp (esp_timer_get_time(), microseconds) of the
 * last moment no host was known to be attached -- i.e. either the
 * instant the current mode was activated (no host yet) or the
 * instant the host most recently detached. Reset to "now" on every
 * ATTACH->DETACH transition and on activation; left alone while
 * s_host_connected is true. */
static int64_t                    s_disconnected_since_us = 0;
static esp_timer_handle_t         s_idle_timer = NULL;

#define IDLE_CHECK_PERIOD_US (10 * 1000 * 1000)  /* 10 s */

static void device_event_cb(tinyusb_event_t *event, void *arg)
{
    (void)arg;
    switch (event->id) {
    case TINYUSB_EVENT_ATTACHED:
        s_host_connected = true;
        ESP_LOGI(TAG, "USB host attached");
        break;
    case TINYUSB_EVENT_DETACHED:
        s_host_connected = false;
        s_disconnected_since_us = esp_timer_get_time();
        ESP_LOGI(TAG, "USB host detached");
        break;
    default:
        break;
    }
}

/* Weak in TinyUSB core (class/msc/msc_device.c); this strong
 * definition is what makes USB_MSC_MODE_READ_ONLY actually reject
 * writes -- it gates both the SCSI MODE SENSE "write protected" bit
 * and WRITE10 itself. */
extern "C" bool tud_msc_is_writable_cb(uint8_t lun)
{
    (void)lun;
    return s_mode == USB_MSC_MODE_READ_WRITE;
}

/* Caller must hold s_lock. Tears down the TinyUSB MSC + device
 * drivers if they are currently installed, and hands the internal
 * FSLS PHY back to USB-Serial-JTAG (flashing, and usually the
 * console too, on these single-USB-port boards). Safe to call when
 * already torn down.
 *
 * The PHY hand-back is the part that is easy to get wrong. The
 * internal FSLS PHY -- and therefore the physical D+/D- pins -- is
 * muxed between the OTG controller (what tinyusb_driver_install()
 * just used) and the separate USB-Serial-JTAG controller by
 * RTCCNTL.usb_conf.sw_usb_phy_sel (see usb_wrap_ll_phy_enable_external()
 * / usb_serial_jtag_ll_phy_enable_external() in ESP-IDF's HAL, which
 * both write it). RTCCNTL lives in the always-on RTC power domain, so
 * unlike the OTG/USB-Serial-JTAG peripherals' own registers it is
 * NOT reset by esp_restart()'s software system reset -- only a true
 * power-on reset (a physical reset button press) clears it, which is
 * exactly the workaround that prompted this comment. Explicitly
 * calling usb_serial_jtag_hal_phy_set_external(NULL, false) mirrors
 * what usb_serial_jtag_driver_install() does at boot and re-points
 * that mux at USB-Serial-JTAG ourselves, in software, without
 * needing a reset for the PHY hand-back specifically.
 *
 * usb_msc_set_mode(OFF)'s callers still esp_restart() afterward
 * anyway (idle_timer_cb() below, and commit_usb_msc_pending_mode()
 * in editor_ui.cpp) as a safety net -- this reclaim has not been
 * confirmed sufficient on its own on physical hardware -- but a
 * clean reboot is a much smaller ask than "won't come back without
 * pressing the reset button" if it turns out not to be needed. */
static void teardown_locked(void)
{
    if (s_idle_timer) {
        esp_timer_stop(s_idle_timer);
    }
    if (s_storage) {
        /* tinyusb_msc_delete_storage() refuses to run (returns
         * ESP_ERR_INVALID_STATE, and does NOT free the storage or
         * flush anything) while a WRITE10 the host already sent is
         * still sitting in TinyUSB's own deferred-write queue -- see
         * msc_storage_write_sector_deferred() / tusb_write_func() in
         * esp_tinyusb's tinyusb_msc.c, and the "deferred writes are
         * pending" case in its header doc comment. There is no
         * separate flush/sync call; retrying this is the only way to
         * wait it out instead of silently dropping a write the host
         * may already believe completed (e.g. a file just copied
         * onto the drive, moments before the user picks Off). TinyUSB
         * services that queue from its own task almost immediately,
         * so this should not normally retry more than once or twice;
         * the bound below is a last resort against a wedged write,
         * not the expected path. */
        esp_err_t del_ret;
        int attempts = 0;
        const int kMaxAttempts = 20;  /* ~1 s at 50 ms each */
        while ((del_ret = tinyusb_msc_delete_storage(s_storage)) == ESP_ERR_INVALID_STATE &&
               ++attempts < kMaxAttempts) {
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        if (del_ret != ESP_OK) {
            ESP_LOGE(TAG, "tinyusb_msc_delete_storage failed after %d attempts: %s "
                          "-- a pending write may have been lost",
                     attempts, esp_err_to_name(del_ret));
        }
        s_storage = NULL;
    }
    tinyusb_msc_uninstall_driver();
    tinyusb_driver_uninstall();
    /* Reclaim the internal FSLS PHY for USB-Serial-JTAG now that OTG
     * (tinyusb_driver_uninstall(), just above) is fully torn down --
     * see this function's comment above for why this call is needed
     * at all. */
    usb_serial_jtag_hal_phy_set_external(NULL, false);
    s_host_connected = false;
}

static void idle_timer_cb(void *arg)
{
    (void)arg;
    if (xSemaphoreTake(s_lock, pdMS_TO_TICKS(1000)) != pdTRUE) return;

    if (s_mode != USB_MSC_MODE_OFF && !s_host_connected) {
        int64_t idle_us = esp_timer_get_time() - s_disconnected_since_us;
        int64_t timeout_us = (int64_t)CONFIG_DRAFTLING_USB_MSC_IDLE_TIMEOUT_SEC * 1000000LL;
        if (idle_us >= timeout_us) {
            ESP_LOGI(TAG, "No USB host for %ds -- disabling SD-via-USB and "
                          "restarting to restore USB-Serial-JTAG",
                     CONFIG_DRAFTLING_USB_MSC_IDLE_TIMEOUT_SEC);
            teardown_locked();
            /* esp_restart() never returns, so there is no need to set
             * s_mode or release s_lock first -- see teardown_locked()'s
             * comment for why the restart itself is required. Safe to
             * call from the esp_timer task like any other context. */
            esp_restart();
        }
    }

    xSemaphoreGive(s_lock);
}

static esp_err_t ensure_lock(void)
{
    if (s_lock) return ESP_OK;
    s_lock = xSemaphoreCreateMutex();
    return s_lock ? ESP_OK : ESP_ERR_NO_MEM;
}

/* Caller must hold s_lock and must have already torn down any prior
 * instance (teardown_locked()). Installs the TinyUSB device + MSC
 * drivers and creates a fresh storage instance bound to the SD card.
 * On failure, leaves everything torn down (s_mode is the caller's to
 * set either way). */
static esp_err_t bring_up_locked(void)
{
    sdmmc_card_t *card = sd_card_get_handle();
    if (!card || !sd_card_is_ready()) {
        ESP_LOGW(TAG, "No SD card mounted -- cannot enable SD-via-USB");
        return ESP_ERR_INVALID_STATE;
    }

    const tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG(device_event_cb, NULL);
    esp_err_t ret = tinyusb_driver_install(&tusb_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "tinyusb_driver_install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    const tinyusb_msc_driver_config_t msc_drv_cfg = {};
    ret = tinyusb_msc_install_driver(&msc_drv_cfg);
    if (ret != ESP_OK) {
        tinyusb_driver_uninstall();
        ESP_LOGE(TAG, "tinyusb_msc_install_driver failed: %s", esp_err_to_name(ret));
        return ret;
    }

    tinyusb_msc_storage_config_t storage_cfg = {};
    storage_cfg.medium.card = card;
    ret = tinyusb_msc_new_storage_sdmmc(&storage_cfg, &s_storage);
    if (ret != ESP_OK) {
        tinyusb_msc_uninstall_driver();
        tinyusb_driver_uninstall();
        ESP_LOGE(TAG, "tinyusb_msc_new_storage_sdmmc failed: %s", esp_err_to_name(ret));
        return ret;
    }

    s_host_connected = false;
    s_disconnected_since_us = esp_timer_get_time();

    if (!s_idle_timer) {
        const esp_timer_create_args_t timer_args = {
            .callback = &idle_timer_cb,
            .arg = NULL,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "usb_msc_idle",
            .skip_unhandled_events = false,
        };
        esp_timer_create(&timer_args, &s_idle_timer);
    }
    if (s_idle_timer) {
        esp_timer_start_periodic(s_idle_timer, IDLE_CHECK_PERIOD_US);
    }

    return ESP_OK;
}

extern "C" esp_err_t usb_msc_set_mode(usb_msc_mode_t mode)
{
    if (mode != USB_MSC_MODE_OFF && mode != USB_MSC_MODE_READ_ONLY &&
        mode != USB_MSC_MODE_READ_WRITE) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = ensure_lock();
    if (ret != ESP_OK) return ret;
    xSemaphoreTake(s_lock, portMAX_DELAY);

    if (mode == s_mode) {
        xSemaphoreGive(s_lock);
        return ESP_OK;
    }

    if (mode == USB_MSC_MODE_OFF) {
        teardown_locked();
        s_mode = USB_MSC_MODE_OFF;
        xSemaphoreGive(s_lock);
        return ESP_OK;
    }

    /* Activating fresh (s_mode was Off), or flipping between Read-only
     * and Read-write while a host may already have the volume mounted
     * (s_mode was the other non-Off value): either way, tear down
     * first and bring the TinyUSB device stack back up from scratch.
     * A host that already queried the write-protect bit (SCSI MODE
     * SENSE) only re-polls it on re-enumeration -- flipping s_mode
     * alone (what an earlier version of this function did) leaves an
     * already-mounted host treating the volume as stuck in whichever
     * mode it first saw until it is physically unplugged and
     * replugged. A full teardown + bring-up forces that
     * detach/reattach in software instead. */
    if (s_mode != USB_MSC_MODE_OFF) {
        teardown_locked();
    }

    ret = bring_up_locked();
    if (ret != ESP_OK) {
        s_mode = USB_MSC_MODE_OFF;
        xSemaphoreGive(s_lock);
        return ret;
    }

    s_mode = mode;
    ESP_LOGI(TAG, "SD-via-USB enabled (%s)",
             mode == USB_MSC_MODE_READ_WRITE ? "read-write" : "read-only");
    xSemaphoreGive(s_lock);
    return ESP_OK;
}

extern "C" usb_msc_mode_t usb_msc_get_mode(void)
{
    return s_mode;
}

extern "C" bool usb_msc_is_host_connected(void)
{
    return s_host_connected;
}

extern "C" int usb_msc_idle_seconds_remaining(void)
{
    if (s_mode == USB_MSC_MODE_OFF || s_host_connected) return -1;
    int64_t idle_us = esp_timer_get_time() - s_disconnected_since_us;
    int64_t timeout_us = (int64_t)CONFIG_DRAFTLING_USB_MSC_IDLE_TIMEOUT_SEC * 1000000LL;
    int64_t remain_us = timeout_us - idle_us;
    if (remain_us < 0) remain_us = 0;
    return (int)(remain_us / 1000000LL);
}

#else /* !CONFIG_DRAFTLING_HAS_USB_MSC */

extern "C" esp_err_t usb_msc_set_mode(usb_msc_mode_t mode)
{
    return (mode == USB_MSC_MODE_OFF) ? ESP_OK : ESP_ERR_NOT_SUPPORTED;
}

extern "C" usb_msc_mode_t usb_msc_get_mode(void)
{
    return USB_MSC_MODE_OFF;
}

extern "C" bool usb_msc_is_host_connected(void)
{
    return false;
}

extern "C" int usb_msc_idle_seconds_remaining(void)
{
    return -1;
}

#endif /* CONFIG_DRAFTLING_HAS_USB_MSC */
