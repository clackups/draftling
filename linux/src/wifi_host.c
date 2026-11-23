/*
 * Linux, CONFIG_DRAFTLING_NETWORK_HOST_MANAGED (the Flatpak): the
 * network connection belongs to the host, and Draftling does not look
 * at it or change it. wifi_manager_network_up() is always true, so Git
 * sync and Draftbox just try the network and report a failure the way
 * they report any other; there is no WiFi device, nothing to scan,
 * connect or forget, and no WiFi icon (wifi_manager_is_connected() is
 * false). The editor leaves out the WiFi menu, Ctrl+W and the WiFi
 * step of "Format SD card" in this build. Used in place of
 * wifi_linux.c, which needs nmcli and the host's network state.
 */
#include <stdio.h>

#include "wifi_manager.h"

esp_err_t wifi_manager_init(void) { return ESP_OK; }
esp_err_t wifi_manager_connect(void) { return ESP_ERR_NOT_SUPPORTED; }

esp_err_t wifi_manager_connect_to(const char *ssid, const char *password, bool save)
{
    (void)ssid; (void)password; (void)save;
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t wifi_manager_disconnect(void) { return ESP_ERR_NOT_SUPPORTED; }
wifi_state_t wifi_manager_get_state(void) { return WIFI_STATE_IDLE; }
bool wifi_manager_is_connected(void) { return false; }
bool wifi_manager_network_up(void) { return true; }
void wifi_manager_set_callback(wifi_state_callback_t cb) { (void)cb; }
const char *wifi_manager_get_ip(void) { return ""; }
const char *wifi_manager_get_ssid(void) { return ""; }

esp_err_t wifi_manager_scan(wifi_scan_result_t *results, int max_results, int *out_count)
{
    (void)results; (void)max_results;
    if (out_count) *out_count = 0;
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t wifi_manager_save_to_file(const char *ssid, const char *password)
{
    (void)ssid; (void)password;
    return ESP_ERR_NOT_SUPPORTED;
}

bool wifi_manager_get_configured_ssid(char *ssid, size_t ssid_sz)
{
    if (ssid && ssid_sz) ssid[0] = '\0';
    return false;
}

bool wifi_manager_get_configured(char *ssid, size_t ssid_sz, char *pass, size_t pass_sz)
{
    if (ssid && ssid_sz) ssid[0] = '\0';
    if (pass && pass_sz) pass[0] = '\0';
    return false;
}

esp_err_t wifi_manager_forget(void) { return ESP_OK; }
bool wifi_manager_last_failure_was_auth(void) { return false; }
bool wifi_manager_has_global_ipv6(void) { return false; }
