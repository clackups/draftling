#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* Modifier key flags */
#define KB_MOD_LCTRL   0x01
#define KB_MOD_LSHIFT  0x02
#define KB_MOD_LALT    0x04
#define KB_MOD_LGUI    0x08
#define KB_MOD_RCTRL   0x10
#define KB_MOD_RSHIFT  0x20
#define KB_MOD_RALT    0x40
#define KB_MOD_RGUI    0x80

typedef enum {
    KB_KEY_NONE      = 0,
    KB_KEY_ENTER     = 0x28,
    KB_KEY_ESCAPE    = 0x29,
    KB_KEY_BACKSPACE = 0x2A,
    KB_KEY_TAB       = 0x2B,
    KB_KEY_SPACE     = 0x2C,
    KB_KEY_DELETE    = 0x4C,
    KB_KEY_RIGHT     = 0x4F,
    KB_KEY_LEFT      = 0x50,
    KB_KEY_DOWN      = 0x51,
    KB_KEY_UP        = 0x52,
    KB_KEY_HOME      = 0x4A,
    KB_KEY_END       = 0x4D,
    KB_KEY_PAGEUP    = 0x4B,
    KB_KEY_PAGEDOWN  = 0x4E,
    KB_KEY_F1        = 0x3A,
    KB_KEY_F2        = 0x3B,
    KB_KEY_F3        = 0x3C,
    KB_KEY_F4        = 0x3D,
    KB_KEY_F5        = 0x3E,
    KB_KEY_F6        = 0x3F,
    KB_KEY_F7        = 0x40,
    KB_KEY_F8        = 0x41,
    KB_KEY_F9        = 0x42,
    KB_KEY_F10       = 0x43,
    KB_KEY_F11       = 0x44,
    KB_KEY_F12       = 0x45,
    KB_KEY_KP_ENTER  = 0x58,
} kb_special_key_t;

typedef struct {
    uint8_t modifier;
    uint8_t keycode;
    char    character;   /* ASCII char, or 0 for special keys */
    bool    pressed;
} kb_event_t;

typedef void (*kb_event_callback_t)(const kb_event_t *event);

/* Passkey callback: called with a 6-digit passkey (0..999999)
 * for the user to enter on the keyboard during pairing, or with
 * BLE_PASSKEY_DISMISS when pairing completes / fails. */
#define BLE_PASSKEY_DISMISS  0xFFFFFFFFU
typedef void (*ble_passkey_cb_t)(uint32_t passkey);

/* Connection status callback: called with true on connect, false on
 * disconnect.  Invoked from the HID host task context. */
typedef void (*ble_connect_cb_t)(bool connected);

/* Status text callback: called with a human-readable status message
 * whenever the BLE keyboard connection state changes (scanning,
 * connecting, retrying, etc.).  The string is valid only for the
 * duration of the call. */
typedef void (*ble_status_text_cb_t)(const char *text);

void ble_keyboard_init(void);
/* Stop all BLE keyboard activity (scanning, reconnection timers,
 * status text updates) and disconnect from any currently paired
 * keyboard. Safe to call at any time, including before
 * ble_keyboard_init() (in which case it is a no-op).
 *
 * Used when a USB HID keyboard is hot-plugged: the BLE component
 * stops driving the radio and stops emitting key events / status
 * text so the wired keyboard becomes the sole input source.
 *
 * Reversible: call ble_keyboard_enable() to resume scanning after
 * the USB keyboard is unplugged. The registered callbacks
 * (key / passkey / connect / status) are preserved across a
 * disable/enable cycle so the editor keeps getting events from
 * whichever keyboard is currently active. */
void ble_keyboard_disable(void);
/* Resume BLE keyboard activity that was suspended by
 * ble_keyboard_disable(). Restarts scanning so the next BLE
 * keyboard the user powers on can pair / reconnect. No-op if BLE
 * is already enabled or has never been initialised. */
void ble_keyboard_enable(void);
/* Erase every stored keyboard bond and start scanning for a new
 * keyboard from scratch. Removes all bonded device entries from
 * the Bluedroid stack and from NVS, disconnects any currently-
 * connected keyboard, and immediately starts a fresh scan. */
void ble_keyboard_forget_all(void);
void ble_keyboard_set_callback(kb_event_callback_t callback);
void ble_keyboard_set_passkey_callback(ble_passkey_cb_t cb);
void ble_keyboard_set_connect_callback(ble_connect_cb_t cb);
void ble_keyboard_set_status_text_callback(ble_status_text_cb_t cb);
bool ble_keyboard_is_connected(void);
void ble_keyboard_start_scan(void);
const char *ble_keyboard_get_device_name(void);
int ble_keyboard_get_battery_level(void);

#ifdef __cplusplus
}
#endif
