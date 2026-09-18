#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <esp_err.h>
#include <stdbool.h>

/*
 * Exposes the board's SD card as a USB Mass Storage device over the
 * ESP32-S3's native USB-OTG controller, for boards whose USB port
 * wires that controller straight to the connector (see the boards
 * listed under CONFIG_DRAFTLING_HAS_USB_MSC in Kconfig.projbuild).
 *
 * usb_msc does NOT unmount the FatFs volume sd_card.cpp keeps
 * registered at sd_card_get_mount_point(): tinyusb's SD/MMC storage
 * backend talks directly to sd_card_get_handle() at the raw-sector
 * level and never touches that mount. It is therefore safe to leave
 * the local mount in place while USB MSC is active PROVIDED the rest
 * of the firmware does not perform any file I/O on it during that
 * window -- editor_ui.cpp enforces that by forcing the file browser
 * closed to editing and refusing to open/create/sync files while
 * usb_msc_get_mode() is not USB_MSC_MODE_OFF.
 */
typedef enum {
    USB_MSC_MODE_OFF = 0,
    USB_MSC_MODE_READ_ONLY,
    USB_MSC_MODE_READ_WRITE,
} usb_msc_mode_t;

/*
 * Switches to the given mode.
 *
 *  OFF -> RO/RW:        installs the TinyUSB device + MSC drivers and
 *                        exposes sd_card_get_handle()'s sectors over
 *                        USB. Fails with ESP_ERR_INVALID_STATE if no
 *                        SD card is currently mounted.
 *  RO/RW -> OFF:         tears the USB MSC device back down.
 *  RO <-> RW:            tears down and re-installs the TinyUSB device
 *                        stack, forcing a real USB disconnect/reconnect,
 *                        so a host that already mounted the volume
 *                        re-reads the write-protect bit instead of
 *                        keeping whichever value it cached at first
 *                        mount.
 *
 * Also resets the "last host seen" clock used by the idle auto-off
 * timer (see usb_msc_idle_seconds_remaining()).
 *
 * IMPORTANT: a RO/RW -> OFF transition reclaims the internal FSLS PHY
 * for USB-Serial-JTAG (flashing, and usually the console too, on
 * these single-USB-port boards) as part of tearing down -- see
 * teardown_locked()'s comment in usb_msc.cpp for the RTCCNTL mux bit
 * involved and why that reclaim, not just tinyusb_driver_uninstall(),
 * is required. That reclaim has not been confirmed sufficient on its
 * own on physical hardware, so as a safety net callers that turn the
 * mode off while the device stays awake (the F1 menu, and usb_msc's
 * own idle auto-off timer) still esp_restart() right after a
 * successful call returns ESP_OK for OFF. The one exception is
 * turning it off immediately before entering deep sleep: the
 * sleep/wake cycle already resets every USB peripheral back to its
 * power-on state, so an extra restart there would only be a wasted
 * reboot (main.cpp's pre_sleep_autosave() relies on this and does
 * not restart).
 */
esp_err_t usb_msc_set_mode(usb_msc_mode_t mode);

usb_msc_mode_t usb_msc_get_mode(void);

/* true once a USB host has enumerated the device and for as long as
 * it stays attached. Always false when the mode is USB_MSC_MODE_OFF. */
bool usb_msc_is_host_connected(void);

/*
 * Seconds left before usb_msc automatically reverts to
 * USB_MSC_MODE_OFF because no host has been connected for
 * CONFIG_DRAFTLING_USB_MSC_IDLE_TIMEOUT_SEC seconds. Returns -1 when
 * the mode is USB_MSC_MODE_OFF or a host is currently connected (no
 * countdown running in either case).
 */
int usb_msc_idle_seconds_remaining(void);

#ifdef __cplusplus
}
#endif
