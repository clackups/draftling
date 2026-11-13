#pragma once
/* ----- Espressif ESP32-S31-Korvo-1 + ESP32-S3-LCD-EV-Board-SUB3 -----
 *
 * ESP32-S31-WROOM-3 (16 MB flash, 16 MB octal PSRAM). The
 * ESP32-S3-LCD-EV-Board-SUB3 4.3" subboard on the LCD connector is an
 * 800 x 480 color panel on a 16-bit parallel RGB565 interface, driven
 * by the chip's LCD RGB peripheral (esp_lcd_new_rgb_panel). All panel
 * data / control GPIOs are owned by the RGB display backend
 * (components/display/display_rgb.cpp, block gated on
 * CONFIG_DRAFTLING_RGB_BOARD_S31_KORVO1) and are NOT redefined here:
 *
 *   hsync=44, vsync=45, de=43, pclk=40, no backlight control,
 *   data[16] B0-B4, G0-G5, R0-R4 =
 *     {8,9,10,11,12, 13,14,15,16,17,18, 19,33,34,35,36}
 *   timings: pclk 18 MHz, hsync pw=1 bp=40 fp=20,
 *            vsync pw=1 bp=10 fp=5, pclk_active_neg=1.
 *
 * Pins, timings and the touch / SD wiring come from Espressif's
 * esp_boards component (boards/esp32_s31_korvo_1/board_peripherals.yaml
 * and board_devices.yaml, version 0.7.2) and the board's user guide
 * (docs.espressif.com/projects/esp-dev-kits, esp32-s31-korvo-1).
 *
 * Unused here: the ES8389 codec (I2C 0x10 on the touch bus, I2S on
 * GPIO2-6), the DVP camera (GPIO46-57), the optional SPI NAND (shares
 * the SD pins) and GPIO38 (LCD_CS, also a strapping pin).
 *
 * Included by main/app_config.h when
 * CONFIG_DRAFTLING_MODEL_ESPRESSIF_S31_KORVO1 is selected.
 */

#define BOARD_NAME      "Espressif ESP32-S31-Korvo-1"

/* microSD on the SDMMC host. The ESP32-S31's SDMMC pins are fixed
 * (IOMUX only, no GPIO matrix): slot 0 is CLK=24, CMD=25, D0..D3=20..23,
 * and slot 1 -- the driver's default -- sits on the LCD pins, so the
 * slot is passed explicitly (sd_card_set_sdmmc_slot()). 1-bit mode. */
#define SD_CLK_PIN      24
#define SD_CMD_PIN      25
#define SD_D0_PIN       20
#define SD_SDMMC_SLOT   0
/* SD_CTRL: the card's power switch, active LOW (Espressif's BSP drives
 * it low to power the slot). Also a strapping pin (boot mode 1). */
#define SD_POWER_EN_PIN 39

/* I2C bus shared by the GT1151 touch controller and the ES8389 codec. */
#define I2C_SDA_PIN     0
#define I2C_SCL_PIN     1

/* GT1151 capacitive touch on the LCD subboard, 7-bit address 0x14. Its
 * register map (status 0x814E, 8-byte point records from 0x814F) is
 * the GT911's, so the touchscreen component's GT911 path drives it.
 * No INT or RST line reaches the ESP32-S31: polled. Reports in the
 * panel's native 800x480 landscape frame (Espressif's BSP applies no
 * swap / mirror). */
#define TOUCH_I2C_ADDR  0x14
#define TOUCH_INT_PIN   CONFIG_DRAFTLING_TOUCH_INT_GPIO
#define TOUCH_RST_PIN   CONFIG_DRAFTLING_TOUCH_RST_GPIO
#define TOUCH_NATIVE_W  800
#define TOUCH_NATIVE_H  480
#define TOUCH_SWAP_XY   0
#define TOUCH_MIRROR_X  0
#define TOUCH_MIRROR_Y  0

/* No battery: the board is powered from USB. */
#define BATT_ADC_PIN    -1
#define BATT_EN_PIN     -1
#define BATT_DIVIDER    1

/* BOOT button, GPIO61, active-low. Not an LP IO pin, so it cannot wake
 * the chip from deep sleep (RESET does); polled for the 2 s "forget
 * keyboards" hold. It is also the LCD connector's (unused) LCD_SCK. */
#define WAKEUP_GPIO_NUM 61

/* Four buttons on a resistor ladder into GPIO42 (ADC1 channel 0):
 * SET ~380 mV, MODE ~820 mV, VOL- ~1340 mV, VOL+ ~1870 mV, idle above
 * ~2 V (esp_boards board_devices.yaml). */
#define BTN_ADC_PIN     42

/* On-board WS2812 RGB LED: switched off at boot (ws2812_off()). */
#define BOARD_WS2812_PIN 37

/* PA_CTRL: enable of both NS4150B speaker amplifiers, driven LOW (off). */
#define BOARD_PA_CTRL_PIN 7
