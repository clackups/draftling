#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/semphr.h>
#include <esp_log.h>
#include <esp_heap_caps.h>
#include <esp_wifi.h>
#include <esp_netif.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include <nvs.h>

#include "wifi_manager.h"
#include "sd_card.h"

static const char *TAG = "WiFiMgr";

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define MAX_RETRY          5

static EventGroupHandle_t s_event_group = NULL;
static wifi_state_t       s_state = WIFI_STATE_IDLE;
static wifi_state_callback_t s_callback = NULL;
static int s_retry_count = 0;
static char s_ip_str[20] = "";
static char s_ssid[33]   = "";
static bool s_initialized = false;
static esp_netif_t *s_sta_netif = NULL;
static bool s_ipv6_global = false;
static char s_ip6_str[48] = "";
/* True while a connection attempt (or an established connection)
 * wants WIFI_EVENT_STA_DISCONNECTED to trigger a reconnect. Cleared
 * before every deliberate disconnect / stop and once the retry budget
 * is spent, so a failed or abandoned attempt cannot keep the driver
 * busy retrying in the background -- that used to leave the STA
 * "connecting" forever and make every later Ctrl+W fail until reboot. */
static volatile bool s_auto_retry = false;
/* Serializes wifi_manager_connect_to() and wifi_manager_scan(): the
 * driver rejects set_config / scan_start while a connect is in flight. */
static SemaphoreHandle_t s_op_mutex = NULL;
/* True when the last connection attempt was abandoned because the AP
 * rejected our credentials -- see is_auth_failure(). */
static volatile bool s_auth_failed = false;

/* Disconnect reasons that mean the credentials (or security mode) are
 * wrong, so retrying the same password cannot succeed. A wrong WPA2
 * PSK normally surfaces as a 4-way handshake timeout (15 / 204), not
 * AUTH_FAIL, which is what WPA3-SAE reports. Everything else (beacon
 * timeout, AP not found, association failures from a weak signal)
 * stays retryable. */
static bool is_auth_failure(uint8_t reason)
{
    switch (reason) {
    case WIFI_REASON_AUTH_FAIL:
    case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
    case WIFI_REASON_HANDSHAKE_TIMEOUT:
    case WIFI_REASON_MIC_FAILURE:
    case WIFI_REASON_802_1X_AUTH_FAILED:
    case WIFI_REASON_NO_AP_FOUND_W_COMPATIBLE_SECURITY:
    case WIFI_REASON_NO_AP_FOUND_IN_AUTHMODE_THRESHOLD:
        return true;
    default:
        return false;
    }
}

static void set_state(wifi_state_t st)
{
    s_state = st;
    if (s_callback) s_callback(st);
}

static void wifi_event_handler(void *arg, esp_event_base_t base,
                               int32_t id, void *data)
{
    if (base == WIFI_EVENT) {
        /* No esp_wifi_connect() on WIFI_EVENT_STA_START: that event
         * only fires when the driver goes from stopped to started, so
         * a connect attempt made while it was already running (after a
         * scan, or after a previous failed attempt) never connected.
         * wifi_manager_connect_to() calls esp_wifi_connect() itself. */
        if (id == WIFI_EVENT_STA_CONNECTED) {
            /* Bring up the link-local IPv6 address on this netif. That
             * in turn makes lwIP send router solicitations, so if the
             * AP's network advertises a global prefix (SLAAC, RFC
             * 4862) we will receive a global address via a later
             * IP_EVENT_GOT_IP6, handled below. */
            if (s_sta_netif) esp_netif_create_ip6_linklocal(s_sta_netif);
        } else if (id == WIFI_EVENT_STA_DISCONNECTED) {
            wifi_event_sta_disconnected_t *ev = (wifi_event_sta_disconnected_t *)data;
            s_ipv6_global = false;
            s_ip6_str[0] = '\0';
            /* ASSOC_LEAVE is our own esp_wifi_disconnect() (e.g. the
             * one wifi_manager_connect_to() issues before switching
             * networks) -- never retry that. */
            if (!s_auto_retry || (ev && ev->reason == WIFI_REASON_ASSOC_LEAVE)) {
                /* nothing */
            } else if (ev && is_auth_failure(ev->reason)) {
                ESP_LOGW(TAG, "Authentication failed (reason %d), not retrying", ev->reason);
                s_auto_retry = false;
                s_auth_failed = true;
                xEventGroupSetBits(s_event_group, WIFI_FAIL_BIT);
                set_state(WIFI_STATE_ERROR);
            } else if (s_retry_count < MAX_RETRY) {
                esp_wifi_connect();
                s_retry_count++;
                ESP_LOGI(TAG, "Retry %d/%d", s_retry_count, MAX_RETRY);
            } else {
                s_auto_retry = false;
                xEventGroupSetBits(s_event_group, WIFI_FAIL_BIT);
                set_state(WIFI_STATE_ERROR);
            }
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)data;
        snprintf(s_ip_str, sizeof(s_ip_str), IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "Connected, IP: %s", s_ip_str);
        s_retry_count = 0;
        xEventGroupSetBits(s_event_group, WIFI_CONNECTED_BIT);
        set_state(WIFI_STATE_CONNECTED);
    } else if (base == IP_EVENT && id == IP_EVENT_GOT_IP6) {
        ip_event_got_ip6_t *event = (ip_event_got_ip6_t *)data;
        esp_ip6_addr_type_t type = esp_netif_ip6_get_addr_type(&event->ip6_info.ip);
        ESP_LOGI(TAG, "Got IPv6 address " IPV6STR " (type %d)",
                 IPV62STR(event->ip6_info.ip), type);
        if (type == ESP_IP6_ADDR_IS_GLOBAL) {
            s_ipv6_global = true;
            snprintf(s_ip6_str, sizeof(s_ip6_str), IPV6STR, IPV62STR(event->ip6_info.ip));
            /* Re-fire the state callback so the UI refreshes the WiFi
             * icon (and anything else derived from dual-stack status)
             * even though wifi_state_t itself did not change. */
            set_state(s_state);
        }
    }
}

/* Load credentials from NVS */
static bool load_from_nvs(char *ssid, size_t ssid_sz, char *pass, size_t pass_sz)
{
    nvs_handle_t h;
    if (nvs_open("wifi", NVS_READONLY, &h) != ESP_OK) return false;
    bool ok = (nvs_get_str(h, "ssid", ssid, &ssid_sz) == ESP_OK &&
               nvs_get_str(h, "pass", pass, &pass_sz) == ESP_OK);
    nvs_close(h);
    return ok;
}

/* Save credentials to NVS */
static void save_to_nvs(const char *ssid, const char *pass)
{
    nvs_handle_t h;
    if (nvs_open("wifi", NVS_READWRITE, &h) != ESP_OK) return;
    nvs_set_str(h, "ssid", ssid);
    nvs_set_str(h, "pass", pass);
    nvs_commit(h);
    nvs_close(h);
}

/* Load credentials from /sdcard/wifi.cfg */
static bool load_from_file(char *ssid, size_t ssid_sz, char *pass, size_t pass_sz)
{
    char *buf = NULL;
    size_t len = 0;
    if (sd_card_read_file("/sdcard/wifi.cfg", &buf, &len) != ESP_OK) return false;

    /* First line = SSID, second line = password */
    char *nl = strchr(buf, '\n');
    if (!nl) { free(buf); return false; }

    *nl = '\0';
    /* Strip trailing \r */
    char *cr = strchr(buf, '\r');
    if (cr) *cr = '\0';
    strncpy(ssid, buf, ssid_sz - 1);
    ssid[ssid_sz - 1] = '\0';

    char *p2 = nl + 1;
    cr = strchr(p2, '\r');
    if (cr) *cr = '\0';
    nl = strchr(p2, '\n');
    if (nl) *nl = '\0';
    strncpy(pass, p2, pass_sz - 1);
    pass[pass_sz - 1] = '\0';

    free(buf);
    return ssid[0] != '\0';
}

extern "C" esp_err_t wifi_manager_init(void)
{
    /* Guard against concurrent first-time initialization from multiple
     * tasks now that this is also called lazily from wifi_manager_connect_to(). */
    static SemaphoreHandle_t init_mutex = NULL;
    static portMUX_TYPE init_spinlock = portMUX_INITIALIZER_UNLOCKED;
    portENTER_CRITICAL(&init_spinlock);
    if (init_mutex == NULL) init_mutex = xSemaphoreCreateMutex();
    portEXIT_CRITICAL(&init_spinlock);
    if (init_mutex == NULL) return ESP_ERR_NO_MEM;
    xSemaphoreTake(init_mutex, portMAX_DELAY);

    if (s_initialized) {
        xSemaphoreGive(init_mutex);
        return ESP_OK;
    }

    s_event_group = xEventGroupCreate();
    if (s_op_mutex == NULL) s_op_mutex = xSemaphoreCreateMutex();
    if (s_event_group == NULL || s_op_mutex == NULL) {
        xSemaphoreGive(init_mutex);
        return ESP_ERR_NO_MEM;
    }

    esp_err_t err = esp_netif_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_netif_init failed: %s", esp_err_to_name(err));
        xSemaphoreGive(init_mutex);
        return err;
    }
    /* esp_event_loop_create_default may already be called */
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "esp_event_loop_create_default failed: %s", esp_err_to_name(err));
        xSemaphoreGive(init_mutex);
        return err;
    }
    /* esp_netif_create_default_wifi_sta() asserts (not returns an
     * error) if the "WIFI_STA_DEF" netif already exists. That can
     * happen if a previous init attempt got past this point and then
     * failed later (esp_wifi_init NO_MEM on heap-tight boards is the
     * realistic case): the netif persists but s_initialized stays
     * false, so the next call into wifi_manager_init aborts the
     * firmware. Reuse the existing handle when present. */
    s_sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (s_sta_netif == NULL) {
        s_sta_netif = esp_netif_create_default_wifi_sta();
    } else {
        ESP_LOGW(TAG, "WIFI_STA_DEF netif already exists, reusing");
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    /* Permanent budget guard: WiFi static buffers come from internal
     * DRAM and cannot move to PSRAM, so esp_wifi_init() is the canary
     * for internal-heap exhaustion on PSRAM-heavy boards (e.g. M5Stack
     * PaperS3 with epdiy framebuffers + Bluedroid + LVGL). Keep this as
     * a single line so it is cheap enough to run on every connect
     * attempt; if INTERNAL free or largest_free_block drop below
     * ~32 KB esp_wifi_init() is likely to fail with ESP_ERR_NO_MEM. */
    ESP_LOGI(TAG, "Heap before esp_wifi_init: INTERNAL free=%u largest=%u, DMA free=%u largest=%u",
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_DMA),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_DMA));
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_init failed: %s", esp_err_to_name(err));
        xSemaphoreGive(init_mutex);
        return err;
    }

    esp_event_handler_instance_t inst_any, inst_ip, inst_ip6;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &inst_any));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &inst_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_GOT_IP6, &wifi_event_handler, NULL, &inst_ip6));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    s_initialized = true;
    xSemaphoreGive(init_mutex);
    ESP_LOGI(TAG, "WiFi manager initialized");
    return ESP_OK;
}

extern "C" esp_err_t wifi_manager_deinit(void)
{
    s_auto_retry = false;
    esp_wifi_stop();
    esp_wifi_deinit();
    s_initialized = false;
    set_state(WIFI_STATE_IDLE);
    return ESP_OK;
}

/* Resolve the configured credentials: /sdcard/wifi.cfg if present,
 * else NVS. When the file differs from NVS and `sync_nvs` is set, NVS
 * is updated to match the file. */
static bool load_configured(char *ssid, size_t ssid_sz, char *pass, size_t pass_sz, bool sync_nvs)
{
    char file_ssid[33] = "", file_pass[65] = "";

    /* Always check /sdcard/wifi.cfg, not just when NVS is empty --
     * otherwise editing the file after the first successful connect
     * had no effect: NVS was consulted first and, once populated,
     * the file was never looked at again, so the device kept
     * reconnecting to whichever SSID it first learned. If the file
     * now names different credentials (new SSID, or just a changed
     * password for the same network), forget the stale NVS entry and
     * use what is on the card. */
    bool have_file = load_from_file(file_ssid, sizeof(file_ssid), file_pass, sizeof(file_pass));
    bool have_nvs  = load_from_nvs(ssid, ssid_sz, pass, pass_sz);

    if (have_file && (!have_nvs || strcmp(ssid, file_ssid) != 0 || strcmp(pass, file_pass) != 0)) {
        strncpy(ssid, file_ssid, ssid_sz - 1); ssid[ssid_sz - 1] = '\0';
        strncpy(pass, file_pass, pass_sz - 1); pass[pass_sz - 1] = '\0';
        if (sync_nvs) save_to_nvs(ssid, pass);
        return true;
    }
    return have_nvs;
}

extern "C" esp_err_t wifi_manager_connect(void)
{
    char ssid[33] = "", pass[65] = "";
    if (!load_configured(ssid, sizeof(ssid), pass, sizeof(pass), true)) {
        ESP_LOGE(TAG, "No WiFi credentials found");
        return ESP_ERR_NOT_FOUND;
    }
    return wifi_manager_connect_to(ssid, pass, false);
}

extern "C" bool wifi_manager_get_configured_ssid(char *ssid, size_t ssid_sz)
{
    if (!ssid || ssid_sz == 0) return false;
    char s[33] = "", p[65] = "";
    bool ok = load_configured(s, sizeof(s), p, sizeof(p), false);
    strncpy(ssid, ok ? s : "", ssid_sz - 1);
    ssid[ssid_sz - 1] = '\0';
    return ok;
}

extern "C" esp_err_t wifi_manager_connect_to(const char *ssid, const char *password, bool save)
{
    /* Lazy-init: WiFi is only brought up when the user requests a
     * connection.  This avoids permanently reserving WiFi's internal-RAM
     * static buffers at boot, which on memory-constrained boards (e.g.
     * M5Stack PaperS3) causes esp_wifi_init() to fail once Bluedroid
     * is also running. */
    esp_err_t init_err = wifi_manager_init();
    if (init_err != ESP_OK) return init_err;

    if (xSemaphoreTake(s_op_mutex, 0) != pdTRUE) {
        ESP_LOGW(TAG, "Connect requested while another WiFi operation is running");
        return ESP_ERR_INVALID_STATE;
    }

    /* Drop any current association or leftover attempt first (the
     * driver refuses esp_wifi_set_config() while still connecting).
     * Errors are expected here when the driver is stopped / idle. */
    s_auto_retry = false;
    esp_wifi_disconnect();

    wifi_config_t wifi_cfg = {};
    strncpy((char *)wifi_cfg.sta.ssid, ssid, sizeof(wifi_cfg.sta.ssid) - 1);
    strncpy((char *)wifi_cfg.sta.password, password, sizeof(wifi_cfg.sta.password) - 1);
    strncpy(s_ssid, ssid, sizeof(s_ssid) - 1);
    s_ssid[sizeof(s_ssid) - 1] = '\0';

    s_retry_count = 0;
    s_auth_failed = false;
    s_ip_str[0] = '\0';
    s_ipv6_global = false;
    s_ip6_str[0] = '\0';
    /* A fail bit left over from an earlier attempt (e.g. one that hit
     * the 30 s timeout and then exhausted its retries) would otherwise
     * end this wait immediately. */
    xEventGroupClearBits(s_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
    set_state(WIFI_STATE_CONNECTING);

    ESP_LOGI(TAG, "WiFi: connecting to %s", ssid);

    esp_err_t err = esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg);
    if (err == ESP_OK) err = esp_wifi_start();
    if (err == ESP_OK) {
        s_auto_retry = true;
        err = esp_wifi_connect();
    }

    EventBits_t bits = 0;
    if (err == ESP_OK) {
        bits = xEventGroupWaitBits(s_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdTRUE, pdFALSE, pdMS_TO_TICKS(30000));
    } else {
        ESP_LOGE(TAG, "Starting connection failed: %s", esp_err_to_name(err));
    }

    if (bits & WIFI_CONNECTED_BIT) {
        if (save) save_to_nvs(ssid, password);
        xSemaphoreGive(s_op_mutex);
        return ESP_OK;
    }

    /* Leave the driver stopped so the next attempt starts clean and no
     * background retries linger. */
    ESP_LOGE(TAG, "Failed to connect to %s", ssid);
    s_auto_retry = false;
    esp_wifi_disconnect();
    esp_wifi_stop();
    /* The event handler has already reported ERROR when it ran out of
     * retries; report it here only for the timeout / start-error cases. */
    if (!(bits & WIFI_FAIL_BIT)) set_state(WIFI_STATE_ERROR);
    xSemaphoreGive(s_op_mutex);
    return ESP_FAIL;
}

extern "C" esp_err_t wifi_manager_disconnect(void)
{
    if (!s_initialized) {
        set_state(WIFI_STATE_DISCONNECTED);
        return ESP_OK;
    }
    s_auto_retry = false;
    esp_wifi_disconnect();
    esp_wifi_stop();
    s_ip_str[0] = '\0';
    s_ipv6_global = false;
    s_ip6_str[0] = '\0';
    set_state(WIFI_STATE_DISCONNECTED);
    return ESP_OK;
}

extern "C" esp_err_t wifi_manager_scan(wifi_scan_result_t *results, int max_results, int *out_count)
{
    if (!results || max_results <= 0 || !out_count) return ESP_ERR_INVALID_ARG;
    *out_count = 0;

    esp_err_t err = wifi_manager_init();
    if (err != ESP_OK) return err;

    if (xSemaphoreTake(s_op_mutex, 0) != pdTRUE) {
        ESP_LOGW(TAG, "Scan requested while another WiFi operation is running");
        return ESP_ERR_INVALID_STATE;
    }

    /* esp_wifi_scan_start() requires the STA interface to already be
     * started. Starting it no longer auto-connects (see
     * wifi_event_handler), so this is safe with a stale config. */
    wifi_ap_record_t *records = NULL;
    uint16_t got = 0;
    err = esp_wifi_start();
    if (err == ESP_OK) {
        wifi_scan_config_t scan_cfg = {};
        scan_cfg.show_hidden = false;
        err = esp_wifi_scan_start(&scan_cfg, true /* block until done */);
        if (err != ESP_OK) ESP_LOGE(TAG, "esp_wifi_scan_start failed: %s", esp_err_to_name(err));
    } else {
        ESP_LOGE(TAG, "esp_wifi_start (for scan) failed: %s", esp_err_to_name(err));
    }
    if (err == ESP_OK) {
        uint16_t ap_count = 0;
        esp_wifi_scan_get_ap_num(&ap_count);
        if (ap_count > 0) {
            records = (wifi_ap_record_t *)heap_caps_malloc(
                sizeof(wifi_ap_record_t) * ap_count, MALLOC_CAP_SPIRAM);
            if (!records) {
                records = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * ap_count);
            }
        }
        if (ap_count > 0 && !records) {
            err = ESP_ERR_NO_MEM;
        } else if (records) {
            got = ap_count;
            err = esp_wifi_scan_get_ap_records(&got, records);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "esp_wifi_scan_get_ap_records failed: %s", esp_err_to_name(err));
            }
        }
    }
    /* Don't keep the radio running after a scan unless we are online;
     * wifi_manager_connect_to() starts it again. Done after the records
     * were copied out, since stopping the driver discards them. */
    if (s_state != WIFI_STATE_CONNECTED) esp_wifi_stop();
    xSemaphoreGive(s_op_mutex);
    if (err != ESP_OK) {
        free(records);
        return err;
    }
    if (!records) return ESP_OK;

    int n = 0;
    for (uint16_t i = 0; i < got && n < max_results; i++) {
        const char *ssid = (const char *)records[i].ssid;
        if (ssid[0] == '\0') continue; /* hidden network -- skip */

        /* A single SSID is often broadcast by multiple BSSIDs (bands /
         * mesh nodes); de-duplicate and keep the strongest signal. */
        bool dup = false;
        for (int j = 0; j < n; j++) {
            if (strcmp(results[j].ssid, ssid) == 0) {
                dup = true;
                if (records[i].rssi > results[j].rssi) {
                    results[j].rssi = records[i].rssi;
                    results[j].open = (records[i].authmode == WIFI_AUTH_OPEN);
                }
                break;
            }
        }
        if (dup) continue;

        strncpy(results[n].ssid, ssid, sizeof(results[n].ssid) - 1);
        results[n].ssid[sizeof(results[n].ssid) - 1] = '\0';
        results[n].rssi = records[i].rssi;
        results[n].open = (records[i].authmode == WIFI_AUTH_OPEN);
        n++;
    }
    free(records);

    /* Strongest signal first; n is bounded by max_results so a plain
     * insertion sort is plenty. */
    for (int i = 1; i < n; i++) {
        wifi_scan_result_t tmp = results[i];
        int j = i - 1;
        while (j >= 0 && results[j].rssi < tmp.rssi) {
            results[j + 1] = results[j];
            j--;
        }
        results[j + 1] = tmp;
    }

    *out_count = n;
    return ESP_OK;
}

extern "C" esp_err_t wifi_manager_save_to_file(const char *ssid, const char *password)
{
    if (!ssid) return ESP_ERR_INVALID_ARG;
    char buf[33 + 1 + 64 + 1 + 1];
    int len = snprintf(buf, sizeof(buf), "%s\n%s\n", ssid, password ? password : "");
    if (len < 0 || (size_t)len >= sizeof(buf)) return ESP_ERR_INVALID_SIZE;
    return sd_card_write_file("/sdcard/wifi.cfg", buf, (size_t)len);
}

extern "C" wifi_state_t wifi_manager_get_state(void) { return s_state; }
extern "C" bool wifi_manager_last_failure_was_auth(void) { return s_auth_failed; }
extern "C" bool wifi_manager_is_connected(void) { return s_state == WIFI_STATE_CONNECTED; }
extern "C" void wifi_manager_set_callback(wifi_state_callback_t cb) { s_callback = cb; }
extern "C" const char *wifi_manager_get_ip(void) { return s_ip_str; }
extern "C" const char *wifi_manager_get_ssid(void) { return s_ssid; }
extern "C" bool wifi_manager_has_global_ipv6(void) { return s_ipv6_global; }
extern "C" const char *wifi_manager_get_ipv6(void) { return s_ip6_str; }
