#pragma once
/* ----- Freenove FNK0104N -----
 *
 * 3.5" color LCD, ST77922 controller over QSPI, 320x480 native panel
 * (rendered landscape at 480x320 by the display backend). Bundled
 * capacitive touch controller on a dedicated I2C bus (exact silicon
 * unconfirmed; Freenove's own reference firmware calls the class
 * "ST77922_Touch"). Pin assignments, protocol and register map below
 * come directly from Freenove's own vendor driver source
 * (Libraries/FNK0104N/TFT_eSPI_v2.5.43.zip -> TFT_eSPI/ST77922.* and
 * ST77922_Touch.*, bundled with the board's GitHub repo), not a
 * third-party reverse-engineering project.
 * See https://github.com/Freenove/Freenove_ESP32_S3_Display
 *
 * Included by main/app_config.h when
 * CONFIG_DRAFTLING_MODEL_FREENOVE_FNK0104N is selected.
 *
 * The QSPI pins, backlight pin/polarity and vendor init sequence are
 * hard-coded inside components/display/display_st77922.cpp (this SKU
 * is the only board using that backend), so this header carries only
 * the pins that vary per SKU: SD card, touch, battery ADC and the
 * wakeup button.
 */

#define BOARD_NAME          "Freenove FNK0104N"

/* SD card on the on-chip SDMMC peripheral (1-bit mode: CLK/CMD/D0
 * only -- D1-D3 are left unconfigured, matching the existing
 * Waveshare RLCD-4.2 usage of sd_card_init()). Different pins from
 * the FNK0104A/B/S SPI-TFT boards because the QSPI panel bus already
 * occupies GPIO9/11-14 on this SKU. */
#define SD_CLK_PIN          5
#define SD_CMD_PIN          4
#define SD_D0_PIN           6

/* Dedicated I2C bus for the panel's bundled touch controller. */
#define I2C_SDA_PIN         38
#define I2C_SCL_PIN         39

/* Touch controller bundled with the ST77922 module, I2C address
 * 0x55. Native touch-panel resolution matches the LCD's native
 * portrait orientation (320x480); the touchscreen component rotates
 * it into the backend's landscape 480x320 logical frame. */
#define TOUCH_I2C_ADDR      0x55
#define TOUCH_INT_PIN       47
#define TOUCH_RST_PIN       48
#define TOUCH_NATIVE_W      320
#define TOUCH_NATIVE_H      480
#define TOUCH_SWAP_XY       1
#define TOUCH_MIRROR_X      1
#define TOUCH_MIRROR_Y      0

/* Battery voltage through a 1:2 divider (matches the M5Stack PaperS3
 * ADC backend usage: analogReadMilliVolts() * 2.0 in Freenove's
 * reference Sketch_05.1_Battery_Voltage; this SKU uses GPIO8 instead
 * of GPIO9 because GPIO9 is part of the QSPI panel bus). */
#define BATT_ADC_PIN        8
#define BATT_EN_PIN         -1
#define BATT_DIVIDER        2

/* BOOT button (KEY_PIN in Freenove's reference sketches) doubles as
 * the deep-sleep wakeup source. */
#define WAKEUP_GPIO_NUM     0
