#pragma once
#include "esp_err.h"
typedef struct { const char *s; } esp_sntp_config_t;
#define ESP_NETIF_SNTP_DEFAULT_CONFIG(x) { x }
static inline esp_err_t esp_netif_sntp_init(const esp_sntp_config_t *c) { (void)c; return ESP_FAIL; }
static inline esp_err_t esp_netif_sntp_sync_wait(uint32_t t) { (void)t; return ESP_FAIL; }
static inline void esp_netif_sntp_deinit(void) {}
