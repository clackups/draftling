#pragma once
/* ----- LilyGO T5 E-Paper S3 Pro original H752 -----
 *
 * Original H752 / v1.0-240810 hardware revision. This board is not
 * pin-compatible with the newer H752-01 Pro / Pro Lite target: there
 * is no PCA9535/TPS65185 epdiy board path. The panel is driven by
 * LilyGO's older EPD47 shift-register backend using CFG_DATA /
 * CFG_CLK / CFG_STR plus direct CKH/CKV/STH and D0..D7 lines.
 *
 * Included by main/app_config.h when
 * CONFIG_DRAFTLING_MODEL_LILYGO_T5_EPD_S3_PRO_H752 is selected.
 */

#define BOARD_NAME      "LilyGO T5 E-Paper S3 Pro H752"

/* Shared SPI bus: MicroSD and SX1262 LoRa. */
#define SD_SPI_MISO_PIN 8
#define SD_SPI_MOSI_PIN 17
#define SD_SPI_SCK_PIN  18
#define SD_SPI_CS_PIN   16
#define SD_EN_PIN       -1

#define BOARD_LORA_CS_PIN 46
#define BOARD_LORA_RST_PIN 43
#define BOARD_LORA_IRQ_PIN 3
#define BOARD_LORA_BUSY_PIN 44

/* Original H752 has no GPS receiver populated on the board variant
 * this port targets. Keeping the macros at -1 makes the existing
 * T5 radio-sleep helper compile into no-op GPS handling. */
#define BOARD_GPS_TX_PIN -1
#define BOARD_GPS_RX_PIN -1

/* I2C bus carrying GT911, PCF8563 RTC, BQ27220 fuel gauge and
 * BQ25896 charger. There is no PCA9535/TPS65185 on original H752. */
#define I2C_SDA_PIN     6
#define I2C_SCL_PIN     5

/* GT911 capacitive touch controller. Calibrated empirically on the
 * actual panel (corner-tap capture, 2026-06-10): the controller
 * reports in a 540x960 PORTRAIT space rotated 90 degrees from the
 * landscape display. Screen TL/TR/BL/BR map to raw (540,0)/
 * (540,960)/(0,0)/(0,960), so screen_x = raw_y and screen_y =
 * 539 - raw_x: mirror X first, then swap. In Draftling, LVGL works
 * in the logical 480x270 frame when DISPLAY_SCALE=2, so the
 * touchscreen component scales the transformed coordinates down. */
#define TOUCH_I2C_ADDR  0x5D
#define TOUCH_INT_PIN   CONFIG_DRAFTLING_TOUCH_INT_GPIO
#define TOUCH_RST_PIN   CONFIG_DRAFTLING_TOUCH_RST_GPIO
#define TOUCH_NATIVE_W  540
#define TOUCH_NATIVE_H  960
#define TOUCH_SWAP_XY   1
#define TOUCH_MIRROR_X  1
#define TOUCH_MIRROR_Y  0

/* Battery monitor. The BQ27220/BQ25896 I2C chips are present on the
 * original H752. GPIO4 also exposes a battery ADC path in LilyGO's
 * pin map, but Draftling prefers the fuel gauge when available. */
#define BATT_ADC_PIN    -1
#define BATT_EN_PIN     -1
#define BATT_DIVIDER    1

/* Front-light / backlight enable, active HIGH. The H752 display
 * backend drives this via LEDC PWM and the settings menu exposes
 * the normal Backlight percentage cycle. */
#define H752_BL_PIN     40

/* User key. BOOT/GPIO0 is left for bootloader entry; GPIO48 is the
 * only practical user button on this revision. It is also the light
 * sleep wake source used by the H752 standby path. */
#define WAKEUP_GPIO_NUM 48
