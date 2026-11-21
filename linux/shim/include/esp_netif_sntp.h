#pragma once
#include <stdint.h>
#include "esp_err.h"
/* Linux: the operating system keeps the clock; the Git client's SNTP
 * query is answered "no time server" and it uses the system time. */
typedef struct { const char *server; } esp_sntp_config_t;
#define ESP_NETIF_SNTP_DEFAULT_CONFIG(s) { s }
static inline esp_err_t esp_netif_sntp_init(const esp_sntp_config_t *c) { (void)c; return ESP_FAIL; }
static inline esp_err_t esp_netif_sntp_sync_wait(uint32_t t) { (void)t; return ESP_FAIL; }
static inline void esp_netif_sntp_deinit(void) { }
