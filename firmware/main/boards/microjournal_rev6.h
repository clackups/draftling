#pragma once
/* ----- MicroJournal Rev.6 -----
 *
 * ESP32-S3 N16R8 (or N8R2) DevKitC writerdeck with a 48-key mechanical
 * keyboard wired straight to the S3's GPIO pins, a 2.8" 240x320 SPI LCD
 * module (shown as 320x240 landscape) and the MicroSD slot of that
 * module on the same SPI bus. Battery (18650, charger / boost module)
 * behind a slide switch; no battery monitor, backlight control or wake
 * button.
 *
 * Display and SD card are wired as on the Rev 5 (same pins, same
 * ILI9341 / ST7789 panels, detected at boot). The keyboard is a 4 x 12
 * matrix with a diode from each key's row to its column: the columns
 * are driven low in turn and the rows read with pull-ups
 * (components/matrix_kbd). Two columns are GPIO19 / GPIO20, the native
 * USB pins, so the DevKitC's native USB port cannot be used: power,
 * flashing and the console go through its UART (USB-to-serial) port.
 *
 * Sources: https://github.com/unkyulee/micro-journal rev.6/6.0/build.md
 * (keyboard pin table, display wiring: SD CS on GPIO9) and the upstream
 * firmware's rev_6_ili9341 / rev_6_st7789 environments and
 * src/keyboard/Keypad/48/keypad_48.cpp
 * (https://github.com/unkyulee/micro-journal-mcu).
 */

#define BOARD_NAME          "MicroJournal Rev.6"

/* SPI panel (display_ili9341.cpp, display_spi_tft_config()). */
#define LCD_MOSI_PIN        11
#define LCD_MISO_PIN        13
#define LCD_SCK_PIN         12
#define LCD_CS_PIN          10
#define LCD_DC_PIN          46
#define LCD_RST_PIN         3
#define LCD_BL_PIN          -1
#define LCD_IM0_PIN         -1
#define LCD_IM1_PIN         -1
#define LCD_SPI_CLOCK_HZ    (40 * 1000 * 1000)
#define LCD_BL_PWM_HZ       50000

/* The display module's MicroSD slot, on the display's SPI2 bus. */
#define SD_SPI_MISO_PIN     13
#define SD_SPI_MOSI_PIN     11
#define SD_SPI_SCK_PIN      12
#define SD_SPI_CS_PIN       9
#define SD_EN_PIN           -1
#define SD_SPI_HOST_NUM     SPI2_HOST

/* Held high while the display controller is probed. */
#define LCD_SHARED_BUS_CS_PIN  SD_SPI_CS_PIN

/* No touch or battery hardware. */
#define I2C_SDA_PIN         -1
#define I2C_SCL_PIN         -1
#define TOUCH_I2C_ADDR      -1
#define TOUCH_INT_PIN       -1
#define TOUCH_RST_PIN       -1
#define TOUCH_NATIVE_W      320
#define TOUCH_NATIVE_H      240
#define TOUCH_SWAP_XY       0
#define TOUCH_MIRROR_X      0
#define TOUCH_MIRROR_Y      0
#define BATT_ADC_PIN        -1
#define BATT_EN_PIN         -1
#define BATT_DIVIDER        1

/* No wake button: any key wakes the device (see the pre-sleep hook in
 * main.cpp, matrix_kbd_prepare_sleep()). */
#define WAKEUP_GPIO_NUM     -1

/* ---- Keyboard matrix ---- */

#include "matrix_kbd.h"

#define KBD_ROWS 4
#define KBD_COLS 12

/* Keyboard PCB pins 1-4 (rows) and 5-16 (columns), as in the Rev.6
 * build guide. GPIO15-18 are RTC IOs, so the rows can wake the chip. */
static const int8_t KBD_ROW_PINS[KBD_ROWS] = { 15, 16, 17, 18 };
static const int8_t KBD_COL_PINS[KBD_COLS] = {
    1, 2, 42, 41, 40, 39, 45, 48, 47, 21, 20, 19 };

/* HID usages */
#define K(x)   MKBD_KC(x)
#define TR     MKBD_TRANS
#define K_A 0x04
#define K_B 0x05
#define K_C 0x06
#define K_D 0x07
#define K_E 0x08
#define K_F 0x09
#define K_G 0x0A
#define K_H 0x0B
#define K_I 0x0C
#define K_J 0x0D
#define K_K 0x0E
#define K_L 0x0F
#define K_M 0x10
#define K_N 0x11
#define K_O 0x12
#define K_P 0x13
#define K_Q 0x14
#define K_R 0x15
#define K_S 0x16
#define K_T 0x17
#define K_U 0x18
#define K_V 0x19
#define K_W 0x1A
#define K_X 0x1B
#define K_Y 0x1C
#define K_Z 0x1D
#define K_1 0x1E
#define K_2 0x1F
#define K_3 0x20
#define K_4 0x21
#define K_5 0x22
#define K_6 0x23
#define K_7 0x24
#define K_8 0x25
#define K_9 0x26
#define K_0 0x27
#define K_ENT  0x28
#define K_ESC  0x29
#define K_BSPC 0x2A
#define K_TAB  0x2B
#define K_SPC  0x2C
#define K_MINS 0x2D
#define K_EQL  0x2E
#define K_LBRC 0x2F
#define K_RBRC 0x30
#define K_BSLS 0x31
#define K_SCLN 0x33
#define K_QUOT 0x34
#define K_GRV  0x35
#define K_COMM 0x36
#define K_DOT  0x37
#define K_SLSH 0x38
#define K_F1   0x3A
#define K_HOME 0x4A
#define K_PGUP 0x4B
#define K_DEL  0x4C
#define K_END  0x4D
#define K_PGDN 0x4E
#define K_RGHT 0x4F
#define K_LEFT 0x50
#define K_DOWN 0x51
#define K_UP   0x52

/* The Rev.6 key positions (upstream keypad_48.cpp) with what Draftling
 * needs on Lower: the top-left key (the Rev.6 menu key) is Esc and
 * Lower + it F1; Lower + - / = are one-shot Ctrl / Alt, Lower + [ is
 * Tab. Documented in HARDWARE.md ("MicroJournal Rev.6"). */
static const uint16_t KBD_LAYERS[2][KBD_ROWS * KBD_COLS] = {
    { /* base */
        K(K_ESC),  K(K_Q), K(K_W), K(K_E), K(K_R), K(K_T), K(K_Y), K(K_U), K(K_I),    K(K_O),   K(K_P),    K(K_BSPC),
        K(K_BSLS), K(K_A), K(K_S), K(K_D), K(K_F), K(K_G), K(K_H), K(K_J), K(K_K),    K(K_L),   K(K_SCLN), K(K_QUOT),
        MKBD_MOD(KB_MOD_LSHIFT), K(K_Z), K(K_X), K(K_C), K(K_V), K(K_B), K(K_N), K(K_M),
                   K(K_COMM), K(K_DOT), K(K_SLSH), MKBD_MOD(KB_MOD_RSHIFT),
        K(K_MINS), K(K_EQL), K(K_LBRC), K(K_RBRC), MKBD_LAYER(1), K(K_SPC), K(K_SPC),
                   K(K_LEFT), K(K_DOWN), K(K_UP), K(K_RGHT), K(K_ENT),
    },
    { /* Lower */
        K(K_F1),   K(K_1), K(K_2), K(K_3), K(K_4), K(K_5), K(K_6), K(K_7), K(K_8),    K(K_9),   K(K_0),    K(K_DEL),
        K(K_GRV),  TR,     TR,     TR,     TR,     TR,     TR,     TR,     TR,        TR,       TR,        TR,
        TR,        TR,     TR,     TR,     TR,     TR,     TR,     TR,     TR,        TR,       TR,        TR,
        MKBD_OSM(KB_MOD_LCTRL), MKBD_OSM(KB_MOD_LALT), K(K_TAB), TR, TR, TR, TR,
                   K(K_HOME), K(K_PGDN), K(K_PGUP), K(K_END), TR,
    },
};

/* While Shift is held: Backspace is Delete (as on the Rev.6). */
static const uint16_t KBD_SHIFT_LAYER[KBD_ROWS * KBD_COLS] = {
    TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, MKBD_KC_NOSHIFT(K_DEL),
    TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR,
    TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR,
    TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR,
};

#undef K
#undef TR
