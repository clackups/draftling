#pragma once
/* ----- VIEWE UEDX80480043E-WB-A / -WB-B (4.3" smart display) -----
 *
 * ESP32-S3 (16 MB flash, 8 MB octal PSRAM), 4.3" color LCD, 800 x 480
 * native landscape, ST7262 panel on a 16-bit parallel RGB565
 * interface driven by the ESP32-S3 LCD RGB peripheral
 * (esp_lcd_new_rgb_panel). All panel data / control GPIOs are owned by
 * the RGB display backend (components/display/display_rgb.cpp, block
 * gated on CONFIG_DRAFTLING_RGB_BOARD_UEDX80480043) and are NOT
 * redefined here:
 *
 *   hsync=39, vsync=41, de=40, pclk=42, backlight=GPIO2 (active high),
 *   data[16] B0-B4, G0-G5, R0-R4 =
 *     {8,3,46,9,1, 5,6,7,15,16,4, 45,48,47,21,14}
 *   timings: pclk 16 MHz, hsync pw=4 bp=40 fp=20,
 *            vsync pw=4 bp=16 fp=16, pclk_active_neg=1.
 *
 * The pin map is the Sunton ESP32-8048S043C's; only the panel timings
 * differ. Pins and timings come from VIEWE's ESP-IDF BSP
 * (viewesmart/bsp_uedx80480043_wb_a 1.0.0 in the Espressif Component
 * Registry) and github.com/VIEWESMART/UEDX80480043ESP32-4.3inch-Touch-
 * Display. VIEWE states there is no software difference between the
 * WB-A and the WB-B revision.
 *
 * Included by main/app_config.h when
 * CONFIG_DRAFTLING_MODEL_VIEWE_UEDX80480043 is selected.
 */

#define BOARD_NAME      "VIEWE UEDX80480043E-WB"

/* MicroSD slot on its own SPI bus: CS=10, MOSI=11, MISO=13, SCK=12. */
#define SD_SPI_MOSI_PIN 11
#define SD_SPI_MISO_PIN 13
#define SD_SPI_SCK_PIN  12
#define SD_SPI_CS_PIN   10
#define SD_EN_PIN       -1

/* I2C bus carrying the GT911 touch controller. */
#define I2C_SDA_PIN     19
#define I2C_SCL_PIN     20

/* GT911 capacitive touch controller. RST is GPIO38. INT reaches
 * GPIO18 on the touch connector, but VIEWE's own BSP leaves it unused
 * and polls, as Draftling does (-1); the touchscreen component then
 * probes both GT911 addresses. Native frame is the panel's 800x480
 * landscape (the BSP uses the raw coordinates). */
#define TOUCH_I2C_ADDR  0x5D
#define TOUCH_INT_PIN   CONFIG_DRAFTLING_TOUCH_INT_GPIO
#define TOUCH_RST_PIN   CONFIG_DRAFTLING_TOUCH_RST_GPIO
#define TOUCH_NATIVE_W  800
#define TOUCH_NATIVE_H  480
#define TOUCH_SWAP_XY   0
#define TOUCH_MIRROR_X  0
#define TOUCH_MIRROR_Y  0

/* No battery: the board is powered from USB only. */
#define BATT_ADC_PIN    -1
#define BATT_EN_PIN     -1
#define BATT_DIVIDER    1

/* Deep-sleep wakeup on the BOOT button (GPIO0, RTC-capable). Unlike
 * the CrowPanel 5.0", GPIO0 is not the LCD pixel clock here. */
#define WAKEUP_GPIO_NUM 0
