#pragma once

/*
 * Keyboard matrix wired straight to GPIO pins (MicroJournal Rev.6).
 *
 * The columns are driven low one at a time (open-drain outputs) and
 * the rows read with pull-ups; each key has a diode from its row to its
 * column (QMK "ROW2COL"). A key's index is row * ncols + col.
 *
 * Each key's meaning comes from a keymap of MKBD_* actions, one table
 * per layer: layer 0 is the base layer, a MKBD_LAYER(n) key selects
 * layer n while held, and MKBD_TRANS in a layer falls through to layer
 * 0. An optional "shift" table is consulted while a Shift modifier key
 * is held (before the layers); MKBD_TRANS there falls through as well.
 *
 * Keys are reported as the same kb_event_t press / release stream the
 * BLE and USB keyboards produce (HID usage + modifier byte, character
 * 0), so the editor's handler and the keyboard layouts apply as for
 * any other keyboard. Modifier and layer keys produce no events of
 * their own; their state is in the modifier byte of the other keys.
 *
 * MKBD_OSM(mods) is a one-shot modifier: tapped, it adds mods to the
 * next key that produces an event (with whatever layer is active
 * then), so a combination does not need three keys held at once;
 * held, it applies to every key pressed meanwhile. Tapping it again
 * before the next key cancels it.
 */

#include <stdbool.h>
#include <stdint.h>

#include "ble_keyboard.h"   /* kb_event_t, kb_event_callback_t, KB_MOD_* */

#ifdef __cplusplus
extern "C" {
#endif

/* Actions (uint16_t): high byte = kind, low byte = argument. */
#define MKBD_TRANS          0x0000              /* see the layer rules above */
#define MKBD_KC(hid)        ((uint16_t)(0x0100 | ((hid) & 0xFF)))  /* HID usage */
#define MKBD_KC_NOSHIFT(h)  ((uint16_t)(0x0200 | ((h) & 0xFF)))  /* usage, Shift dropped */
#define MKBD_MOD(mods)      ((uint16_t)(0x0300 | ((mods) & 0xFF)))  /* held modifier */
#define MKBD_OSM(mods)      ((uint16_t)(0x0400 | ((mods) & 0xFF)))  /* one-shot modifier */
#define MKBD_LAYER(n)       ((uint16_t)(0x0500 | ((n) & 0xFF)))  /* momentary layer */
#define MKBD_NONE           ((uint16_t)0x0600)  /* key does nothing */

#define MATRIX_KBD_MAX_ROWS 8
#define MATRIX_KBD_MAX_COLS 16

typedef struct {
    const int8_t   *row_pins;      /* nrows GPIOs, read with pull-ups */
    const int8_t   *col_pins;      /* ncols GPIOs, driven low in turn */
    uint8_t         nrows;
    uint8_t         ncols;
    const uint16_t *layers;        /* nlayers tables of nrows * ncols actions */
    uint8_t         nlayers;
    const uint16_t *shift_layer;   /* nrows * ncols actions, or NULL */
} matrix_kbd_config_t;

/* Configure the pins and start the scan task. The config and its tables
 * must stay valid (static). Returns 0 on success. */
int  matrix_kbd_init(const matrix_kbd_config_t *cfg);

/* True once matrix_kbd_init() has succeeded. */
bool matrix_kbd_is_present(void);

/* Receives every key event (from the scan task). */
void matrix_kbd_set_callback(kb_event_callback_t cb);

/* Before deep sleep: stop scanning, wait (up to 3 s) until no key is
 * held, drive every column low and hold it through sleep, and enable
 * the RTC pull-ups of the rows. Returns the bit mask of the row GPIOs
 * for an EXT1 any-low wake (a key press pulls its row low), or 0 when
 * a key stayed held or the rows cannot wake the chip. */
uint64_t matrix_kbd_prepare_sleep(void);

#ifdef __cplusplus
}
#endif
