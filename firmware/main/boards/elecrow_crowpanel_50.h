#pragma once
/* ----- Elecrow CrowPanel ESP32 HMI 5.0-inch (DIS07050H) -----
 *
 * ESP32-S3-WROOM-1-N4R8 (4 MB flash, 8 MB octal PSRAM), 5.0" TN color
 * LCD, 800 x 480 native landscape, on a 16-bit parallel RGB565
 * interface driven by the ESP32-S3 LCD RGB peripheral
 * (esp_lcd_new_rgb_panel). All panel data / control GPIOs are owned by
 * the RGB display backend (components/display/display_rgb.cpp, block
 * gated on CONFIG_DRAFTLING_RGB_BOARD_CROWPANEL_50) and are NOT
 * redefined here:
 *
 *   hsync=39, vsync=41, de=40, pclk=0, backlight=GPIO2,
 *   data[16] B0-B4, G0-G5, R0-R4 =
 *     {8,3,46,9,1, 5,6,7,15,16,4, 45,48,47,21,14}
 *   backlight PWM: CONFIG_DRAFTLING_BACKLIGHT_PWM_HZ, 15 kHz by
 *   default (1 kHz whined audibly when dimmed, 25 kHz flickered),
 *   timings: pclk 16 MHz (12 MHz flickered), hsync pw=4 bp=43 fp=8,
 *            vsync pw=4 bp=12 fp=8, pclk_active_neg=1.
 *
 * The data-line map is the same as the Sunton ESP32-8048S043C; the
 * pixel clock is on GPIO0, which is also the BOOT button. Pins and
 * timings come from Elecrow's v3.0 PlatformIO example
 * (github.com/Elecrow-RD/CrowPanel-5.0-HMI-ESP32-Display-800x480,
 * example/V3.0/PlatformIO50) and the v3.0 schematic.
 *
 * Included by main/app_config.h when
 * CONFIG_DRAFTLING_MODEL_ELECROW_CROWPANEL_50 is selected.
 */

#define BOARD_NAME      "Elecrow CrowPanel ESP32 HMI 5.0\""

/* MicroSD slot on its own SPI bus: CS=10, MOSI=11, MISO=13, SCK=12. */
#define SD_SPI_MOSI_PIN 11
#define SD_SPI_MISO_PIN 13
#define SD_SPI_SCK_PIN  12
#define SD_SPI_CS_PIN   10
#define SD_EN_PIN       -1

/* I2C bus (also on the HY2.0-4P "I2C" connector): GT911 touch and,
 * on hardware v3.0, the PCA9557 IO-expander at 0x18. */
#define I2C_SDA_PIN     19
#define I2C_SCL_PIN     20

/* PCA9557 pins (v3.0 boards): IO0 = GT911 RST, IO1 = GT911 INT.
 * main.cpp holds both low, releases RST, then turns INT back into an
 * input -- Elecrow's touch_reset() sequence -- so the GT911 latches
 * address 0x5D. Boards before v3.0 have no PCA9557 and the touchscreen
 * component probes 0x5D and 0x14. */
#define PCA9557_TOUCH_RST_PIN 0
#define PCA9557_TOUCH_INT_PIN 1

/* GT911 capacitive touch controller. Neither INT nor RST reaches an
 * ESP32 GPIO, so the controller is polled. Native frame is the panel's
 * 800x480 landscape (Elecrow's example uses the raw coordinates). */
#define TOUCH_I2C_ADDR  0x5D
#define TOUCH_INT_PIN   CONFIG_DRAFTLING_TOUCH_INT_GPIO
#define TOUCH_RST_PIN   CONFIG_DRAFTLING_TOUCH_RST_GPIO
#define TOUCH_NATIVE_W  800
#define TOUCH_NATIVE_H  480
#define TOUCH_SWAP_XY   0
#define TOUCH_MIRROR_X  0
#define TOUCH_MIRROR_Y  0

/* Battery connector with a charger, but no voltage sense line. */
#define BATT_ADC_PIN    -1
#define BATT_EN_PIN     -1
#define BATT_DIVIDER    1

/* No GPIO wake: BOOT (GPIO0) is the LCD pixel clock net (through a
 * 0 ohm link, no pull-up resistor). The panel is powered whenever the
 * chip is out of reset, deep sleep included, and keeps the line LOW,
 * so the button cannot be read. The device wakes by RESET. */
#define WAKEUP_GPIO_NUM -1
