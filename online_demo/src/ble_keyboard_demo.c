/*
 * Online demo: the Bluetooth keyboard is the computer's keyboard.
 * web/demo.js turns browser key events into HID usage codes and
 * modifier bits and calls demo_key(), which delivers them through the
 * callback the editor registered, exactly as the BLE HID host does.
 * "Connecting" takes a moment after boot, and after "forget keyboards",
 * so the BLE prompt screen shows as it does on the device.
 */

#include <string.h>

#include <emscripten.h>

#include "ble_keyboard.h"
#include "esp_timer.h"
#include "lvgl.h"

#define DEMO_KB_NAME "Browser keyboard"

static kb_event_callback_t s_key_cb;
static ble_connect_cb_t s_connect_cb;
static ble_status_text_cb_t s_status_cb;
static bool s_connected;
static bool s_enabled = true;
static int64_t s_last_pairing_us;
static lv_timer_t *s_connect_timer;

static void connect_timer_cb(lv_timer_t *t)
{
    lv_timer_delete(t);
    s_connect_timer = NULL;
    if (!s_enabled || s_connected) return;
    s_connected = true;
    s_last_pairing_us = esp_timer_get_time();
    if (s_connect_cb) s_connect_cb(true);
}

static void connect_later(uint32_t ms)
{
    if (s_connect_timer) lv_timer_delete(s_connect_timer);
    if (s_status_cb) s_status_cb("Connecting to " DEMO_KB_NAME "...");
    s_connect_timer = lv_timer_create(connect_timer_cb, ms, NULL);
}

EMSCRIPTEN_KEEPALIVE void demo_key(int keycode, int modifier, int pressed)
{
    if (!s_connected || !s_enabled || !s_key_cb) return;
    kb_event_t ev;
    memset(&ev, 0, sizeof(ev));
    ev.keycode = (uint8_t)keycode;
    ev.modifier = (uint8_t)modifier;
    ev.character = 0;   /* the editor maps keycodes through kb_layout */
    ev.pressed = pressed != 0;
    s_key_cb(&ev);
}

void ble_keyboard_init(void)
{
    connect_later(900);
}

void ble_keyboard_disable(void) { s_enabled = false; }

void ble_keyboard_enable(void)
{
    s_enabled = true;
    if (!s_connected) connect_later(900);
}

void ble_keyboard_shutdown(void) { s_enabled = false; }

void ble_keyboard_forget_all(void)
{
    if (s_connected) {
        s_connected = false;
        if (s_connect_cb) s_connect_cb(false);
    }
    connect_later(1500);
}

void ble_keyboard_set_callback(kb_event_callback_t cb) { s_key_cb = cb; }
void ble_keyboard_set_passkey_callback(ble_passkey_cb_t cb) { (void)cb; }
void ble_keyboard_set_connect_callback(ble_connect_cb_t cb) { s_connect_cb = cb; }
void ble_keyboard_set_status_text_callback(ble_status_text_cb_t cb) { s_status_cb = cb; }
bool ble_keyboard_is_connected(void) { return s_connected; }

void ble_keyboard_start_scan(void)
{
    s_enabled = true;
    if (!s_connected) connect_later(900);
}

const char *ble_keyboard_get_device_name(void)
{
    return s_connected ? DEMO_KB_NAME : NULL;
}

bool ble_keyboard_get_device_addr(uint8_t *addr)
{
    static const uint8_t a[6] = { 0xd0, 0x2a, 0x7e, 0x5d, 0x00, 0x01 };
    if (!s_connected) return false;
    memcpy(addr, a, 6);
    return true;
}

int ble_keyboard_get_battery_level(void) { return -1; }
void ble_keyboard_set_leds(uint8_t leds) { (void)leds; }
int64_t ble_keyboard_last_pairing_activity_us(void) { return s_last_pairing_us; }
