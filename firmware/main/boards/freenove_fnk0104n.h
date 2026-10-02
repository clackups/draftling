#pragma once
/* ----- Freenove FNK0104N -----
 *
 * 3.5" IPS color LCD, 320x480 native portrait, Sitronix ST77922 TDDI
 * controller (display + touch in one chip) on a 4-lane QSPI bus,
 * rendered landscape at 480x320 through the LVGL port's 90-degree base
 * rotation. Bare ESP32-S3R8 chip (8 MB octal PSRAM) with 16 MB flash.
 *
 * Board: https://github.com/Freenove/Freenove_ESP32_S3_Display
 * Pins are taken from that repository's
 * Schematic/3.5inch_ESP32-S3_Display_Schematic.pdf and agree with the
 * vendor driver (Libraries/FNK0104N, TFT_eSPI/ST77922.h). The LCDWiki
 * ES3C35P board supported by XiaoZhi uses the identical pin map.
 *
 * The QSPI panel pins (CS=10, SCK=12, SDA0..3=11/13/14/9), the
 * backlight (GPIO41) and TE (GPIO42, unused) are hard-coded in
 * components/display/display_st77922.cpp, the only user of that
 * backend. The panel's RESET pin is wired to the ESP32-S3's CHIP_PU
 * (EN), so there is no LCD reset GPIO.
 *
 * Not used by Draftling yet: the ST77922's touch interface (I2C 0x55
 * on SDA=38 / SCL=39, INT=47, RST=48) and the ES8311 audio codec on
 * the same I2C bus (amplifier enable GPIO1).
 *
 * Included by main/app_config.h when
 * CONFIG_DRAFTLING_MODEL_FREENOVE_FNK0104N is selected.
 */

#define BOARD_NAME          "Freenove FNK0104N"

/* MicroSD on the on-chip SDMMC peripheral. The slot is wired for
 * 4-bit mode (D1=7, D2=2, D3=3); Draftling uses 1-bit mode. */
#define SD_CLK_PIN          5
#define SD_CMD_PIN          4
#define SD_D0_PIN           6

/* LiPo cell through a 1:2 divider on GPIO8 (BAT_ADC). */
#define BATT_ADC_PIN        8
#define BATT_EN_PIN         -1
#define BATT_DIVIDER        2

/* On-board WS2812 RGB LED. Its data line is otherwise undriven, so
 * the LED latches noise and lights up; main.cpp's ws2812_off() sends
 * it an all-off frame at boot and holds the line low. */
#define BOARD_WS2812_PIN    40

/* BOOT button on GPIO0 is the deep-sleep wakeup source. */
#define WAKEUP_GPIO_NUM     0
