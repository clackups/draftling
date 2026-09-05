idf_component_register(
    SRCS "wifi_manager.cpp"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES esp_wifi esp_netif nvs_flash esp_event sd_card
)
