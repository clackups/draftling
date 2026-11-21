/*
 * Online demo: WiFi. Nothing goes on the network.
 *
 * A scan finds a few made-up networks. Connecting takes a moment and
 * then succeeds -- on a secured network only with a password of at
 * least 8 characters, so the "wrong password" path can be seen too.
 * Credentials are kept like on the device: /sdcard/wifi.cfg (SSID on
 * the first line, password on the second) wins over the copy in NVS.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "lvgl.h"
#include "nvs.h"
#include "sd_card.h"
#include "wifi_manager.h"

static const char *TAG = "WiFiDemo";

#define WIFI_CFG_PATH "/sdcard/wifi.cfg"
#define CONNECT_MS 1500

typedef struct {
    const char *ssid;
    int8_t rssi;
    bool open;
} demo_net_t;

static const demo_net_t s_nets[] = {
    { "Draftling Cafe", -48, true },
    { "HomeNet",        -55, false },
    { "Library-Guest",  -67, true },
    { "Office 5G",      -74, false },
    { "Neighbour",      -86, false },
};

static wifi_state_t s_state = WIFI_STATE_IDLE;
static wifi_state_callback_t s_callback;
static char s_ssid[33];
static char s_ip[16];
static bool s_auth_failed;
static bool s_ipv6;
static lv_timer_t *s_pending;
static bool s_pending_ok;

static void set_state(wifi_state_t st)
{
    s_state = st;
    if (s_callback) s_callback(st);
}

static const demo_net_t *find_net(const char *ssid)
{
    for (size_t i = 0; i < sizeof(s_nets) / sizeof(s_nets[0]); i++) {
        if (strcmp(s_nets[i].ssid, ssid) == 0) return &s_nets[i];
    }
    return NULL;
}

static bool load_from_file(char *ssid, size_t ssid_sz, char *pass, size_t pass_sz)
{
    char *buf = NULL;
    size_t len = 0;
    if (sd_card_read_file(WIFI_CFG_PATH, &buf, &len) != ESP_OK) return false;
    char *nl = strchr(buf, '\n');
    if (!nl) { free(buf); return false; }
    *nl = '\0';
    char *cr = strchr(buf, '\r');
    if (cr) *cr = '\0';
    snprintf(ssid, ssid_sz, "%s", buf);
    char *p2 = nl + 1;
    p2[strcspn(p2, "\r\n")] = '\0';
    snprintf(pass, pass_sz, "%s", p2);
    free(buf);
    return ssid[0] != '\0';
}

static bool load_from_nvs(char *ssid, size_t ssid_sz, char *pass, size_t pass_sz)
{
    nvs_handle_t h;
    if (nvs_open("wifi", NVS_READONLY, &h) != ESP_OK) return false;
    bool ok = nvs_get_str(h, "ssid", ssid, &ssid_sz) == ESP_OK &&
              nvs_get_str(h, "pass", pass, &pass_sz) == ESP_OK;
    nvs_close(h);
    return ok && ssid[0];
}

static void save_to_nvs(const char *ssid, const char *pass)
{
    nvs_handle_t h;
    if (nvs_open("wifi", NVS_READWRITE, &h) != ESP_OK) return;
    nvs_set_str(h, "ssid", ssid);
    nvs_set_str(h, "pass", pass);
    nvs_commit(h);
    nvs_close(h);
}

static bool load_configured(char *ssid, size_t ssid_sz, char *pass, size_t pass_sz)
{
    if (load_from_file(ssid, ssid_sz, pass, pass_sz)) return true;
    return load_from_nvs(ssid, ssid_sz, pass, pass_sz);
}

static void connect_done_cb(lv_timer_t *t)
{
    lv_timer_delete(t);
    s_pending = NULL;
    if (s_pending_ok) {
        snprintf(s_ip, sizeof(s_ip), "192.168.1.%d", 20 + (int)(strlen(s_ssid) * 7 % 200));
        s_ipv6 = true;
        ESP_LOGI(TAG, "Connected to %s (simulated)", s_ssid);
        set_state(WIFI_STATE_CONNECTED);
    } else {
        s_ssid[0] = '\0';
        set_state(WIFI_STATE_ERROR);
    }
}

esp_err_t wifi_manager_init(void)
{
    return ESP_OK;
}

esp_err_t wifi_manager_connect(void)
{
    char ssid[33] = "", pass[65] = "";
    if (!load_configured(ssid, sizeof(ssid), pass, sizeof(pass))) {
        ESP_LOGE(TAG, "No WiFi credentials found");
        return ESP_ERR_NOT_FOUND;
    }
    return wifi_manager_connect_to(ssid, pass, false);
}

/* The outcome is known at once; the state changes follow over the next
 * moment, as they do while the device associates. */
esp_err_t wifi_manager_connect_to(const char *ssid, const char *password, bool save)
{
    if (s_pending) return ESP_ERR_INVALID_STATE;
    const demo_net_t *net = find_net(ssid);
    bool secured = net ? !net->open : (password && password[0]);
    s_auth_failed = secured && (!password || strlen(password) < 8);
    s_pending_ok = !s_auth_failed;
    s_ip[0] = '\0';
    s_ipv6 = false;
    snprintf(s_ssid, sizeof(s_ssid), "%s", ssid);
    set_state(WIFI_STATE_CONNECTING);
    s_pending = lv_timer_create(connect_done_cb, CONNECT_MS, NULL);
    if (s_pending_ok && save) save_to_nvs(ssid, password ? password : "");
    return s_pending_ok ? ESP_OK : ESP_FAIL;
}

esp_err_t wifi_manager_disconnect(void)
{
    if (s_pending) {
        lv_timer_delete(s_pending);
        s_pending = NULL;
    }
    s_ip[0] = '\0';
    s_ipv6 = false;
    set_state(WIFI_STATE_DISCONNECTED);
    return ESP_OK;
}

wifi_state_t wifi_manager_get_state(void) { return s_state; }
bool wifi_manager_is_connected(void) { return s_state == WIFI_STATE_CONNECTED; }
bool wifi_manager_network_up(void) { return wifi_manager_is_connected(); }
void wifi_manager_set_callback(wifi_state_callback_t cb) { s_callback = cb; }
const char *wifi_manager_get_ip(void) { return s_ip; }
const char *wifi_manager_get_ssid(void) { return s_ssid; }
bool wifi_manager_last_failure_was_auth(void) { return s_auth_failed; }
bool wifi_manager_has_global_ipv6(void) { return s_ipv6; }

esp_err_t wifi_manager_scan(wifi_scan_result_t *results, int max_results, int *out_count)
{
    if (s_pending) return ESP_ERR_INVALID_STATE;
    int n = 0;
    for (size_t i = 0; i < sizeof(s_nets) / sizeof(s_nets[0]) && n < max_results; i++) {
        snprintf(results[n].ssid, sizeof(results[n].ssid), "%s", s_nets[i].ssid);
        results[n].rssi = s_nets[i].rssi;
        results[n].open = s_nets[i].open;
        results[n].known = false;
        n++;
    }
    *out_count = n;
    return ESP_OK;
}

esp_err_t wifi_manager_save_to_file(const char *ssid, const char *password)
{
    if (!ssid) return ESP_ERR_INVALID_ARG;
    char buf[33 + 1 + 64 + 1 + 1];
    int len = snprintf(buf, sizeof(buf), "%s\n%s\n", ssid, password ? password : "");
    if (len < 0 || (size_t)len >= sizeof(buf)) return ESP_ERR_INVALID_SIZE;
    return sd_card_write_file(WIFI_CFG_PATH, buf, (size_t)len);
}

bool wifi_manager_get_configured_ssid(char *ssid, size_t ssid_sz)
{
    char p[65];
    if (!ssid || ssid_sz == 0) return false;
    bool ok = load_configured(ssid, ssid_sz, p, sizeof(p));
    if (!ok) ssid[0] = '\0';
    return ok;
}

bool wifi_manager_get_configured(char *ssid, size_t ssid_sz, char *pass, size_t pass_sz)
{
    bool ok = load_configured(ssid, ssid_sz, pass, pass_sz);
    if (!ok) {
        if (ssid_sz) ssid[0] = '\0';
        if (pass_sz) pass[0] = '\0';
    }
    return ok;
}

esp_err_t wifi_manager_forget(void)
{
    wifi_manager_disconnect();
    nvs_handle_t h;
    if (nvs_open("wifi", NVS_READWRITE, &h) == ESP_OK) {
        nvs_erase_all(h);
        nvs_commit(h);
        nvs_close(h);
    }
    return ESP_OK;
}
