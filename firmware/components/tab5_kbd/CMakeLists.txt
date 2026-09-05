## M5Stack Tab5 attachable keyboard host (I2C + GPIO interrupt).
##
## Used only on the M5Stack Tab5 (ESP32-P4), where the detachable
## keyboard talks over its own dedicated I2C bus. The component has no
## exotic dependencies, so it registers on every target; main / editor
## pull it in via idf_component_optional_requires() under
## CONFIG_DRAFTLING_HAS_TAB5_KBD.
##
## The public header exposes driver/i2c_master.h (i2c_master_bus_handle_t)
## and ble_keyboard.h (kb_event_t), so both are public REQUIRES.
idf_component_register(
    SRCS "tab5_kbd.cpp"
    INCLUDE_DIRS "include"
    REQUIRES ble_keyboard esp_driver_i2c
    PRIV_REQUIRES esp_driver_gpio esp_timer
)
