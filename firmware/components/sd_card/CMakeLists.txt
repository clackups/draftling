idf_component_register(
    SRCS "sd_card.cpp"
    INCLUDE_DIRS "include"
    ## ESP-IDF 6.0 removed the legacy `driver` component's public
    ## dependencies on the split `esp_driver_*` components (see the
    ## 6.0 peripherals migration guide). sd_card.cpp includes
    ## `driver/sdmmc_host.h`, `driver/sdspi_host.h`,
    ## `driver/spi_common.h`, `driver/spi_master.h`, and
    ## `driver/gpio.h`, so the matching
    ## `esp_driver_*` components must be listed explicitly. They
    ## also exist on IDF 5.3+, so the same list works on older IDFs.
    PRIV_REQUIRES fatfs sdmmc vfs driver esp_driver_sdmmc esp_driver_sdspi esp_driver_spi esp_driver_gpio
)
