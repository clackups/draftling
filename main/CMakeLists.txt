## On ESP32-P4 builds we add `espressif/esp_hosted` (declared in
## main/idf_component.yml under `target == esp32p4`) so main.cpp can
## bring up the SDIO link to the on-board C6 before BLE / Wi-Fi
## init. We list it as a private requirement under a CONFIG guard so
## ESP32-S3 builds (which do not pull esp_hosted in at all) keep
## the same dependency set as before.
## ESP-IDF 6.0 removed the legacy `driver` component's public
## dependencies on the split `esp_driver_*` components (see the 6.0
## peripherals migration guide). main.cpp includes
## `driver/spi_common.h`, `driver/spi_master.h`, `driver/gpio.h`,
## `driver/i2c_master.h`, and `driver/uart.h` (now in esp_driver_spi /
## esp_driver_gpio / esp_driver_i2c / esp_driver_uart) and
## `driver/rtc_io.h` (still in the legacy `driver` component).
set(MAIN_PRIV_REQS display sd_card ble_keyboard editor wifi_manager git_sync standby battery touchscreen power nvs_flash bt esp_hid driver esp_driver_gpio esp_driver_spi esp_driver_i2c esp_driver_uart esp_event)
if(CONFIG_ESP_HOSTED_ENABLED)
    list(APPEND MAIN_PRIV_REQS espressif__esp_hosted)
endif()

idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    PRIV_REQUIRES ${MAIN_PRIV_REQS}
)

if(CONFIG_DRAFTLING_HAS_USB_HOST)
    idf_component_optional_requires(PRIVATE usb_kbd)
endif()
