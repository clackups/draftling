idf_component_register(
    SRCS "io_expander_ch422g.cpp" "io_expander_m5ioe1.cpp" "io_expander_pca9557.cpp"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_driver_i2c
)
