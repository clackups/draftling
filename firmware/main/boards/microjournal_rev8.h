#pragma once
/* ----- MicroJournal Rev.8 "Melodica" -----
 *
 * ESP32-S3-DevKitC-1 N16R8 writerdeck with a 4.2" 400x300 Osptek
 * YDP420H001-V3 reflective LCD (ST7306, driven by display_rlcd.cpp with
 * CONFIG_DRAFTLING_RLCD_PANEL_YDP420) and a built-in 69-key ISO
 * keyboard: an 8 x 9 diode matrix wired straight to the GPIOs
 * (components/matrix_kbd). No MicroSD slot (documents in internal flash,
 * CONFIG_DRAFTLING_STORAGE_FLASH), no battery monitor, no wake button
 * (a rocker switch cuts the battery; any key wakes it from standby).
 *
 * Sources: https://github.com/unkyulee/micro-journal rev.8/8.0/build.md
 * (display and keyboard wiring tables) and the upstream firmware
 * (https://github.com/unkyulee/micro-journal-mcu): the rev_8
 * environment in platformio.ini (display pins),
 * src/keyboard/Keypad/68/Keypad_68.cpp (matrix pins, key order) and
 * src/display/RLCD/KeyboardScreen/KeyboardScreen.cpp (the HID map of
 * the bottom row).
 */

#define BOARD_NAME          "MicroJournal Rev.8"

/* Reflective LCD, SPI (write only). */
#define RLCD_MOSI_PIN       11
#define RLCD_SCK_PIN        12
#define RLCD_CS_PIN         10
#define RLCD_DC_PIN         46
#define RLCD_RST_PIN        3

/* No wake button (CONFIG_DRAFTLING_WAKEUP_GPIO is -1). */
#define WAKEUP_GPIO_NUM     -1

/* No battery monitor. */
#define BATT_ADC_PIN        -1
#define BATT_EN_PIN         -1
#define BATT_DIVIDER        1

/* ---- Keyboard matrix (components/matrix_kbd) ---- */

#include "matrix_kbd.h"

/* A column is driven low, a pressed key pulls its row (input with
 * pull-up) low. Every row is an RTC IO, which the deep-sleep keyboard
 * wake needs. Key number = row * 9 + col. */
#define KBD_ROWS 8
#define KBD_COLS 9
static const int8_t KBD_ROW_PINS[KBD_ROWS] = { 8, 18, 17, 16, 15, 7, 6, 5 };
static const int8_t KBD_COL_PINS[KBD_COLS] = { 1, 2, 42, 41, 40, 39, 45, 48, 47 };

/* The keys are numbered in reading order of the keyboard (Esc first,
 * Right arrow 68); 69 is the optional EC11 encoder's push switch, 70
 * and 71 are not wired.
 *
 *   Esc 1 2 3 4 5 6 7 8 9 0 - = Bksp Del            0..14
 *   Tab Q W E R T Y U I O P [ ] Enter PgUp           15..29
 *   Caps A S D F G H J K L ; ' \ PgDn                30..43
 *   Shift ` Z X C V B N M , . / Shift Up End          44..58
 *   Ctrl Win Alt Space AltGr Fn Home Left Down Right  59..68
 *
 * The key between the left Shift and Z (the ISO key, keycap ` ~) sends
 * the US grave key, 0x35, like the upstream firmware; the key left of
 * Enter (\ |) sends 0x31, which the ISO layouts also map. The encoder
 * push is F1 (the menu). Layer 1 (Fn held): the number row gives
 * F1..F12 (there is no function row; F1 is the menu, F2 rename, F10
 * help), Up / Down Page Up / Page Down and Left / Right Home / End. */
#define K(x)  MKBD_KC(x)
#define TR    MKBD_TRANS
#define NO    MKBD_NONE
static const uint16_t KBD_LAYERS[2][KBD_ROWS * KBD_COLS] = {
    { /* base */
        K(0x29), K(0x1E), K(0x1F), K(0x20), K(0x21), K(0x22), K(0x23), K(0x24), K(0x25),
        K(0x26), K(0x27), K(0x2D), K(0x2E), K(0x2A), K(0x4C),
        K(0x2B), K(0x14), K(0x1A), K(0x08), K(0x15), K(0x17), K(0x1C), K(0x18), K(0x0C),
        K(0x12), K(0x13), K(0x2F), K(0x30), K(0x28), K(0x4B),
        K(0x39), K(0x04), K(0x16), K(0x07), K(0x09), K(0x0A), K(0x0B), K(0x0D), K(0x0E),
        K(0x0F), K(0x33), K(0x34), K(0x31), K(0x4E),
        MKBD_MOD(KB_MOD_LSHIFT), K(0x35), K(0x1D), K(0x1B), K(0x06), K(0x19), K(0x05),
        K(0x11), K(0x10), K(0x36), K(0x37), K(0x38), MKBD_MOD(KB_MOD_RSHIFT), K(0x52), K(0x4D),
        MKBD_MOD(KB_MOD_LCTRL), MKBD_MOD(KB_MOD_LGUI), MKBD_MOD(KB_MOD_LALT), K(0x2C),
        MKBD_MOD(KB_MOD_RALT), MKBD_LAYER(1), K(0x4A), K(0x50), K(0x51), K(0x4F),
        K(0x3A), NO, NO,
    },
    { /* Fn */
        TR,      K(0x3A), K(0x3B), K(0x3C), K(0x3D), K(0x3E), K(0x3F), K(0x40), K(0x41),
        K(0x42), K(0x43), K(0x44), K(0x45), TR,      TR,
        TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR,
        TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR,
        TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, K(0x4B), TR,
        TR, TR, TR, TR, TR, TR, TR, K(0x4A), K(0x4E), K(0x4D),
        TR, TR, TR,
    },
};
/* No Shift layer: Shift is just Shift. */
#define KBD_SHIFT_LAYER NULL
#undef K
#undef TR
#undef NO
