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

/* Key matrix: a column is driven LOW, a pressed key pulls its row
 * (input with pull-up) LOW. Every row is an RTC IO, which the deep-sleep
 * keyboard wake needs. Key number = row * 9 + col. */
#define MATRIX_KBD_ROWS     8
#define MATRIX_KBD_COLS     9
#define MATRIX_KBD_ROW_PINS { 8, 18, 17, 16, 15, 7, 6, 5 }
#define MATRIX_KBD_COL_PINS { 1, 2, 42, 41, 40, 39, 45, 48, 47 }

/* HID usage of every key, by key number. The keys are numbered in
 * reading order of the keyboard (Esc first, Right arrow 68); 69 is the
 * optional EC11 encoder's push switch, 70 and 71 are not wired.
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
 * push is F1 (the menu). MATRIX_KBD_FN (matrix_kbd.h) marks the Fn
 * key. */
#define MATRIX_KBD_KEYMAP {                                                  \
    0x29, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x2D,  \
    0x2E, 0x2A, 0x4C,                                                        \
    0x2B, 0x14, 0x1A, 0x08, 0x15, 0x17, 0x1C, 0x18, 0x0C, 0x12, 0x13, 0x2F,  \
    0x30, 0x28, 0x4B,                                                        \
    0x39, 0x04, 0x16, 0x07, 0x09, 0x0A, 0x0B, 0x0D, 0x0E, 0x0F, 0x33, 0x34,  \
    0x31, 0x4E,                                                              \
    0xE1, 0x35, 0x1D, 0x1B, 0x06, 0x19, 0x05, 0x11, 0x10, 0x36, 0x37, 0x38,  \
    0xE5, 0x52, 0x4D,                                                        \
    0xE0, 0xE3, 0xE2, 0x2C, 0xE6, MATRIX_KBD_FN,     0x4A, 0x50, 0x51, 0x4F, \
    0x3A, 0x00, 0x00 }

/* While Fn is held: the number row gives F1..F12 (the keyboard has no
 * function row; F1 is the menu, F2 rename, F10 help), Up / Down give
 * Page Up / Page Down and Left / Right give Home / End. 0 = the normal
 * key. */
#define MATRIX_KBD_FN_KEYMAP {                                               \
    0x00, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44,  \
    0x45, 0x00, 0x00,                                                        \
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  \
    0x00, 0x00, 0x00,                                                        \
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  \
    0x00, 0x00,                                                              \
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  \
    0x00, 0x4B, 0x00,                                                        \
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4A, 0x4E, 0x4D,              \
    0x00, 0x00, 0x00 }
