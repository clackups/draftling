#pragma once
/* ----- Freenove FNK0104A -----
 *
 * 2.8" color LCD, ILI9341 controller, 240x320 native panel (rendered
 * landscape at 320x240 by the display backend). Standard 4-wire SPI
 * interface. No touch controller. Pin assignments below come from
 * Freenove's own TFT_eSPI setup header
 * (FNK0104AB_2.8_240x320_ILI9341.h) and the Sketch_03/05 reference
 * sketches bundled with the board.
 * See https://github.com/Freenove/Freenove_ESP32_S3_Display
 *
 * Included by main/app_config.h when
 * CONFIG_DRAFTLING_MODEL_FREENOVE_FNK0104A is selected.
 *
 * The SPI pins, backlight pin/polarity and MADCTL orientation are
 * hard-coded inside components/display/display_ili9341.cpp (shared
 * by every FNK0104 SPI-TFT board, matching the convention already
 * used by display_rgb.cpp for the Sunton boards), so this header
 * carries only the pins that vary per SKU: SD card, touch (none on
 * this SKU), battery ADC and the wakeup button.
 */

#define BOARD_NAME          "Freenove FNK0104A"

/* SD card on the on-chip SDMMC peripheral (1-bit mode: CLK/CMD/D0
 * only -- D1-D3 (GPIO41/48/47) are left unconfigured, matching the
 * existing Waveshare RLCD-4.2 usage of sd_card_init()). */
#define SD_CLK_PIN          38
#define SD_CMD_PIN          40
#define SD_D0_PIN           39

/* I2C bus (unused on this no-touch SKU, kept defined for API
 * compatibility with the shared main.cpp init path). */
#define I2C_SDA_PIN         16
#define I2C_SCL_PIN         15

/* No touch controller on the FNK0104A. */
#define TOUCH_I2C_ADDR      -1
#define TOUCH_INT_PIN       -1
#define TOUCH_RST_PIN       -1
#define TOUCH_NATIVE_W      320
#define TOUCH_NATIVE_H      240
#define TOUCH_SWAP_XY       0
#define TOUCH_MIRROR_X      0
#define TOUCH_MIRROR_Y      0

/* Battery voltage through a 1:2 divider (matches the M5Stack PaperS3
 * ADC backend usage: analogReadMilliVolts() * 2.0 in Freenove's
 * reference Sketch_05.1_Battery_Voltage). */
#define BATT_ADC_PIN        9
#define BATT_EN_PIN         -1
#define BATT_DIVIDER        2

/* BOOT button (KEY_PIN in Freenove's reference sketches) doubles as
 * the deep-sleep wakeup source. */
#define WAKEUP_GPIO_NUM     0
