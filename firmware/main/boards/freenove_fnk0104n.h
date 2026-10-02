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
 * Not used by Draftling: the ES8311 audio codec on the touch I2C bus
 * (amplifier enable GPIO1).
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

/* I2C bus carrying the ST77922's touch interface (and the unused
 * ES8311 audio codec). */
#define I2C_SDA_PIN         38
#define I2C_SCL_PIN         39

/* Touch half of the ST77922 TDDI controller (DRAFTLING_TOUCH_ST77922
 * in components/touchscreen). It reports points in the panel's native
 * 320x480 portrait frame, the same frame the display backend draws in
 * (MADCTL 0), so no swap or mirror: the LVGL port's rotation turns
 * the point landscape along with the picture. Pins and register map
 * agree between Freenove's ST77922_Touch.cpp and XiaoZhi's
 * lcdwiki-es3c35p board. */
#define TOUCH_I2C_ADDR      0x55
#define TOUCH_INT_PIN       CONFIG_DRAFTLING_TOUCH_INT_GPIO
#define TOUCH_RST_PIN       CONFIG_DRAFTLING_TOUCH_RST_GPIO
#define TOUCH_NATIVE_W      320
#define TOUCH_NATIVE_H      480
#define TOUCH_SWAP_XY       0
#define TOUCH_MIRROR_X      0
#define TOUCH_MIRROR_Y      0

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
