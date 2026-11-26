## Keyboard matrix wired to GPIO pins (MicroJournal Rev.6), see
## include/matrix_kbd.h. Built only with CONFIG_DRAFTLING_HAS_MATRIX_KBD;
## an empty component otherwise. main / editor / standby pull it in via
## idf_component_optional_requires() under the same flag, and its own
## dependencies are optional requirements too (CONFIG_ symbols are not
## known yet in ESP-IDF's early requirements pass, see usb_kbd).
##
## The public header includes ble_keyboard.h (kb_event_t).
if(CONFIG_DRAFTLING_HAS_MATRIX_KBD)
    idf_component_register(
        SRCS "matrix_kbd.cpp"
        INCLUDE_DIRS "include"
    )
    idf_component_optional_requires(PUBLIC ble_keyboard)
    idf_component_optional_requires(PRIVATE driver esp_driver_gpio esp_hw_support)
else()
    idf_component_register()
endif()
