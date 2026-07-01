#pragma once
/* ----- Waveshare ESP32-S3-RLCD-4.2 -----
 *
 * Pin definitions and board constants for the Waveshare
 * ESP32-S3-RLCD-4.2 reflective LCD board.  Included by
 * main/app_config.h when CONFIG_DRAFTLING_MODEL_WAVESHARE_RLCD42
 * is selected in menuconfig.
 */

#define BOARD_NAME      "Waveshare ESP32-S3-RLCD-4.2"

/* RLCD Display - SPI interface */
#define RLCD_MOSI_PIN   12
#define RLCD_SCK_PIN    11
#define RLCD_DC_PIN     5
#define RLCD_CS_PIN     40
#define RLCD_RST_PIN    41
#define RLCD_TE_PIN     6

/* SD Card - SDMMC 1-bit interface */
#define SD_CLK_PIN      38
#define SD_CMD_PIN      21
#define SD_D0_PIN       39

/* I2C Bus */
#define I2C_SDA_PIN     13
#define I2C_SCL_PIN     14

/* Battery voltage ADC (GPIO4, ADC1_CH3, 3:1 divider) */
#define BATT_ADC_PIN    4
#define BATT_EN_PIN     -1
#define BATT_DIVIDER    3

/* Deep-sleep wakeup on GPIO18 (EXT0, active-low) */
#define WAKEUP_GPIO_NUM 18
