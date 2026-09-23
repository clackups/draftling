#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <esp_err.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    WIFI_STATE_IDLE,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_DISCONNECTED,
    WIFI_STATE_ERROR,
} wifi_state_t;

typedef void (*wifi_state_callback_t)(wifi_state_t state);

/* One de-duplicated access point found by wifi_manager_scan(). Kept
 * deliberately narrow (no raw esp_wifi_types.h fields) since this is
 * the type UI code consumes. */
typedef struct {
    char ssid[33];
    int8_t rssi;
    bool open; /* true = no password required (WIFI_AUTH_OPEN) */
} wifi_scan_result_t;

#define WIFI_SCAN_MAX_RESULTS 24

esp_err_t wifi_manager_init(void);
esp_err_t wifi_manager_deinit(void);
esp_err_t wifi_manager_connect(void);
esp_err_t wifi_manager_connect_to(const char *ssid, const char *password, bool save);
esp_err_t wifi_manager_disconnect(void);
wifi_state_t wifi_manager_get_state(void);
bool wifi_manager_is_connected(void);
void wifi_manager_set_callback(wifi_state_callback_t callback);
const char *wifi_manager_get_ip(void);
const char *wifi_manager_get_ssid(void);

/* Blocking scan for visible access points (call from a background
 * task, not the LVGL task -- like wifi_manager_connect_to(), this can
 * take a few seconds). Results are de-duplicated by SSID (the
 * strongest RSSI of any BSSID sharing that SSID is kept), sorted
 * strongest-first, and hidden (blank-SSID) networks are skipped.
 * Writes up to max_results entries into `results` and the actual
 * count into *out_count. */
esp_err_t wifi_manager_scan(wifi_scan_result_t *results, int max_results, int *out_count);

/* Write ssid/password to /sdcard/wifi.cfg in the same two-line format
 * that wifi_manager_connect() reads back (SSID on line 1, password on
 * line 2), overwriting any existing file. */
esp_err_t wifi_manager_save_to_file(const char *ssid, const char *password);

/* True once the STA interface has a global-scope IPv6 address
 * (RA-advertised SLAAC prefix, not just the auto-assigned link-local
 * address every interface gets). Callers use this to decide whether
 * the network is IPv4/IPv6 dual-stack -- e.g. to pick the WiFi status
 * icon, or to prefer an AAAA lookup for an outgoing connection. */
bool wifi_manager_has_global_ipv6(void);
const char *wifi_manager_get_ipv6(void);

#ifdef __cplusplus
}
#endif
