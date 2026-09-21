#pragma once

/*
 * Board configuration: Seeed Studio reTerminal E1001
 *
 * 7.5-inch 800x480 monochrome e-paper display (UC8179 / Good Display
 * GDEY075T7) on SPI, with MicroSD slot sharing the same SPI bus,
 * battery ADC with active-high divider enable, three user buttons,
 * and PCF8563 RTC on I2C.
 *
 * Hardware specifications and pin mappings:
 *   - MCU: ESP32-S3R8 (8 MB Octal PSRAM)
 *   - Flash: 32 MB SPI NOR (W25Q256)
 *   - Display: 7.5-inch 800x480 e-paper, UC8179 controller
 *   - SPI Bus: shared between EPD and MicroSD (SPI2_HOST)
 *       * SCLK: GPIO7
 *       * MISO: GPIO8
 *       * MOSI: GPIO9
 *   - EPD Control:
 *       * CS:   GPIO10 (active low)
 *       * DC:   GPIO11
 *       * RST:  GPIO12 (active low)
 *       * BUSY: GPIO13 (low while busy, high when ready)
 *   - MicroSD Control:
 *       * CS:   GPIO14 (active low)
 *       * DET:  GPIO15 (card detect)
 *       * EN:   GPIO16 (power enable, active high)
 *   - Battery Monitoring:
 *       * ADC:     GPIO1 (ADC1_CH0)
 *       * EN:      GPIO21 (divider enable, active high)
 *       * DIVIDER: 2 (10k / 10k divider)
 *   - Buttons:
 *       * KEY0: GPIO3 (active low, multi-function: confirm / F1 / back / sleep)
 *       * KEY1: GPIO4 (active low, Down)
 *       * KEY2: GPIO5 (active low, Up)
 *   - I2C Buses:
 *       * I2C0: SDA=GPIO19, SCL=GPIO20 (PCF8563 RTC 0x51, SHT40 0x44)
 *       * I2C1: SDA=GPIO39, SCL=GPIO40 (SY6974B charger, touch connector)
 *   - Peripherals:
 *       * LED:    GPIO6
 *       * BUZZER: GPIO45
 */

#define BOARD_NAME          "Seeed reTerminal E1001"

/* E-paper Display (UC8179, 800x480) - SPI interface */
#define EPD_MOSI_PIN        9
#define EPD_MISO_PIN        8
#define EPD_SCK_PIN         7
#define EPD_CS_PIN          10
#define EPD_DC_PIN          11
#define EPD_RST_PIN         12
#define EPD_BUSY_PIN        13

/* MicroSD Card - SPI interface (shares SCK/MOSI/MISO bus with the e-paper) */
#define SD_SPI_MOSI_PIN     9
#define SD_SPI_MISO_PIN     8
#define SD_SPI_SCK_PIN      7
#define SD_SPI_CS_PIN       14
#define SD_DET_PIN          15
#define SD_EN_PIN           16
#define SD_SPI_HOST_NUM     SPI2_HOST

/* Battery voltage ADC (GPIO1, ADC1_CH0, 2:1 divider, enable on GPIO21) */
#define BATT_ADC_PIN        1
#define BATT_EN_PIN         21
#define BATT_DIVIDER        2

/* Buttons:
 *   - KEY0 (GPIO3): deep-sleep wakeup / multi-function (confirm / F1 / back / sleep)
 *   - KEY1 (GPIO4): Down
 *   - KEY2 (GPIO5): Up
 */
#define WAKEUP_GPIO_NUM     3
#define BTN_UP_PIN          5
#define BTN_DOWN_PIN        4

/* I2C Bus 0 (RTC PCF8563 at 0x51, SHT40 sensor at 0x44) */
#define I2C_SDA_PIN         19
#define I2C_SCL_PIN         20

/* User LED and Buzzer */
#define LED_PIN             6
#define BUZZER_PIN          45
