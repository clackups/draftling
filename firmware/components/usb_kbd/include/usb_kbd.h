#pragma once

/*
 * USB HID keyboard host.
 *
 * Brings up the ESP-IDF USB Host stack + espressif/usb_host_hid HID
 * driver, opens any attached USB HID keyboard interface -- boot
 * protocol or a generic report-protocol interface whose descriptor
 * contains a standard keyboard collection (e.g. a composite
 * keyboard+mouse device built with TinyUSB) -- and translates its
 * reports into the same kb_event_t event stream emitted by the
 * ble_keyboard component.
 *
 * Designed as a sibling to ble_keyboard: callers wire the same
 * editor key-handler into both via *_set_callback(), and pick which
 * radio to bring up based on usb_kbd_is_connected() (USB keyboard
 * physically attached at boot time wins -- BLE pairing is not even
 * started).
 *
 * On ESP32-S3, usb_kbd_init() installs the USB Host stack and starts
 * its daemon task; the board must provide 5 V on host VBUS. On the
 * M5Stack Tab5, the BSP owns PHY, VBUS and daemon startup, and
 * main.cpp calls bsp_usb_host_start() first.
 */

#include "ble_keyboard.h"   /* kb_event_t, kb_event_callback_t */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Install the USB Host library + HID driver and start the internal
 * task that processes hot-plug events. On ESP32-S3 this initializes
 * the host stack; on Tab5, call after bsp_usb_host_start().
 *
 * Returns 0 on success, non-zero on failure.
 */
int  usb_kbd_init(void);

/*
 * True iff a USB HID keyboard interface has been opened and is
 * currently delivering reports. Set asynchronously by the HID-host
 * connection callback. Polled by main.cpp after a brief settle
 * delay to decide whether to skip BLE keyboard bring-up.
 */
bool usb_kbd_is_connected(void);

/*
 * Register a callback that receives the same kb_event_t stream
 * format as ble_keyboard_set_callback(). May be called before or
 * after usb_kbd_init(); subsequent calls replace the previous
 * callback.
 */
void usb_kbd_set_callback(kb_event_callback_t cb);

/*
 * Set the keyboard LEDs (KB_LED_* bits from ble_keyboard.h) on every
 * connected USB keyboard; remembered for keyboards plugged in later.
 * Non-blocking: the transfer runs on the internal HID event task.
 */
void usb_kbd_set_leds(uint8_t leds);

/*
 * Callback fired when a USB HID keyboard is connected (true) or
 * disconnected (false). Mirrors ble_keyboard_set_connect_callback()
 * so the editor UI can react to a late USB hot-plug (e.g. dismiss
 * the "Keyboard disconnected" prompt screen left behind by a
 * preceding BLE disconnect) without having to poll
 * usb_kbd_is_connected().
 *
 * Runs in the USB HID host event task -- do NOT block in the
 * callback.
 */
typedef void (*usb_kbd_connect_cb_t)(bool connected);
void usb_kbd_set_connect_callback(usb_kbd_connect_cb_t cb);

#ifdef __cplusplus
}
#endif
