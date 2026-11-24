#pragma once
/* ----- LilyGO T5 4.7" E-Paper, ESP32-S3 version -----
 *
 * ESP32-S3 (16 MB flash, 8 MB octal PSRAM) with a 4.7" 960x540
 * ED047TC1 e-paper panel. The panel's data bus and clocks are direct
 * GPIOs; output enable, mode, STV, the panel supplies and the row
 * latch sit behind a 74HC4094 shift register. The display backend
 * (components/display/display_h752.cpp, FastEPD panel
 * BB_PANEL_LILYGO_T5_47_S3 in components/fastepd) owns all of these:
 *
 *   D0-D7  8, 1, 2, 3, 4, 5, 6, 7
 *   CKH    41 (i80 WR)          CKV 38          STH 40 (i80 CS)
 *   shift register: DATA 13, CLK 12, STR 0 (the BOOT button net)
 *   GPIO39 (expansion header) is the i80 driver's dummy D/C line.
 *
 * The USB-C port is the ESP32-S3's native USB (GPIO19 / GPIO20). With
 * CONFIG_DRAFTLING_USB_HOST_NATIVE it is a USB host for a keyboard,
 * which needs 5 V fed onto VBUS from outside (the board does not
 * source it) -- the MicroJournal Rev.7 does this from its battery
 * boost module, which powers both the board and the keyboard.
 *
 * Not used yet: the touch panel of the touch version (I2C SDA=18,
 * SCL=17, INT=47), the PCF8563 RTC on the same bus, and the battery
 * ADC on GPIO14 (its divider is reported to be powered only while the
 * panel supply is on).
 *
 * Sources: LilyGO's LilyGo-EPD47 driver (ESP32-S3 pin map in
 * ed047tc1.h / utilities.h), as vendored in the MicroJournal firmware
 * (github.com/unkyulee/micro-journal-mcu, env rev_7); the shift-register
 * bit order also matches epdiy's epd_board_lilygo_t5_47.c.
 */

#define BOARD_NAME          "LilyGO T5 4.7\" E-Paper S3"

/* MicroSD slot on its own SPI bus. */
#define SD_SPI_MISO_PIN     16
#define SD_SPI_MOSI_PIN     15
#define SD_SPI_SCK_PIN      11
#define SD_SPI_CS_PIN       42
#define SD_EN_PIN           -1

/* No touch or battery monitor (see above). */
#define I2C_SDA_PIN         -1
#define I2C_SCL_PIN         -1
#define TOUCH_I2C_ADDR      -1
#define TOUCH_INT_PIN       -1
#define TOUCH_RST_PIN       -1
#define TOUCH_NATIVE_W      960
#define TOUCH_NATIVE_H      540
#define TOUCH_SWAP_XY       0
#define TOUCH_MIRROR_X      0
#define TOUCH_MIRROR_Y      0
#define BATT_ADC_PIN        -1
#define BATT_EN_PIN         -1
#define BATT_DIVIDER        1

/* User button (IO21, active low, RTC-capable): deep-sleep wake source,
 * 2 s hold forgets the BLE keyboards. GPIO0 (BOOT) is the shift
 * register's strobe line and must not be used as a button. */
#define WAKEUP_GPIO_NUM     21
