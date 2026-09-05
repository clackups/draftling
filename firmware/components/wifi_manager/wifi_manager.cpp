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
            esp_wifi_connect();
        } else if (id == WIFI_EVENT_STA_DISCONNECTED) {
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
    if (esp_netif_get_handle_from_ifkey("WIFI_STA_DEF") == NULL) {
        esp_netif_create_default_wifi_sta();
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

    esp_event_handler_instance_t inst_any, inst_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &inst_any));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &inst_ip));

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

    /* Try NVS first, then config file */
    if (!load_from_nvs(ssid, sizeof(ssid), pass, sizeof(pass))) {
        if (!load_from_file(ssid, sizeof(ssid), pass, sizeof(pass))) {
            ESP_LOGE(TAG, "No WiFi credentials found");
            return ESP_ERR_NOT_FOUND;
        }
        /* Save file credentials to NVS for next time */
        save_to_nvs(ssid, pass);
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
    set_state(WIFI_STATE_DISCONNECTED);
    return ESP_OK;
}

extern "C" wifi_state_t wifi_manager_get_state(void) { return s_state; }
extern "C" bool wifi_manager_is_connected(void) { return s_state == WIFI_STATE_CONNECTED; }
extern "C" void wifi_manager_set_callback(wifi_state_callback_t cb) { s_callback = cb; }
extern "C" const char *wifi_manager_get_ip(void) { return s_ip_str; }
extern "C" const char *wifi_manager_get_ssid(void) { return s_ssid; }
