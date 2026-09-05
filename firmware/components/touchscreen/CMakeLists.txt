set(touch_reqs driver esp_driver_i2c esp_driver_gpio esp_timer lvgl__lvgl)

idf_component_register(
    SRCS "touchscreen.cpp"
    INCLUDE_DIRS "include"
    REQUIRES ${touch_reqs}
)

## On the M5Stack Tab5 (ESP32-P4) the touchscreen backend delegates
## to the espressif/m5stack_tab5 BSP's bsp_touch_new() and polls
## through the generic esp_lcd_touch API, so both v1 (GT911) and
## v2 (ST7123) variants work transparently. Pull in the BSP and
## the esp_lcd_touch core only on that target; non-Tab5 builds
## stay on the existing direct-I2C GT911 / AXS5106L paths and do
## not need either component.
##
## NOTE: this MUST use idf_component_optional_requires() rather than
## a plain `if(CONFIG_DRAFTLING_MODEL_M5STACK_TAB5) ... REQUIRES`
## block before idf_component_register(). ESP-IDF processes each
## CMakeLists.txt in two passes; during the early-expansion pass
## (which extracts REQUIRES to build the inter-component dependency
## graph) sdkconfig has NOT been parsed yet, so every CONFIG_*
## symbol is empty. A naive Kconfig-guarded list(APPEND ... REQUIRES)
## therefore silently drops espressif__m5stack_tab5 and
## espressif__esp_lcd_touch, and you get
##   managed_components/espressif__m5stack_tab5/include/bsp/touch.h:
##     fatal error: esp_lcd_touch.h: No such file or directory
## even though both managed components are installed. See the long
## NOTE block in components/ble_keyboard/CMakeLists.txt for the
## same gotcha and the supported workaround.
if(CONFIG_DRAFTLING_MODEL_M5STACK_TAB5)
    idf_component_optional_requires(PRIVATE
        espressif__m5stack_tab5
        espressif__esp_lcd_touch)
endif()
