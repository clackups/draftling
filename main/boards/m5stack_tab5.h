#pragma once
/* ----- M5Stack Tab5 -----
 *
 * 5" IPS color LCD, 1280 x 720 landscape, driven over MIPI-DSI
 * (2 data lanes, DPI continuous scanout) by the ESP32-P4. Two
 * hardware revisions exist in the wild:
 *   v1: ILI9881C panel + GT911 capacitive touch (default in BSP)
 *   v2: ST7123 panel + ST7123 touch (auto-detected by I2C probe)
 *
 * All panel + power-rail bring-up is handled by the upstream
 * espressif/m5stack_tab5 BSP managed component, which is pulled in
 * by components/display/idf_component.yml for the esp32p4 target.
 * That includes:
 *   - PI4IOE5V6408 I/O expander (I2C addr 0x43 / 0x44) that drives
 *     LCD_EN (IO4) and TOUCH_EN (IO5) power rails;
 *   - MIPI-DSI PHY LDO power-up;
 *   - the ILI9881C / ST7123 init data blocks;
 *   - LEDC PWM backlight on GPIO22.
 *
 * Pins listed below are the ones Draftling itself touches outside
 * of the BSP (SD card, shared I2C bus carrying the GT911 touch).
 * See https://docs.m5stack.com/en/core/Tab5 for the full pinout.
 *
 * Included by main/app_config.h when
 * CONFIG_DRAFTLING_MODEL_M5STACK_TAB5 is selected.
 */

#define BOARD_NAME          "M5Stack Tab5"

/* On-board MicroSD slot, wired to ESP32-P4 SDMMC pins. The Draftling
 * SD layer uses a generic SPI bus (sd_card_init_spi) on every board
 * except the Waveshare RLCD-4.2; map the SDMMC pins (per BSP
 * m5stack_tab5.h: BSP_SD_SPI_CLK=43, BSP_SD_SPI_MISO=39,
 * BSP_SD_SPI_MOSI=44, BSP_SD_SPI_CS=42) to the SPI alias so the
 * existing SPI3-host code path works without change. */
#define SD_SPI_MOSI_PIN     44
#define SD_SPI_MISO_PIN     39
#define SD_SPI_SCK_PIN      43
#define SD_SPI_CS_PIN       42
#define SD_EN_PIN           -1

/* I2C bus shared by the GT911 touch controller, PI4IOE5V6408
 * I/O expander, BMI270 IMU, audio codecs and other on-board
 * peripherals (BSP_I2C_SDA=31, BSP_I2C_SCL=32). The MIPI-DSI
 * display backend creates this bus via bsp_i2c_init() and the
 * touchscreen component shares it via touchscreen_config_t.i2c_bus
 * (same pattern as the LilyGO T5 E-Paper S3 Pro / Pro Lite). */
#define I2C_SDA_PIN         31
#define I2C_SCL_PIN         32

/* M5Stack Tab5 touch controller. Two hardware variants exist:
 *
 *   - v1: Goodix GT911 capacitive touch at I2C 0x14 (backup
 *         address). RST is not wired to any ESP32-P4 GPIO, so
 *         the GT911 internal POR is triggered by power-cycling
 *         BSP_TOUCH_EN through the first PI4IOE5V6408. main.cpp
 *         pre-drives GPIO23 LOW (overriding the 3V3 pull-up on
 *         INT) before that toggle so the GT911 latches its
 *         backup address rather than the unused 0x5D.
 *
 *   - v2: Sitronix ST7123 integrated panel+touch at I2C 0x55.
 *         No INT-latch dance is needed.
 *
 * Rather than re-implement the per-variant bring-up, the
 * Draftling touchscreen component delegates to the upstream
 * espressif/m5stack_tab5 BSP's bsp_touch_new() on this board.
 * The BSP probes 0x55 first (ST7123, board v2) and falls back
 * to 0x14 backup (GT911, board v1), instantiates the matching
 * esp_lcd_touch_* driver, and returns a generic
 * esp_lcd_touch_handle_t. touchscreen.cpp polls through
 * esp_lcd_touch_read_data + esp_lcd_touch_get_coordinates so
 * the same source handles both variants transparently. The
 * pre-display TOUCH_EN power-cycle in main.cpp still runs (it
 * is the only thing that fixes the v1 GT911 address latch).
 *
 * TOUCH_I2C_ADDR / TOUCH_RST_PIN below are kept as nominal
 * (v1 GT911) values for the legacy touchscreen_config_t struct;
 * the BSP path ignores them and uses BSP_LCD_TOUCH_INT for INT
 * and GPIO_NUM_NC for RST. Native panel coordinate range is
 * 720 x 1280 portrait (matches BSP_LCD_H_RES / BSP_LCD_V_RES).
 *
 * draftling_lvgl_port_init() is called with the user-configured
 * DISPLAY_ROTATE (default 270 for landscape) and LVGL v9.2 then
 * rotates indev points itself inside indev_pointer_proc(). So
 * touchscreen.cpp must hand LVGL coords in the *pre-rotation*
 * portrait frame -- main.cpp passes user_rotate_deg=0 for that
 * reason. Empirically (Tab5 v1, GT911 via BSP), the controller's
 * native axes are inverted on both axes relative to the panel's
 * portrait pixel axes: a tap at landscape upper-left produces
 * raw_x near 0 (small end of the 0..720 short-axis range) and
 * raw_y near 1280 (large end of the long-axis range), which
 * corresponds to panel-native portrait corner (0, 1280-ish) --
 * the opposite portrait corner from what LVGL's rotation-270
 * indev transform expects for landscape (0, 0). Mirroring both
 * X and Y in native_to_logical() flips raw onto panel-native
 * portrait so LVGL's own indev rotation lands the cursor under
 * the finger. */
#define TOUCH_I2C_ADDR      0x14
#define TOUCH_INT_PIN       CONFIG_DRAFTLING_TOUCH_INT_GPIO
#define TOUCH_RST_PIN       CONFIG_DRAFTLING_TOUCH_RST_GPIO
#define TOUCH_NATIVE_W      720
#define TOUCH_NATIVE_H      1280
#define TOUCH_SWAP_XY       0
#define TOUCH_MIRROR_X      1
#define TOUCH_MIRROR_Y      1

/* No board-managed battery ADC: Tab5 carries a 2S NP-F550 Li-ion
 * pack monitored by an INA226 power monitor at I2C 0x41 (on the
 * system I2C bus, SDA=31 / SCL=32 above). main.cpp calls
 * battery_init_ina226(bsp_i2c_get_handle(), 0x41, 2) under
 * CONFIG_DRAFTLING_BATTERY_INA226 (defaulted y for this board), and
 * the editor status bar picks the percentage up via
 * DRAFTLING_HAS_BATTERY. The ADC-based battery_init() stays a
 * no-op (BATT_ADC_PIN=-1). */
#define BATT_ADC_PIN        -1
#define BATT_EN_PIN         -1
#define BATT_DIVIDER        1

/* Standby on Tab5 uses real deep sleep (esp_deep_sleep_start). The
 * touch INT pin (GPIO 23) is not an LP_IO on the ESP32-P4 and no
 * user button is wired to a LP_IO either, so there is no usable
 * GPIO wake source: the only wake path is the hardware RESET
 * button (cold boot, autosave restores the editor state).
 * WAKEUP_GPIO_NUM is set anyway for code paths that reference it
 * directly but is NOT armed as an EXT0/LP_IO wake source on the
 * P4 (see components/standby/standby.cpp). */
#define WAKEUP_GPIO_NUM     0
