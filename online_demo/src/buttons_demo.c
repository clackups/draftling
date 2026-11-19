/*
 * Online demo: the board's own buttons, shown next to the simulated
 * screen (web/demo.js) and pressed through demo_button(). They act as
 * main.cpp's button pollers do: a release before 2 s is a short press,
 * holding for 2 s is a long press.
 *
 *   Freenove FNK0104S        BOOT: hold = forget the paired keyboards
 *   Seeed reTerminal Sticky  Power: press = sleep (asking about unsaved
 *                            changes first), hold = forget keyboards;
 *                            Up / Down: Page Up / Page Down
 */

#include <stdbool.h>
#include <string.h>

#include <emscripten.h>

#include "ble_keyboard.h"
#include "editor_ui.h"
#include "lvgl.h"
#include "sdkconfig.h"

#define LONG_PRESS_MS 2000

enum { BTN_POWER = 0, BTN_UP = 1, BTN_DOWN = 2, BTN_COUNT };

static uint32_t s_down_at[BTN_COUNT];
static bool s_down[BTN_COUNT], s_long_done[BTN_COUNT];
static lv_timer_t *s_timer;

static void inject_key(uint8_t keycode)
{
    kb_event_t ev;
    memset(&ev, 0, sizeof(ev));
    ev.keycode = keycode;
    ev.pressed = true;
    editor_ui_handle_key(&ev);
    ev.pressed = false;
    editor_ui_handle_key(&ev);
}

static void long_press(int id)
{
    if (id == BTN_POWER) ble_keyboard_forget_all();
}

static void short_press(int id)
{
#if defined(CONFIG_DRAFTLING_MODEL_SEEED_RETERMINAL_STICKY)
    if (id == BTN_POWER) editor_ui_request_sleep();
    else if (id == BTN_UP) inject_key(KB_KEY_PAGEUP);
    else if (id == BTN_DOWN) inject_key(KB_KEY_PAGEDOWN);
#else
    (void)id;
#endif
}

static void poll_cb(lv_timer_t *t)
{
    (void)t;
    for (int i = 0; i < BTN_COUNT; i++) {
        if (s_down[i] && !s_long_done[i] &&
            lv_tick_elaps(s_down_at[i]) >= LONG_PRESS_MS) {
            s_long_done[i] = true;
            long_press(i);
        }
    }
}

EMSCRIPTEN_KEEPALIVE void demo_button(int id, int pressed)
{
    if (id < 0 || id >= BTN_COUNT) return;
    if (!s_timer) s_timer = lv_timer_create(poll_cb, 50, NULL);
    if (pressed) {
        if (s_down[id]) return;
        s_down[id] = true;
        s_long_done[id] = false;
        s_down_at[id] = lv_tick_get();
    } else if (s_down[id]) {
        s_down[id] = false;
        if (!s_long_done[id]) short_press(id);
    }
}
