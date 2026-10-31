#pragma once
/* ----- Guition JC4827W543C (4.3" QSPI smart display) -----
 *
 * ESP32-S3-WROOM-1-N4R8 (4 MB flash, 8 MB octal PSRAM), 4.3" IPS
 * 480x272 panel with an NV3041A controller on a 4-lane QSPI bus
 * (components/display/display_nv3041a.cpp), GT911 capacitive touch,
 * MicroSD on SPI, NS4168 I2S speaker amplifier, IP5306 battery
 * charger / boost (no fuel gauge, no battery ADC).
 *
 * Pins from the vendor IO table ("4.3 inches IO pin distribution.xls"
 * in github.com/profi-max/JC4827W543_4.3inch_ESP32S3_board), which
 * agrees with that repository's Arduino demos and ESPHome's device
 * config (devices.esphome.io/devices/guition-jc4827543c). The
 * resistive-touch variant (JC4827W543R, XPT2046 on the SD card's SPI
 * bus) is not supported.
 *
 * Included by main/app_config.h when
 * CONFIG_DRAFTLING_MODEL_GUITION_JC4827W543 is selected.
 */

#define BOARD_NAME          "Guition JC4827W543C"

/* NV3041A QSPI display interface; no reset line. The panel's TE
 * output is wired to GPIO0 (BOOT) and left disabled. */
#define LCD_QSPI_CS_PIN     45
#define LCD_QSPI_SCK_PIN    47
#define LCD_QSPI_D0_PIN     21
#define LCD_QSPI_D1_PIN     48
#define LCD_QSPI_D2_PIN     40
#define LCD_QSPI_D3_PIN     39
#define LCD_BL_PIN          1
/* Arduino_GFX's NV3041A maximum. */
#define LCD_SPI_CLOCK_HZ    (32 * 1000 * 1000)
/* LEDC frequency of the vendor demos. */
#define LCD_BL_PWM_HZ       5000

/* MicroSD slot on its own SPI bus (SPI3). The vendor table labels
 * GPIO11 "TF_MISO" and GPIO13 "TF_MOSI", but its XPT2046 demo (same
 * lines) uses 11 = MOSI, 13 = MISO, as does every other Guition
 * board with this layout. */
#define SD_SPI_MOSI_PIN     11
#define SD_SPI_MISO_PIN     13
#define SD_SPI_SCK_PIN      12
#define SD_SPI_CS_PIN       10
#define SD_EN_PIN           -1

/* I2C bus carrying the GT911 touch controller. */
#define I2C_SDA_PIN         8
#define I2C_SCL_PIN         4

/* GT911: INT on GPIO3, RST on GPIO38 (the touchscreen component
 * probes both GT911 addresses after the reset). With the panel's
 * MADCTL 0xC0 the GT911 reports in the picture's own 480x272 frame
 * (checked on hardware: a bottom-right tap reads raw (465, 243)), as
 * in the vendor demo. ESPHome's config mirrors X and Y only to undo
 * its own 180-degree display rotation. */
#define TOUCH_I2C_ADDR      0x5D
#define TOUCH_INT_PIN       CONFIG_DRAFTLING_TOUCH_INT_GPIO
#define TOUCH_RST_PIN       CONFIG_DRAFTLING_TOUCH_RST_GPIO
#define TOUCH_NATIVE_W      480
#define TOUCH_NATIVE_H      272
#define TOUCH_SWAP_XY       0
#define TOUCH_MIRROR_X      0
#define TOUCH_MIRROR_Y      0

/* NS4168 I2S speaker amplifier, unused. Its enable is not wired to a
 * GPIO; the pre-sleep hook drives these lines LOW and holds them so
 * the amplifier sees no clock and no floating inputs. */
#define SPK_I2S_LRCLK_PIN   2
#define SPK_I2S_BCLK_PIN    42
#define SPK_I2S_DIN_PIN     41

/* No battery level indication: the IP5306 charger and the battery are
 * not connected to the ESP32-S3 (no voltage divider, no fuel gauge). */
#define BATT_ADC_PIN        -1
#define BATT_EN_PIN         -1
#define BATT_DIVIDER        1

/* Deep-sleep wakeup on the BOOT button (GPIO0, RTC-capable). The
 * GT911 is put to sleep before deep sleep, so its INT line cannot
 * wake the board. */
#define WAKEUP_GPIO_NUM     0
