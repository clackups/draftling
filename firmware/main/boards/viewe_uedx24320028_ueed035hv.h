#pragma once
/* ----- Viewe UEDX24320028E-WB-A + UEED035HV-RX40-L001 -----
 *
 * VIEWE UEDX24320028E-WB-A ESP32-S3 board (ESP32-S3-WROOM-1-N16R8:
 * 16 MB flash, 8 MB octal PSRAM) with the VIEWE UEED035HV-RX40-L001
 * display attached in place of the board's stock 2.8" GC9307 panel:
 * 3.5" 320x480 sunlight-readable transflective color LCD, ST7365
 * controller (ST7796-compatible), rendered landscape at 480x320 by the
 * display backend over 4-wire SPI. It uses Viewe's UEDX32480035E-WB-A
 * panel configuration (same pins, IM straps both HIGH, inversion on).
 * The panel stays readable in ambient light with the backlight fully
 * off, so the backlight setting goes down to 0 %
 * (CONFIG_DRAFTLING_BACKLIGHT_MIN_PCT). No battery.
 *
 * Display: https://viewedisplay.com/product/3-5-inch-320x480-sunlight-readable-transflective-with-super-low-power-tft-lcd-module/
 *
 * Pin assignments come from the vendor repository
 * https://github.com/VIEWESMART/UEDX24320028ESP32-2.8inch-Touch-Display
 * (README, V1.1 schematic, ESP-IDF example and the ESP32_Display_Panel
 * board files BOARD_VIEWE_UEDX24320028E_WB_A.h /
 * BOARD_VIEWE_UEDX32480035E_WB_A.h).
 *
 * Included by main/app_config.h when
 * CONFIG_DRAFTLING_MODEL_VIEWE_UEDX24320028_UEED035HV is selected.
 *
 * The LCD pins (MOSI=45, SCK=40, CS=42, DC=41, RST=39, backlight=13,
 * interface-mode straps IM0=47 / IM1=48) are hard-coded inside
 * components/display/display_ili9341.cpp's ST7365 branch, matching
 * that backend's per-controller pin convention, so this header
 * carries only the SD card, touch and wakeup pins.
 */

#define BOARD_NAME          "Viewe UEDX24320028E-WB-A + UEED035HV"

/* MicroSD on the on-chip SDMMC peripheral. The slot is wired for
 * 4-bit mode (D1=18, D2=15, D3=21); Draftling uses 1-bit mode. */
#define SD_CLK_PIN          14
#define SD_CMD_PIN          17
#define SD_D0_PIN           16

/* I2C bus carrying only the CHSC6540 touch controller. */
#define I2C_SDA_PIN         1
#define I2C_SCL_PIN         3

/* Capacitive touch at 0x2E, Chipsemi CHSC6540-compatible. Its
 * touch-point registers use the FocalTech layout (count at 0x02, first
 * point's X/Y at 0x03..0x06), so the FT6336U poll routine drives it. The
 * chip ignores the register address of a read and always answers from
 * 0x00, which is why that routine reads from 0x00.
 *
 * Native touch frame is the panel's portrait 320x480 in the vendor's
 * MADCTL 0x48 orientation (the vendor firmware applies no touch
 * transform). The backend's landscape MADCTL 0x28 maps logical
 * (x, y) to portrait (319 - y, x), so the inverse is mirror X, then
 * swap -- the same as the Freenove FNK0104B's ILI9341 + FT6336U. */
#define TOUCH_I2C_ADDR      0x2E
#define TOUCH_INT_PIN       CONFIG_DRAFTLING_TOUCH_INT_GPIO
#define TOUCH_RST_PIN       CONFIG_DRAFTLING_TOUCH_RST_GPIO
#define TOUCH_NATIVE_W      320
#define TOUCH_NATIVE_H      480
#define TOUCH_SWAP_XY       1
#define TOUCH_MIRROR_X      1
#define TOUCH_MIRROR_Y      0

/* No battery on this board (USB-C 5 V only). */
#define BATT_ADC_PIN        -1
#define BATT_EN_PIN         -1
#define BATT_DIVIDER        1

/* BOOT button on GPIO0 (external 10k pull-up) is the deep-sleep
 * wakeup source. GPIO0 also carries the WS2812B RGB LED's data line
 * through a diode; Draftling does not drive the LED. */
#define WAKEUP_GPIO_NUM     0
