idf_component_register(
    SRCS "battery.cpp"
    INCLUDE_DIRS "include"
    ## ESP-IDF 6.0 removed the legacy `driver` component's public
    ## dependencies on the split `esp_driver_*` components (see the
    ## 6.0 peripherals migration guide). battery.cpp includes
    ## `driver/gpio.h` and `driver/i2c_master.h`, so the matching
    ## `esp_driver_gpio` and `esp_driver_i2c` components must be
    ## listed explicitly. They also exist on IDF 5.3+, so the same
    ## list works on older IDFs.
    PRIV_REQUIRES esp_adc driver esp_driver_gpio esp_driver_i2c
)
