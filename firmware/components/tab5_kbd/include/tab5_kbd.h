#pragma once

/*
 * M5Stack Tab5 attachable keyboard host.
 *
 * The Tab5 keyboard is a detachable QWERTY keyboard that talks to the
 * Tab5 (ESP32-P4) over a dedicated I2C bus (7-bit address 0x6D) plus
 * a dedicated interrupt line on GPIO 50. Its co-processor exposes a
 * small register map (version, mode, interrupt status, event queue,
 * RGB indicator LEDs). We drive it in "interrupt HID" mode: the
 * keyboard latches each state change into an event queue and pulls
 * the INT line low; we read the 2-byte HID report (modifier +
 * keycode) for every queued event and turn it into the same
 * kb_event_t press/release stream the ble_keyboard / usb_kbd
 * components emit, so the editor's key handler is shared verbatim.
 *
 * Presence is probed once, at init time, by reading the version
 * register. If the keyboard is not attached the probe fails and the
 * component goes permanently idle: no further I2C traffic is issued
 * (so an absent keyboard never slows its dedicated bus) until the
 * next boot / wake re-runs tab5_kbd_init().
 *
 * Protocol reference: m5stack/M5Tab5-Keyboard-UserDemo
 * (components/m5_tab5_keyboard_component).
 */

#include <stdbool.h>

#include "driver/i2c_master.h"

#include "ble_keyboard.h"   /* kb_event_t, kb_event_callback_t */

#ifdef __cplusplus
extern "C" {
#endif

/* Default 7-bit I2C address of the Tab5 keyboard co-processor. */
#define TAB5_KBD_I2C_ADDR   0x6D

/*
 * Probe for an attached Tab5 keyboard on the given (already created)
 * I2C master bus and, if present, initialise it in interrupt HID
 * mode wired to the interrupt GPIO `int_gpio`.
 *
 * On success the indicator LEDs flash green for one second and the
 * internal worker task starts dispatching key events through the
 * callback registered with tab5_kbd_set_callback().
 *
 * Returns 0 if a keyboard was detected and initialised, non-zero
 * otherwise (no keyboard attached or an I2C / setup error). When it
 * returns non-zero the component stays idle and issues no further
 * I2C traffic this boot.
 */
int  tab5_kbd_init(i2c_master_bus_handle_t bus, int int_gpio);

/*
 * True iff a Tab5 keyboard was detected and initialised by the most
 * recent tab5_kbd_init() call.
 */
bool tab5_kbd_is_present(void);

/*
 * Register a callback that receives the same kb_event_t stream
 * format as ble_keyboard_set_callback(). May be called before or
 * after tab5_kbd_init(); subsequent calls replace the previous
 * callback.
 */
void tab5_kbd_set_callback(kb_event_callback_t cb);

#ifdef __cplusplus
}
#endif
