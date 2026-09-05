idf_component_register(
    SRCS "editor.cpp" "md_parser.cpp" "editor_ui.cpp" "wifi_icon.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES display sd_card ble_keyboard kb_layout wifi_manager git_sync standby battery lvgl__lvgl fonts nvs_flash esp_app_format
)

# Kconfig-conditional REQUIRES must use idf_component_optional_requires()
# after register: CONFIG_* is empty during the early-expansion pass that
# builds the requires graph.
if(CONFIG_DRAFTLING_HAS_USB_HOST)
    idf_component_optional_requires(PRIVATE usb_kbd)
endif()

if(CONFIG_DRAFTLING_HAS_TAB5_KBD)
    idf_component_optional_requires(PRIVATE tab5_kbd)
endif()
