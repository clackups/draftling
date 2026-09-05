#pragma once
/* ----- M5Stack PaperS3 -----
 *
 * The PaperS3 drives an ED047TC1 e-paper panel (960 x 540 native
 * landscape) via the ESP32-S3 LCD/I80 parallel peripheral, fed by
 * the vroland/epdiy library through the in-tree
 * components/display/epd_board_papers3.c board definition.
 *
 * That board definition owns GPIOs 6-18 (8-bit data bus + CKH, CKV,
 * STH, SPV, XLE) and GPIO 45/46 (EPD_EN and BST_EN power-rail gates).
 * Pins below are the ones Draftling itself touches outside of the
 * display driver (SD card, wakeup, I2C touch / RTC / IMU, battery
 * monitor). See the M5Stack hardware reference for the full pin
 * list: https://docs.m5stack.com/en/core/papers3
 *
 * Included by main/app_config.h when
 * CONFIG_DRAFTLING_MODEL_M5STACK_PAPERS3 is selected.
 */

#define BOARD_NAME      "M5Stack PaperS3"

/* Onboard MicroSD on a dedicated SPI host (SPI3 - the EPD parallel
 * bus owned by epdiy claims GPIO 6-18 plus 45/46 for its data /
 * control / power lines, so the SD slot must use a separate set of
 * pins).
 *
 * Pin assignments verified against the M5Stack PaperS3 hardware
 * reference and the FastEPD/Arduino "papers3_screenshot" example:
 *   CS=47, SCK=39, MOSI=38, MISO=40. */
#define SD_SPI_MOSI_PIN 38
#define SD_SPI_MISO_PIN 40
#define SD_SPI_SCK_PIN  39
#define SD_SPI_CS_PIN   47
#define SD_EN_PIN       -1

/* I2C bus (BM8563 RTC, BMI270 IMU and GT911 capacitive touch
 * controller on the PaperS3) */
#define I2C_SDA_PIN     41
#define I2C_SCL_PIN     42

/* GT911 capacitive touch controller.
 *
 * Pin assignments match the M5Stack PaperS3 hardware reference
 * (pin_int=GPIO48, pin_sda=GPIO41, pin_scl=GPIO42, no RST GPIO).
 * The GT911 reset line is not wired to any ESP32-S3 pin on this
 * board -- it is released by the power-rail RC, so the touchscreen
 * driver cannot perform the INT-driven address-select reset
 * sequence and instead probes both possible I2C addresses (0x5D
 * primary, 0x14 backup).
 *
 * Native panel coordinate range as reported by the GT911 is
 * 540 wide x 960 tall (portrait). The epdiy backend rotates the
 * framebuffer to a 960x540 landscape orientation
 * (EPD_ROT_LANDSCAPE), so the touch axes need swapping and the
 * vertical axis needs inverting -- swap_xy=1, mirror_x=1,
 * mirror_y=0. */
#define TOUCH_I2C_ADDR  0x5D
#define TOUCH_INT_PIN   CONFIG_DRAFTLING_TOUCH_INT_GPIO
#define TOUCH_RST_PIN   CONFIG_DRAFTLING_TOUCH_RST_GPIO
#define TOUCH_NATIVE_W  540
#define TOUCH_NATIVE_H  960
#define TOUCH_SWAP_XY   1
#define TOUCH_MIRROR_X  1
#define TOUCH_MIRROR_Y  0

/* Battery voltage monitor.
 *
 * The PaperS3 routes the LiPo cell through a 1:2 resistive divider
 * (R_top == R_bottom, V_pin = V_bat / 2) into ADC1 channel 2 on
 * GPIO3. There is no enable transistor -- the divider is always
 * powered. Pin and divider ratio match the M5Unified Power_Class
 * configuration for board_M5PaperS3 (BAT_ADC = ADC1_GPIO3,
 * adc_ratio = 2.0). The battery component will multiply the
 * measured pin voltage by BATT_DIVIDER (=2) to recover the actual
 * cell voltage.
 *
 * GPIO4 carries the charger CHG_STAT signal on the PaperS3 (active
 * low while charging). It is not wired through the battery API yet;
 * the editor only displays the cell percentage. */
#define BATT_ADC_PIN    3
#define BATT_EN_PIN     -1
#define BATT_DIVIDER    2

/* Deep-sleep wakeup on the BOOT button (GPIO0, active-low).
 *
 * Earlier revisions tried GPIO21 (wrong -- that's the on-board
 * buzzer/speaker pin) and GPIO48 (the GT911 touch-panel INT line).
 * GPIO48 also failed because the GT911 holds its INT line low while
 * uninitialised (the line doubles as I2C-address selection during
 * reset), so the standby manager would see GPIO48 stuck low and
 * fire the GPIO_INTR_LOW_LEVEL wake immediately.
 *
 * GPIO0 is the only digital input button on the PaperS3 besides the
 * hardware power switch, and -- being an ESP32-S3 strapping pin with
 * a board-level pull-up -- is guaranteed high while the device is
 * idle. It is also RTC-capable, so we can use real EXT0 deep sleep
 * (matching the other supported boards) instead of the light-sleep
 * + esp_restart() workaround. */
#define WAKEUP_GPIO_NUM 0
