idf_component_register(
    SRCS "display_rlcd.cpp" "display_epdiy.cpp" "display_h752.cpp" "epd_board_papers3.c" "display_axs15231b.cpp" "display_mipi_dsi.cpp" "lvgl_port.cpp"
    INCLUDE_DIRS "include"
    ## ESP-IDF 6.0 removed the legacy `driver` component's public
    ## dependencies on the split `esp_driver_*` components (see the
    ## 6.0 peripherals migration guide). The display backends
    ## directly include `driver/gpio.h`, `driver/spi_master.h`,
    ## `driver/i2c_master.h`, and `driver/ledc.h`, so the matching
    ## `esp_driver_*` components must be listed explicitly. They
    ## also exist on IDF 5.3+, so the same list works on older IDFs.
    PRIV_REQUIRES driver esp_driver_gpio esp_driver_spi esp_driver_i2c esp_driver_ledc esp_lcd esp_timer lvgl__lvgl fastepd
)

# Inject the LVGL custom-allocator hooks (lv_mem_init / lv_malloc_core /
# lv_free_core / lv_realloc_core / ...) directly into the lvgl__lvgl
# component library, so they end up in the SAME static archive that
# defines lv_mem.c.obj. With CONFIG_LV_USE_CUSTOM_MALLOC=y, lvgl's
# lv_mem.c references these symbols; if they live in a different
# archive (e.g. libdisplay.a) the cross-archive lookup is fragile and
# can fail with "undefined reference to lv_mem_init / lv_malloc_core /
# ..." even with --start-group, because the display archive may be
# processed before lvgl__lvgl is scanned for unresolved symbols.
# Putting the file inside the lvgl__lvgl library makes the link order
# question moot.
idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
target_sources(${lvgl_lib} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/lv_mem_psram.c")
target_link_libraries(${lvgl_lib} PRIVATE idf::esp_hw_support)
