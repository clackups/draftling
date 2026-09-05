#pragma once
/* ----- Sunton ESP32-8048S043C -----
 *
 * 4.3" IPS color LCD, 800 x 480 native landscape, ST7262 panel on a
 * 16-bit parallel RGB565 interface driven by the ESP32-S3 LCD RGB
 * peripheral (esp_lcd_new_rgb_panel). All panel data / control GPIOs
 * are owned by the RGB display backend (components/display/
 * display_rgb.cpp) and are NOT redefined here -- they are baked into
 * the backend's S043 block (gated on CONFIG_DRAFTLING_RGB_BOARD_S043):
 *
 *   hsync=39, vsync=41, de=40, pclk=42, disp=-1, backlight=GPIO2,
 *   data[16] in R,G,B order (RGB565 LSB-first) =
 *     {8,3,46,9,1, 5,6,7,15,16,4, 45,48,47,21,14}
 *   timings: pclk 12.5 MHz, hsync pw=4 bp=8 fp=8,
 *            vsync pw=4 bp=8 fp=8, pclk_active_neg=1.
 *
 * The control-pin map (VSYNC/DE swapped), panel timings and data-line
 * order all differ from the 7" ESP32-8048S070C; everything Draftling
 * touches itself (SD card, I2C touch, wakeup button) is the same as
 * the 7" board's standard Sunton ESP32-8048S0xx pinout. Verified
 * against the rzeldent/platformio-espressif32-sunton board reference.
 *
 * Included by main/app_config.h when
 * CONFIG_DRAFTLING_MODEL_SUNTON_8048S043 is selected.
 */

#define BOARD_NAME      "Sunton ESP32-8048S043C"

/* On-board MicroSD slot on a dedicated SPI bus (not the RGB bus).
 * Standard Sunton ESP32-8048S0xx pinout: CS=10, MOSI=11, MISO=13,
 * SCK=12. */
#define SD_SPI_MOSI_PIN 11
#define SD_SPI_MISO_PIN 13
#define SD_SPI_SCK_PIN  12
#define SD_SPI_CS_PIN   10
#define SD_EN_PIN       -1

/* I2C bus carrying the GT911 capacitive touch controller. Pins per
 * the Sunton reference: SDA=19, SCL=20, RST=38, INT=NC. */
#define I2C_SDA_PIN     19
#define I2C_SCL_PIN     20

/* GT911 capacitive touch controller. Touch is optional on this
 * board (Draftling is keyboard-driven); these are kept for a future
 * touch enable. The panel is natively landscape 800x480 so no axis
 * swap is needed. INT is not wired on the Sunton reference. */
#define TOUCH_I2C_ADDR  0x5D
#define TOUCH_INT_PIN   CONFIG_DRAFTLING_TOUCH_INT_GPIO
#define TOUCH_RST_PIN   CONFIG_DRAFTLING_TOUCH_RST_GPIO
#define TOUCH_NATIVE_W  800
#define TOUCH_NATIVE_H  480
#define TOUCH_SWAP_XY   0
#define TOUCH_MIRROR_X  0
#define TOUCH_MIRROR_Y  0

/* No on-board battery monitor on this dev board. */
#define BATT_ADC_PIN    -1
#define BATT_EN_PIN     -1
#define BATT_DIVIDER    1

/* Deep-sleep wakeup on the BOOT button (GPIO0, active-low strapping
 * pin with a board-level pull-up; RTC-capable so EXT0 wake works). */
#define WAKEUP_GPIO_NUM 0
