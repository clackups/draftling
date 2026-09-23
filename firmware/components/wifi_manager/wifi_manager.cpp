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
/* True for the duration of wifi_manager_scan()'s esp_wifi_start()
 * call. WIFI_EVENT_STA_START unconditionally auto-connects (see
 * wifi_event_handler below); scanning also needs the STA interface
 * started but must NOT trigger a connection attempt using whatever
 * config happens to be set (stale, or none at all on a first-ever
 * scan), which would otherwise race the scan itself and can spuriously
 * flip the UI to "connection failed". */
static bool s_scan_in_progress = false;

static void set_state(wifi_state_t st)
{
    s_state = st;
    if (s_callback) s_callback(st);
}

static void wifi_event_handler(void *arg, esp_event_base_t base,
                               int32_t id, void *data)
{
    if (base == WIFI_EVENT) {
        if (id == WIFI_EVENT_STA_START) {
            if (!s_scan_in_progress) esp_wifi_connect();
        } else if (id == WIFI_EVENT_STA_CONNECTED) {
            /* Bring up the link-local IPv6 address on this netif. That
             * in turn makes lwIP send router solicitations, so if the
             * AP's network advertises a global prefix (SLAAC, RFC
             * 4862) we will receive a global address via a later
             * IP_EVENT_GOT_IP6, handled below. */
            if (s_sta_netif) esp_netif_create_ip6_linklocal(s_sta_netif);
        } else if (id == WIFI_EVENT_STA_DISCONNECTED) {
            s_ipv6_global = false;
            s_ip6_str[0] = '\0';
            if (s_retry_count < MAX_RETRY) {
                esp_wifi_connect();
                s_retry_count++;
                ESP_LOGI(TAG, "Retry %d/%d", s_retry_count, MAX_RETRY);
            } else {
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
    esp_wifi_stop();
    esp_wifi_deinit();
    s_initialized = false;
    set_state(WIFI_STATE_IDLE);
    return ESP_OK;
}

extern "C" esp_err_t wifi_manager_connect(void)
{
    char ssid[33] = "", pass[65] = "";
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
    bool have_nvs  = load_from_nvs(ssid, sizeof(ssid), pass, sizeof(pass));

    if (have_file && (!have_nvs || strcmp(ssid, file_ssid) != 0 || strcmp(pass, file_pass) != 0)) {
        strncpy(ssid, file_ssid, sizeof(ssid) - 1); ssid[sizeof(ssid) - 1] = '\0';
        strncpy(pass, file_pass, sizeof(pass) - 1); pass[sizeof(pass) - 1] = '\0';
        save_to_nvs(ssid, pass);
    } else if (!have_nvs) {
        ESP_LOGE(TAG, "No WiFi credentials found");
        return ESP_ERR_NOT_FOUND;
    }

    return wifi_manager_connect_to(ssid, pass, false);
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

    wifi_config_t wifi_cfg = {};
    strncpy((char *)wifi_cfg.sta.ssid, ssid, sizeof(wifi_cfg.sta.ssid) - 1);
    strncpy((char *)wifi_cfg.sta.password, password, sizeof(wifi_cfg.sta.password) - 1);
    strncpy(s_ssid, ssid, sizeof(s_ssid) - 1);

    s_retry_count = 0;
    s_ipv6_global = false;
    s_ip6_str[0] = '\0';
    set_state(WIFI_STATE_CONNECTING);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi: connecting to %s", ssid);

    EventBits_t bits = xEventGroupWaitBits(s_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdTRUE, pdFALSE, pdMS_TO_TICKS(30000));

    if (bits & WIFI_CONNECTED_BIT) {
        if (save) save_to_nvs(ssid, password);
        return ESP_OK;
    }

    ESP_LOGE(TAG, "Failed to connect to %s", ssid);
    set_state(WIFI_STATE_ERROR);
    return ESP_FAIL;
}

extern "C" esp_err_t wifi_manager_disconnect(void)
{
    if (!s_initialized) {
        set_state(WIFI_STATE_DISCONNECTED);
        return ESP_OK;
    }
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

    /* esp_wifi_scan_start() requires the STA interface to already be
     * started. If we are idle (never connected, or disconnected since
     * wifi_manager_disconnect() stops the driver), esp_wifi_start()
     * fires WIFI_EVENT_STA_START -- guard it with s_scan_in_progress
     * (see its declaration above) so that does not also kick off an
     * unwanted connection attempt using a stale/absent config. */
    s_scan_in_progress = true;
    err = esp_wifi_start();
    if (err != ESP_OK && err != ESP_ERR_WIFI_CONN) {
        s_scan_in_progress = false;
        ESP_LOGE(TAG, "esp_wifi_start (for scan) failed: %s", esp_err_to_name(err));
        return err;
    }

    wifi_scan_config_t scan_cfg = {};
    scan_cfg.show_hidden = false;
    err = esp_wifi_scan_start(&scan_cfg, true /* block until done */);
    s_scan_in_progress = false;
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_scan_start failed: %s", esp_err_to_name(err));
        return err;
    }

    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_num(&ap_count);
    if (ap_count == 0) return ESP_OK;

    wifi_ap_record_t *records = (wifi_ap_record_t *)heap_caps_malloc(
        sizeof(wifi_ap_record_t) * ap_count, MALLOC_CAP_SPIRAM);
    if (!records) {
        records = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * ap_count);
    }
    if (!records) return ESP_ERR_NO_MEM;

    uint16_t got = ap_count;
    err = esp_wifi_scan_get_ap_records(&got, records);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_scan_get_ap_records failed: %s", esp_err_to_name(err));
        free(records);
        return err;
    }

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
extern "C" bool wifi_manager_is_connected(void) { return s_state == WIFI_STATE_CONNECTED; }
extern "C" void wifi_manager_set_callback(wifi_state_callback_t cb) { s_callback = cb; }
extern "C" const char *wifi_manager_get_ip(void) { return s_ip_str; }
extern "C" const char *wifi_manager_get_ssid(void) { return s_ssid; }
extern "C" bool wifi_manager_has_global_ipv6(void) { return s_ipv6_global; }
extern "C" const char *wifi_manager_get_ipv6(void) { return s_ip6_str; }
