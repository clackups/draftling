#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <esp_err.h>
#include <stdbool.h>

typedef enum {
    WIFI_STATE_IDLE,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_DISCONNECTED,
    WIFI_STATE_ERROR,
} wifi_state_t;

typedef void (*wifi_state_callback_t)(wifi_state_t state);

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
