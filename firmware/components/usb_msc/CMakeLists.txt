## usb_msc.cpp is always compiled -- it is only functional on the
## boards listed under CONFIG_DRAFTLING_HAS_USB_MSC in Kconfig.projbuild
## (those whose USB port wires the ESP32-S3's native USB-OTG
## controller straight to the connector); on every other board it
## compiles to no-op stubs via its own
## `#if defined(CONFIG_DRAFTLING_HAS_USB_MSC)` guard, matching
## components/ble_keyboard.cpp's approach.
##
## This -- rather than the components/usb_kbd style of conditionally
## registering an empty component -- is needed because usb_kbd's
## condition is `target STREQUAL "esp32p4"`, a CMake property known
## during ESP-IDF's early, Kconfig-blind REQUIRES-extraction pass;
## ours needs CONFIG_DRAFTLING_HAS_USB_MSC, which reads empty in that
## early pass (see the long comment in
## components/ble_keyboard/CMakeLists.txt). Making SRCS/PRIV_REQUIRES
## conditional on it here would silently produce a component with no
## requires at all, breaking the `#include`s in usb_msc.cpp even
## though the file itself still gets compiled in the later,
## Kconfig-aware pass.
idf_component_register(
    SRCS "usb_msc.cpp"
    INCLUDE_DIRS "include"
    ## esp_hal_usb provides hal/usb_serial_jtag_hal.h, used by
    ## teardown_locked() (see its comment) to hand the internal FSLS
    ## PHY back to USB-Serial-JTAG when SD-via-USB is switched off.
    PRIV_REQUIRES sd_card esp_timer esp_hal_usb
)

if(CONFIG_DRAFTLING_HAS_USB_MSC)
    ## espressif/esp_tinyusb is declared in this component's own
    ## idf_component.yml (mirroring components/usb_kbd's
    ## espressif/usb_host_hid dependency), so -- unlike a dependency
    ## declared in main/idf_component.yml, which a sibling component
    ## must additionally pull in via idf_component_optional_requires()
    ## -- it is already automatically available to usb_msc.cpp without
    ## repeating it here. This idf_component_optional_requires() call
    ## is (unlike PRIV_REQUIRES above) safe to gate on Kconfig: it
    ## runs after idf_component_register(), in the later pass.
    idf_component_optional_requires(PRIVATE espressif__esp_tinyusb)
endif()
