#pragma once
#include "esp_err.h"
typedef enum { WIFI_PS_NONE, WIFI_PS_MIN_MODEM } wifi_ps_type_t;
typedef struct { int rssi; } wifi_ap_record_t;
static inline esp_err_t esp_wifi_get_ps(wifi_ps_type_t *p) { *p = WIFI_PS_NONE; return ESP_OK; }
static inline esp_err_t esp_wifi_set_ps(wifi_ps_type_t p) { (void)p; return ESP_OK; }
static inline esp_err_t esp_wifi_sta_get_ap_info(wifi_ap_record_t *a) { a->rssi = 0; return ESP_OK; }
