#pragma once
/* Linux: git_util.c picks its SHA-1 API by the ESP-IDF version; 5
 * selects the mbedtls_sha1_* calls, which mbedtls/sha1.h maps onto
 * OpenSSL. */
#define ESP_IDF_VERSION_MAJOR 5
#define ESP_IDF_VERSION_MINOR 0
