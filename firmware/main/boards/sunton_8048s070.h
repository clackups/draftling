#pragma once
/* ----- Sunton ESP32-8048S070C -----
 *
 * 7" TN color LCD, 800 x 480 native landscape, 16-bit parallel
 * RGB565 interface driven by the ESP32-S3 LCD RGB peripheral
 * (esp_lcd_new_rgb_panel). All panel data / control GPIOs are owned
 * by the RGB display backend (components/display/display_rgb.cpp)
 * and are NOT redefined here -- they are baked into the backend:
 *
 *   hsync=39, vsync=40, de=41, pclk=42, disp=-1, backlight=GPIO2,
 *   data[16] in B,G,R order (RGB565 LSB-first) =
 *     {15,7,6,5,4, 9,46,3,8,16,1, 14,21,47,48,45}
 *   (the physical wiring is R,G,B; esp_lcd maps data_gpio_nums[0]
 *    to the RGB565 LSB, so blue must come first or R/B swap).
 *   timings: pclk 12 MHz, hsync pw=2 bp=43 fp=8,
 *            vsync pw=2 bp=12 fp=8, pclk_idle_high=1.
 *
 * Pins below are the ones Draftling touches outside the display
 * backend (SD card, I2C touch, wakeup button). References:
 *   https://www.haraldkreuzer.net/aktuelles/erste-schritte-mit-dem-
 *     sunton-esp32-s3-7-zoll-display-lovyangfx-und-lvgl
 *   https://www.makerfabs.com/sunton-esp32-s3-7-inch-tn-display-with-touch.html
 *
 * Included by main/app_config.h when
 * CONFIG_DRAFTLING_MODEL_SUNTON_8048S070 is selected.
 */

#define BOARD_NAME      "Sunton ESP32-8048S070C"

/* On-board MicroSD slot on a dedicated SPI bus (not the RGB bus).
 * Standard Sunton ESP32-8048S0xx pinout: CS=10, MOSI=11, MISO=13,
 * SCK=12. */
#define SD_SPI_MOSI_PIN 11
#define SD_SPI_MISO_PIN 13
#define SD_SPI_SCK_PIN  12
#define SD_SPI_CS_PIN   10
#define SD_EN_PIN       -1

/* I2C bus carrying the GT911 capacitive touch controller. Pins per
 * the LovyanGFX Sunton reference: SDA=19, SCL=20, RST=38, INT=NC. */
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
