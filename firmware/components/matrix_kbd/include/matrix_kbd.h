#pragma once

/*
 * Built-in key-matrix keyboard: rows and columns wired straight to GPIOs,
 * one diode per key (no ghosting), as on the MicroJournal Rev.8.
 *
 * A task scans the matrix every few milliseconds: each column in turn is
 * driven LOW while the others are HIGH, and a pressed key pulls its row
 * (an input with pull-up) LOW through its diode. Key number =
 * row * ncols + col. The board header supplies the pins and two keymaps
 * of HID usage codes indexed by key number: the normal one and the one
 * used while the Fn key is held (0 there = same as normal). Modifier
 * keys are given as their HID usages 0xE0..0xE7 and the Fn key as
 * MATRIX_KBD_FN. Key presses and releases come out as the same
 * kb_event_t stream the BLE / USB keyboards emit (modifier mask +
 * keycode; the editor maps the keycode through the active layout).
 *
 * The component has no board knowledge: main.cpp fills a
 * matrix_kbd_config_t from the board header.
 */

#include <stdbool.h>
#include <stdint.h>

#include "ble_keyboard.h"   /* kb_event_t, kb_event_callback_t */

#ifdef __cplusplus
extern "C" {
#endif

/* Keymap value for the Fn (layer) key. HID usage 0x01 is ErrorRollOver,
 * which no key sends, so it is free for this. */
#define MATRIX_KBD_FN   0x01

#define MATRIX_KBD_MAX_ROWS 16
#define MATRIX_KBD_MAX_COLS 16

typedef struct {
    const int8_t  *rows;       /* row GPIOs: inputs with pull-up */
    int            nrows;
    const int8_t  *cols;       /* column GPIOs: driven LOW one at a time */
    int            ncols;
    const uint8_t *keymap;     /* nrows * ncols HID usages, 0 = no key */
    const uint8_t *fn_keymap;  /* same size, used while Fn is held; may be NULL */
} matrix_kbd_config_t;

/* Configure the GPIOs and start the scan task. The arrays must stay
 * valid for the life of the program. Also undoes the pin holds and RTC
 * IO set-up matrix_kbd_prepare_sleep() left behind. Returns 0 on
 * success. */
int  matrix_kbd_init(const matrix_kbd_config_t *cfg);

/* True once matrix_kbd_init() has succeeded. */
bool matrix_kbd_is_present(void);

/* Register the key event callback (same format as
 * ble_keyboard_set_callback()). */
void matrix_kbd_set_callback(kb_event_callback_t cb);

/* Before deep sleep: stop scanning, wait (up to 10 s) until no key is
 * held, then drive every column LOW and hold it, and arm an EXT1
 * any-low wake on the rows, so any key wakes the chip. The rows must
 * be RTC IOs. The caller enables gpio_deep_sleep_hold_en(). Returns
 * true if the wake was armed. */
bool matrix_kbd_prepare_sleep(void);

#ifdef __cplusplus
}
#endif
