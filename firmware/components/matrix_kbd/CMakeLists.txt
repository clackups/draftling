## Built-in key-matrix keyboard (rows / columns wired straight to GPIOs).
##
## Compiled only with CONFIG_DRAFTLING_HAS_MATRIX_KBD (the MicroJournal
## Rev.8); on every other board it registers as an empty component, and
## main / editor / standby pull it in through
## idf_component_optional_requires() under the same flag.
##
## CONFIG_* reads empty during ESP-IDF's early, Kconfig-blind
## requirements pass (see components/ble_keyboard/CMakeLists.txt), so
## the requirements are added with idf_component_optional_requires()
## after registration. The header includes ble_keyboard.h (kb_event_t).
if(CONFIG_DRAFTLING_HAS_MATRIX_KBD)
    idf_component_register(
        SRCS "matrix_kbd.cpp"
        INCLUDE_DIRS "include"
    )
    idf_component_optional_requires(PUBLIC ble_keyboard)
    idf_component_optional_requires(PRIVATE esp_driver_gpio driver
                                    esp_hw_support esp_rom log)
else()
    idf_component_register()
endif()
