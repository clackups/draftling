## Draftling's native Git client.
##
## The core (git_*.c) implements a loose-object store, the smart Git
## HTTP protocol (gitprotocol-http), packfile v2 read/write and a
## three-way merge.  zlib is provided by the miniz implementation in the
## ESP32 ROM (esp_rom/include/miniz.h -- tinfl_* / tdefl_*), SHA-1 by
## Mbed TLS / PSA Crypto, and TLS + HTTP by esp_http_client / esp-tls.

idf_component_register(
    SRCS
        "git_sync.cpp"
        "git_util.c"
        "git_odb.c"
        "git_pack.c"
        "git_net.c"
        "git_merge.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "."
    PRIV_REQUIRES esp_http_client esp-tls mbedtls esp_rom esp_netif
                  sd_card wifi_manager nvs_flash
)
