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

#ifdef __cplusplus
}
#endif
