#pragma once
/* ----- Waveshare ESP32-S3-Touch-LCD-3.49 -----
 *
 * 3.49" IPS color LCD, 172 x 640 native portrait (presented as
 * 640 x 172 landscape via software rotation), AXS15231B controller
 * over QSPI. Pin assignments below are taken verbatim from the
 * official Waveshare reference firmware
 * (Examples/Arduino/09_LVGL_V8_Test/user_config.h at
 * https://github.com/waveshareteam/ESP32-S3-Touch-LCD-3.49); they
 * differ from the placeholder values used during early bring-up.
 *
 * Touch input is an Allystar AXS5106-family capacitive controller
 * sharing the AXS15231B's I2C protocol family. INT and RST are not
 * wired to the ESP32 on this board (Waveshare's own example also
 * sets both to -1).
 *
 * Included by main/app_config.h when
 * CONFIG_DRAFTLING_MODEL_WAVESHARE_TOUCH_LCD_349 is selected.
 */

#define BOARD_NAME          "Waveshare ESP32-S3-Touch-LCD-3.49"

/* AXS15231B QSPI display interface */
#define LCD_QSPI_CS_PIN     9
#define LCD_QSPI_SCK_PIN    10
#define LCD_QSPI_D0_PIN     11
#define LCD_QSPI_D1_PIN     12
#define LCD_QSPI_D2_PIN     13
#define LCD_QSPI_D3_PIN     14
#define LCD_RST_PIN         21
#define LCD_TE_PIN          -1   /* TE not wired on this board */
/* GPIO 8 drives the BL boost-converter enable, and is wired
 * **active LOW**: a logical LOW on the pin turns the backlight ON
 * at full brightness, a logical HIGH turns it OFF. This is the
 * polarity used by Waveshare's own reference firmware
 * (Examples/ESP-IDF/10_LVGL_V9_Test/components/lcd_bl_pwm_bsp/
 * lcd_bl_pwm_bsp.h), where `LCD_PWM_MODE_255` (the brightest level)
 * is defined as `(0xff - 255) = 0` and `LCD_PWM_MODE_0` (off) is
 * `(0xff - 0) = 255`.
 *
 * We mirror that reference exactly: an LEDC channel on GPIO 8
 * driven at duty 0 keeps the boost circuit on at full brightness,
 * and intermediate duties dim the panel proportionally. The pin is
 * pre-configured as an output with the internal pull-up enabled
 * before the LEDC channel is bound so the line never floats during
 * the boot-to-LEDC handoff. `LCD_BL_ACTIVE_LOW = 1` tells the
 * AXS15231B backend to invert the percent->duty mapping (and to
 * drive the pin HIGH for deep-sleep BL cut). */
#define LCD_BL_PIN          8
#define LCD_BL_ACTIVE_LOW   1

/* On-board MicroSD card slot on a dedicated SPI bus. Pin assignments
 * match the official Waveshare ESP32-S3-Touch-LCD-3.49 pin map
 * (https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-3.49): SD_CS=38,
 * SD_MOSI=39, SD_MISO=40, SD_SCLK=41. */
#define SD_SPI_MOSI_PIN     39
#define SD_SPI_MISO_PIN     40
#define SD_SPI_SCK_PIN      41
#define SD_SPI_CS_PIN       38
#define SD_EN_PIN           -1

/* I2C bus carrying the AXS15231-family touch controller. Pins match
 * Touch_SDA_NUM / Touch_SCL_NUM in Waveshare's user_config.h. */
#define I2C_SDA_PIN         17
#define I2C_SCL_PIN         18

/* AXS5106-family touch controller. INT and RST are not wired on
 * this board (both -1 in Waveshare's reference). The controller
 * reports coordinates in the panel's landscape orientation
 * (640 wide x 172 tall) but with both axes flipped relative to
 * the LCD scan-out (per Waveshare's USER_DISP_ROT_90 touch handler:
 * data->point.x = LCD_H_RES - pointX, data->point.y = LCD_V_RES -
 * pointY), so mirror both X and Y. */
#define TOUCH_I2C_ADDR      0x3B
#define TOUCH_INT_PIN       CONFIG_DRAFTLING_TOUCH_INT_GPIO
#define TOUCH_RST_PIN       CONFIG_DRAFTLING_TOUCH_RST_GPIO
#define TOUCH_NATIVE_W      640
#define TOUCH_NATIVE_H      172
#define TOUCH_SWAP_XY       0
#define TOUCH_MIRROR_X      1
#define TOUCH_MIRROR_Y      1

/* On-board battery monitor.
 *
 * The Touch-LCD-3.49 wires the LiPo cell through a ~3:1 resistive
 * divider into GPIO 4 (ADC1_CH3). Matches the Waveshare BAT_ADC pin
 * map in the official wiki and the `01_ADC_Test/adc_bsp.c` reference
 * firmware, which recovers the cell voltage with
 * `*value = 0.001 * vol * 3` -- i.e. BATT_DIVIDER = 3, not the 2
 * we initially guessed (a 2 multiplier produced a ~18 % reading on
 * a fully charged cell, since the table maps ~2.8 V to ~0 %). */
#define BATT_ADC_PIN        4
#define BATT_EN_PIN         -1
#define BATT_DIVIDER        3

/* Power management.
 *
 * The Touch-LCD-3.49 has a hardware power latch that keeps the
 * battery rail alive after the user releases the PWR button: a
 * TCA9554 I2C IO-expander pin 6 must be driven HIGH at boot to keep
 * the latch closed, and driving it LOW cuts the battery rail and
 * fully powers the board off. The PWR button itself is wired to
 * GPIO 16 (active LOW) -- a momentary press on the unpowered board
 * applies VBAT just long enough for the firmware to latch IO6 HIGH;
 * holding the same button while powered triggers the firmware's
 * long-press handler which auto-saves and cuts the latch.
 *
 * The TCA9554 lives on a *second* I2C bus on GPIO 47 (SDA) / GPIO 48
 * (SCL), shared with the on-board BM8563 RTC and BMI270 IMU; the
 * touch-controller I2C bus on GPIO 17 / 18 above is separate and is
 * owned by the touchscreen component.
 *
 * Pin assignments and the TCA9554 latch bit match the Waveshare
 * reference firmware (Examples/ESP-IDF/07_BATT_PWR_Test/components/
 * user_app/user_app.cpp and i2c_bsp/i2c_bsp.c). */
#define PWR_BUTTON_GPIO     16
#define PWR_I2C_SDA_PIN     47
#define PWR_I2C_SCL_PIN     48
#define PWR_TCA9554_ADDR    0x20  /* A2=A1=A0=0 */
#define PWR_TCA9554_LATCH_BIT 6

/* Deep-sleep wakeup on BOOT (GPIO0, active-low strapping pin).
 * Used only on USB power: on battery, the inactivity timer cuts the
 * power latch (via the `power` component) instead of entering deep
 * sleep, so the wake source is then the PWR button on GPIO 16
 * (which re-applies VBAT and cold-boots the firmware). */
#define WAKEUP_GPIO_NUM     0
