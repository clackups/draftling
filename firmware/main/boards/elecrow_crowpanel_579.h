#pragma once
/* ----- Elecrow CrowPanel ESP32-S3 5.79" E-Paper HMI Display -----
 *
 * ESP32-S3-WROOM-1-N8R8 (8 MB flash, 8 MB PSRAM) driving a 5.79"
 * 792x272 black/white e-paper panel over plain SPI. The panel is
 * built from two SSD1683 controllers, one per half (left/right),
 * sharing a single SPI bus (CS/DC/RST/BUSY); the display backend
 * (components/display/display_ssd1683.cpp) drives both halves with
 * the dual-window command sequence ported from the community
 * ESPHome driver at github.com/samperk1/esphome-crowpanel-579
 * (reverse-engineered and photograph-verified against real
 * hardware). All EPD panel GPIOs and the panel power-enable pin are
 * hard-coded directly inside display_ssd1683.cpp -- this backend is
 * used by exactly one board, matching the convention already used
 * by display_ili9341.cpp / display_xteink_epd.cpp.
 *
 * Buttons: a Menu button (GPIO2) doubles as the deep-sleep wake
 * source and, during normal operation, as an F1 (open/close the
 * Settings menu) key on short press / "forget all BLE keyboards" on
 * a 2 s hold -- see crowpanel_menu_key_init() in main.cpp (same
 * convention as the LilyGO T5 E-Paper S3 Pro H752 side key). A Back
 * button (GPIO1) and a 3-way dial switch (Up/Down/OK -- physically a
 * rocker-style navigation switch on this board, not a rotary
 * encoder) round out full menu navigation without a keyboard:
 * Back -> Esc, dial Up/Down -> arrow keys, dial press (OK) -> Enter.
 * See crowpanel_nav_init() in main.cpp.
 *
 * No on-board battery ADC or fuel gauge was found in the vendor
 * Eagle schematic (net "BAT" is a bare JST connector with no
 * resistive divider wired to any GPIO) or in any of the vendor's
 * Arduino examples -- see HARDWARE.md.
 *
 * This board has been added without on-hardware testing. Pin
 * assignments come from the vendor's Arduino examples and Eagle
 * schematic (github.com/Elecrow-RD/CrowPanel-ESP32-5.79-E-paper-HMI-
 * Display-with-272-792) cross-checked against the community ESPHome
 * driver referenced above.
 *
 * Included by main/app_config.h when
 * CONFIG_DRAFTLING_MODEL_ELECROW_CROWPANEL_579 is selected.
 */

#define BOARD_NAME      "Elecrow CrowPanel ESP32-S3 5.79in E-Paper"

/* SD card on its own 4-wire SPI bus, separate from the EPD panel's
 * SPI bus. Power-enable line is active HIGH (IO42_TF_3.3_CTL on the
 * vendor schematic; driven in main.cpp before sd_card_init_spi(),
 * matching the vendor's 5.79_TF Arduino example). */
#define SD_SPI_MOSI_PIN     40
#define SD_SPI_MISO_PIN     13
#define SD_SPI_SCK_PIN      39
#define SD_SPI_CS_PIN       10
#define SD_EN_PIN           -1
#define SD_POWER_EN_PIN     42

/* No on-board battery monitor (see the file-header comment above). */
#define BATT_ADC_PIN        -1
#define BATT_EN_PIN         -1
#define BATT_DIVIDER        1

/* Menu button: deep-sleep wake source (RTC-capable) and, while
 * awake, the F1 / forget-keyboards key -- see
 * crowpanel_menu_key_init() in main.cpp. */
#define WAKEUP_GPIO_NUM     2

/* Back button and 3-way dial switch (Up/Down/OK), all active-low
 * with internal pull-ups -- see crowpanel_nav_init() in main.cpp. */
#define BTN_BACK_PIN        1
#define BTN_DIAL_UP_PIN     6
#define BTN_DIAL_DOWN_PIN   4
#define BTN_DIAL_OK_PIN     5
