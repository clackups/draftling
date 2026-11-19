/*
 * Online demo: the ESP-IDF system calls the editor makes. The answers
 * describe the simulated board, a Freenove FNK0104S (ESP32-S3, 16 MB
 * flash, 8 MB PSRAM).
 */

#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include <emscripten.h>

#include "demo_host.h"
#include "esp_app_desc.h"
#include "esp_chip_info.h"
#include "esp_err.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "esp_timer.h"

#ifndef DRAFTLING_VERSION
#define DRAFTLING_VERSION "dev"
#endif

const char *esp_err_to_name(esp_err_t code)
{
    switch (code) {
    case ESP_OK:                  return "ESP_OK";
    case ESP_FAIL:                return "ESP_FAIL";
    case ESP_ERR_NO_MEM:          return "ESP_ERR_NO_MEM";
    case ESP_ERR_INVALID_ARG:     return "ESP_ERR_INVALID_ARG";
    case ESP_ERR_INVALID_STATE:   return "ESP_ERR_INVALID_STATE";
    case ESP_ERR_INVALID_SIZE:    return "ESP_ERR_INVALID_SIZE";
    case ESP_ERR_NOT_FOUND:       return "ESP_ERR_NOT_FOUND";
    case ESP_ERR_NOT_SUPPORTED:   return "ESP_ERR_NOT_SUPPORTED";
    case ESP_ERR_TIMEOUT:         return "ESP_ERR_TIMEOUT";
    case ESP_ERR_NVS_NOT_FOUND:   return "ESP_ERR_NVS_NOT_FOUND";
    default:                      return "ERROR";
    }
}

#define PSRAM_TOTAL    (8u * 1024 * 1024)
#define INTERNAL_TOTAL (390u * 1024)

/* The PSRAM heap holds the documents, LVGL and the rest, as on the
 * device, so its free space follows the C heap's use. */
static size_t psram_free(void)
{
    struct mallinfo mi = mallinfo();
    size_t used = (size_t)mi.uordblks;
    return used < PSRAM_TOTAL ? PSRAM_TOTAL - used : 0;
}

size_t heap_caps_get_total_size(uint32_t caps)
{
    return (caps & MALLOC_CAP_SPIRAM) ? PSRAM_TOTAL : INTERNAL_TOTAL;
}

size_t heap_caps_get_free_size(uint32_t caps)
{
    return (caps & MALLOC_CAP_SPIRAM) ? psram_free() : 182u * 1024;
}

size_t heap_caps_get_largest_free_block(uint32_t caps)
{
    return heap_caps_get_free_size(caps);
}

esp_err_t esp_read_mac(uint8_t *mac, esp_mac_type_t type)
{
    static const uint8_t base[6] = { 0x02, 0xd2, 0xaf, 0x7d, 0xe0, 0x00 };
    memcpy(mac, base, 6);
    if (type == ESP_MAC_BT) mac[5] += 2;
    return ESP_OK;
}

void esp_chip_info(esp_chip_info_t *out)
{
    memset(out, 0, sizeof(*out));
    out->model = CHIP_ESP32S3;
    out->features = CHIP_FEATURE_WIFI_BGN | CHIP_FEATURE_BLE;
    out->revision = 2;
    out->cores = 2;
}

esp_flash_t *esp_flash_default_chip = NULL;

esp_err_t esp_flash_get_size(esp_flash_t *chip, uint32_t *size)
{
    (void)chip;
    *size = 16u * 1024 * 1024;
    return ESP_OK;
}

esp_err_t esp_flash_get_physical_size(esp_flash_t *chip, uint32_t *size)
{
    return esp_flash_get_size(chip, size);
}

const esp_app_desc_t *esp_app_get_description(void)
{
    static esp_app_desc_t desc;
    if (!desc.version[0]) {
        snprintf(desc.version, sizeof(desc.version), "%s", DRAFTLING_VERSION);
        snprintf(desc.project_name, sizeof(desc.project_name), "draftling");
        snprintf(desc.idf_ver, sizeof(desc.idf_ver), "online demo");
    }
    return &desc;
}

int64_t esp_timer_get_time(void)
{
    return (int64_t)(emscripten_get_now() * 1000.0);
}

void esp_restart(void)
{
    demo_host_restart();
    demo_halt();
}
