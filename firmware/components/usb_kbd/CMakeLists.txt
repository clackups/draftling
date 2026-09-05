## usb_kbd is only relevant on the M5Stack Tab5 (ESP32-P4): it is
## the only supported board with a USB-A host receptacle, and the
## espressif/usb_host_hid managed component this file builds against
## is itself gated to `target == esp32p4` in idf_component.yml. On
## every other target the `usb/hid_host.h` header therefore does not
## exist and compiling usb_kbd.cpp fails. Register an empty component
## on non-P4 targets so any `idf_component_optional_requires(usb_kbd)`
## from main / editor stays a no-op.
idf_build_get_property(target IDF_TARGET)
if(target STREQUAL "esp32p4")
    idf_component_register(
        SRCS "usb_kbd.cpp"
        INCLUDE_DIRS "include"
        PRIV_REQUIRES ble_keyboard usb driver
    )
else()
    idf_component_register()
endif()
