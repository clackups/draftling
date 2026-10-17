#pragma once
/* ----- MicroJournal Rev 5.0 -----
 *
 * ESP32-S3 N16R8 writerdeck with an ILI9341 SPI LCD, an on-board SPI
 * MicroSD reader sharing the LCD bus, and a native USB host connector.
 * The panel is configured as 320x240 landscape. Backlight wiring was
 * not provided, so software brightness control is disabled.
 */

#define BOARD_NAME          "MicroJournal Rev 5.0"

/* ILI9341 SPI panel (passed to display_ili9341.cpp through
 * display_spi_tft_config()). MISO is wired because the MicroSD card
 * shares this bus. No software backlight control. */
#define LCD_MOSI_PIN        11
#define LCD_MISO_PIN        13
#define LCD_SCK_PIN         12
#define LCD_CS_PIN          10
#define LCD_DC_PIN          46
#define LCD_RST_PIN         3
#define LCD_BL_PIN          -1
#define LCD_IM0_PIN         -1
#define LCD_IM1_PIN         -1
#define LCD_SPI_CLOCK_HZ    (40 * 1000 * 1000)
#define LCD_BL_PWM_HZ       50000

/* On-board MicroSD shares the display's SPI2 bus, with its own CS. */
#define SD_SPI_MISO_PIN     13
#define SD_SPI_MOSI_PIN     11
#define SD_SPI_SCK_PIN      12
#define SD_SPI_CS_PIN       9
#define SD_EN_PIN           -1
#define SD_SPI_HOST_NUM     SPI2_HOST

/* No touch or battery hardware was specified. */
#define I2C_SDA_PIN         -1
#define I2C_SCL_PIN         -1
#define TOUCH_I2C_ADDR      -1
#define TOUCH_INT_PIN       -1
#define TOUCH_RST_PIN       -1
#define TOUCH_NATIVE_W      320
#define TOUCH_NATIVE_H      240
#define TOUCH_SWAP_XY       0
#define TOUCH_MIRROR_X      0
#define TOUCH_MIRROR_Y      0
#define BATT_ADC_PIN        -1
#define BATT_EN_PIN         -1
#define BATT_DIVIDER        1

/* No user wake button is wired. GPIO0 is reserved for hardware power
 * control and is not configured by firmware. */
#define WAKEUP_GPIO_NUM     -1
