## usb_kbd is only built on boards with a USB host keyboard port
## (CONFIG_DRAFTLING_HAS_USB_HOST: M5Stack Tab5, and every ESP32-S3 board
## built with CONFIG_DRAFTLING_USB_HOST_NATIVE, e.g. the MicroJournal
## Rev 5 and Rev.7).
## Everywhere else it registers as an empty component, so the
## `idf_component_optional_requires(usb_kbd)` calls in main / editor
## (themselves gated on the same symbol) stay a no-op and neither this
## code nor espressif/usb_host_hid (whose manifest rule uses the same
## symbol) is compiled.
##
## CONFIG_* reads empty during ESP-IDF's early, Kconfig-blind
## requirements-expansion pass (see components/ble_keyboard/CMakeLists.txt),
## so the requirements are added with idf_component_optional_requires()
## after registration; the SRCS list is only consumed by the later,
## Kconfig-aware pass.
if(CONFIG_DRAFTLING_HAS_USB_HOST)
    idf_component_register(
        SRCS "usb_kbd.cpp"
        INCLUDE_DIRS "include"
    )
    idf_component_optional_requires(PRIVATE ble_keyboard usb driver
                                    espressif__usb_host_hid)
else()
    idf_component_register()
endif()
