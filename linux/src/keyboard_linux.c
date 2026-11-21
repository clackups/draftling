/*
 * Linux: the computer's keyboard stands in for the Bluetooth keyboard.
 *
 * Every backend reports physical keys as Linux input event codes
 * (KEY_*: Wayland sends them, X11 sends them plus 8, evdev is them).
 * They are turned into HID usage codes and the HID modifier byte, and
 * delivered through the callback the editor registered with
 * ble_keyboard_set_callback(), exactly as the BLE HID host does. So
 * the firmware's own keyboard layouts (Ctrl+L), dead keys, input
 * methods and key repeat apply, whatever layout the desktop has set:
 * the keyboard counts as a US-layout keyboard on the device, and the
 * layout is chosen in Draftling. Modifier keys only change the
 * modifier byte, as in a HID report; the computer's auto-repeat is
 * ignored (the editor repeats keys itself).
 *
 * The keyboard is "connected" from the start; the Bluetooth menu shows
 * it as the host keyboard, and "forget keyboards" / "scan" do nothing.
 */

#include <stdbool.h>
#include <string.h>

#include "ble_keyboard.h"
#include "editor_ui.h"
#include "esp_timer.h"
#include "linux_host.h"

#define HOST_KB_NAME "Computer keyboard"

/* HID usage -> Linux key code, the kernel's usbkbd table
 * (drivers/hid/usbhid/usbkbd.c), used in reverse. */
static const unsigned char s_hid_to_evdev[256] = {
      0,  0,  0,  0, 30, 48, 46, 32, 18, 33, 34, 35, 23, 36, 37, 38,
     50, 49, 24, 25, 16, 19, 31, 20, 22, 47, 17, 45, 21, 44,  2,  3,
      4,  5,  6,  7,  8,  9, 10, 11, 28,  1, 14, 15, 57, 12, 13, 26,
     27, 43, 43, 39, 40, 41, 51, 52, 53, 58, 59, 60, 61, 62, 63, 64,
     65, 66, 67, 68, 87, 88, 99, 70,119,110,102,104,111,107,109,106,
    105,108,103, 69, 98, 55, 74, 78, 96, 79, 80, 81, 75, 76, 77, 71,
     72, 73, 82, 83, 86,127,116,117,183,184,185,186,187,188,189,190,
    191,192,193,194,134,138,130,132,128,129,131,137,133,135,136,113,
    115,114,  0,  0,  0,121,  0, 89, 93,124, 92, 94, 95,  0,  0,  0,
    122,123, 90, 91, 85,  0,  0,  0,  0,  0,  0,  0,111,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     29, 42, 56,125, 97, 54,100,126,164,166,165,163,161,115,114,113,
    150,158,159,128,136,177,178,176,142,152,173,140,
};

/* Linux key code -> HID usage (0 = none), and -> modifier bit. */
static uint8_t s_evdev_to_hid[256];
static uint8_t s_evdev_to_mod[256];
static bool s_tables_built;

static kb_event_callback_t s_key_cb;
static ble_connect_cb_t s_connect_cb;
static bool s_connected;
static bool s_enabled = true;
static uint8_t s_mods;
static bool s_down[256];

static void build_tables(void)
{
    if (s_tables_built) return;
    /* Ordinary keys 0x04..0xA4. The first usage wins where two map to
     * one key code: HID 0x31 (backslash) before 0x32 (the ISO key left
     * of Enter on some keyboards, which kb_layout handles on 0x31 too). */
    for (int u = 0x04; u < 0xE0; u++) {
        int k = s_hid_to_evdev[u];
        if (k && !s_evdev_to_hid[k]) s_evdev_to_hid[k] = (uint8_t)u;
    }
    /* 0xE0..0xE7: LCtrl, LShift, LAlt, LGUI, RCtrl, RShift, RAlt, RGUI. */
    for (int i = 0; i < 8; i++) s_evdev_to_mod[s_hid_to_evdev[0xE0 + i]] = (uint8_t)(1u << i);
    s_tables_built = true;
}

static void deliver(uint8_t usage, bool pressed)
{
    if (!s_connected || !s_enabled || !s_key_cb) return;
    kb_event_t ev;
    memset(&ev, 0, sizeof(ev));
    ev.keycode = usage;
    ev.modifier = s_mods;
    ev.character = 0;   /* the editor maps keycodes through kb_layout */
    ev.pressed = pressed;
    s_key_cb(&ev);
}

void linux_io_key(int code, bool pressed)
{
    build_tables();
    if (code < 0 || code > 255) return;
    uint8_t mod = s_evdev_to_mod[code];
    if (mod) {
        if (pressed) s_mods |= mod;
        else s_mods &= (uint8_t)~mod;
        s_down[code] = pressed;
        return;
    }
    uint8_t usage = s_evdev_to_hid[code];
    if (!usage) return;
    if (pressed && s_down[code]) return;   /* the host's auto-repeat */
    if (!pressed && !s_down[code]) return;
    s_down[code] = pressed;
    deliver(usage, pressed);
}

void linux_io_release_all_keys(void)
{
    build_tables();
    s_mods = 0;
    for (int k = 0; k < 256; k++) {
        if (!s_down[k]) continue;
        s_down[k] = false;
        if (s_evdev_to_hid[k]) deliver(s_evdev_to_hid[k], false);
    }
}

/* The mouse wheel moves the cursor three lines per notch, as Up /
 * Down would (the editor has no scroll position apart from the
 * cursor's). */
void linux_io_wheel(int steps)
{
    uint8_t usage = steps > 0 ? KB_KEY_DOWN : KB_KEY_UP;
    int n = (steps > 0 ? steps : -steps) * 3;
    uint8_t saved = s_mods;
    s_mods = 0;
    for (int i = 0; i < n && i < 30; i++) {
        deliver(usage, true);
        deliver(usage, false);
    }
    s_mods = saved;
}

/* ---- the BLE keyboard API --------------------------------------------- */

void ble_keyboard_init(void)
{
    build_tables();
    s_connected = true;
    if (s_connect_cb) s_connect_cb(true);
}

void ble_keyboard_disable(void) { s_enabled = false; }
void ble_keyboard_enable(void) { s_enabled = true; }
void ble_keyboard_shutdown(void) { s_enabled = false; }

/* The computer's keyboard cannot be forgotten. */
void ble_keyboard_forget_all(void) { }

void ble_keyboard_set_callback(kb_event_callback_t cb) { s_key_cb = cb; }
void ble_keyboard_set_passkey_callback(ble_passkey_cb_t cb) { (void)cb; }
void ble_keyboard_set_connect_callback(ble_connect_cb_t cb) { s_connect_cb = cb; }
void ble_keyboard_set_status_text_callback(ble_status_text_cb_t cb) { (void)cb; }
bool ble_keyboard_is_connected(void) { return s_connected; }
void ble_keyboard_start_scan(void) { s_enabled = true; }

const char *ble_keyboard_get_device_name(void)
{
    return s_connected ? HOST_KB_NAME : NULL;
}

bool ble_keyboard_get_device_addr(uint8_t *addr)
{
    (void)addr;
    return false;
}

int ble_keyboard_get_battery_level(void) { return -1; }
/* The Caps Lock LED belongs to the desktop. */
void ble_keyboard_set_leds(uint8_t leds) { (void)leds; }
int64_t ble_keyboard_last_pairing_activity_us(void) { return 0; }
