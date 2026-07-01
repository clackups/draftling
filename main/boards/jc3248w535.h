#pragma once
/* ----- Guition JC3248W535 -----
 *
 * 3.5" IPS color LCD, 480 x 320, AXS15231B controller over QSPI.
 * Pin assignments below come from the JC3248W535 reference schematic
 * widely shared in the LovyanGFX / LVGL community for this board;
 * verify against the actual silk-screen of your unit if init fails.
 *
 * Touch input (AXS5106L capacitive controller, I2C address 0x3B)
 * is used as a secondary input device on this board (the JC3248W535
 * has no user buttons besides the power switch). The touch INT line
 * doubles as the EXT0 deep-sleep wake source. The touch pins are
 * configurable via menuconfig under "DRAFTLING Configuration ->
 * Touchscreen INT/RST GPIO" so a non-stock wiring can be supported
 * without code changes.
 *
 * Included by main/app_config.h when
 * CONFIG_DRAFTLING_MODEL_JC3248W535 is selected.
 */

#define BOARD_NAME          "Guition JC3248W535"

/* AXS15231B QSPI display interface */
#define LCD_QSPI_CS_PIN     45
#define LCD_QSPI_SCK_PIN    47
#define LCD_QSPI_D0_PIN     21
#define LCD_QSPI_D1_PIN     48
#define LCD_QSPI_D2_PIN     40
#define LCD_QSPI_D3_PIN     39
/* The AXS15231B has no dedicated external reset pin on this
 * board (the Arduino reference driver passes GFX_NOT_DEFINED);
 * the controller is reset by its vendor unlock/lock SPI sequence.
 * GPIO4 is in fact the touch I2C SDA line -- driving it as a
 * push-pull RST output during display init would hold touch SDA
 * low and prevent the I2C bus from ever coming up. */
#define LCD_RST_PIN         -1
#define LCD_TE_PIN          38
#define LCD_BL_PIN          1

/* SD card on a dedicated SPI bus */
#define SD_SPI_MOSI_PIN     11
#define SD_SPI_MISO_PIN     13
#define SD_SPI_SCK_PIN      12
#define SD_SPI_CS_PIN       10
#define SD_EN_PIN           -1

/* I2C bus carrying the AXS15231B integrated touch controller.
 * Per the JC3248W535 board pinout (Tactility, Yoradio, edgerun
 * and multiple community references): SDA=GPIO4, SCL=GPIO8. */
#define I2C_SDA_PIN         4
#define I2C_SCL_PIN         8

/* AXS5106L touch controller. INT defaults to GPIO3 (RTC-capable,
 * required for wake-on-touch). The reset line is tied to the LCD
 * reset on this board so we leave the dedicated touch RST at -1.
 *
 * The controller reports coordinates in the panel's native portrait
 * orientation (320 wide x 480 tall). The LCD backend software-
 * rotates to 480x320 landscape, so we swap XY and mirror X on the
 * touch side to match. */
#define TOUCH_I2C_ADDR      0x3B
#define TOUCH_INT_PIN       CONFIG_DRAFTLING_TOUCH_INT_GPIO
#define TOUCH_RST_PIN       CONFIG_DRAFTLING_TOUCH_RST_GPIO
#define TOUCH_NATIVE_W      320
#define TOUCH_NATIVE_H      480
#define TOUCH_SWAP_XY       1
#define TOUCH_MIRROR_X      1
#define TOUCH_MIRROR_Y      0

/* No on-board battery monitor on this dev board */
#define BATT_ADC_PIN        -1
#define BATT_EN_PIN         -1
#define BATT_DIVIDER        1

/* Deep-sleep wakeup. The JC3248W535 has no BOOT button on an
 * RTC-capable GPIO, so the only way to wake the device is via the
 * touch INT line -- enabled by setting both DRAFTLING_TOUCHSCREEN
 * and DRAFTLING_STANDBY_WAKE_ON_TOUCH (the per-board default).
 * WAKEUP_GPIO_NUM below is just the fallback; the standby code
 * overrides it with CONFIG_DRAFTLING_TOUCH_INT_GPIO when
 * wake-on-touch is enabled. */
#define WAKEUP_GPIO_NUM     0
