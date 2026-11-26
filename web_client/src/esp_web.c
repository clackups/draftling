/*
 * Web client: the ESP-IDF system calls the editor and the Git client
 * make, answered for the browser (the Hardware information page shows
 * them). The counterpart of linux/src/esp_linux.c.
 */

#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <emscripten.h>
#include <emscripten/heap.h>
#include <emscripten/threading.h>

#include "esp_app_desc.h"
#include "esp_chip_info.h"
#include "esp_err.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "linux_host.h"

#ifndef DRAFTLING_VERSION
#define DRAFTLING_VERSION "dev"
#endif

int linux_log_level = LINUX_LOG_WARN;

/* Messages go to the browser's console (Emscripten prints standard
 * error with console.error / console.warn). */
void linux_log(int level, const char *tag, const char *fmt, ...)
{
    static const char lv[] = "?EWID";
    char ts[16];
    time_t t = time(NULL);
    struct tm tm;
    localtime_r(&t, &tm);
    strftime(ts, sizeof(ts), "%H:%M:%S", &tm);
    char msg[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    fprintf(stderr, "%s %c %s: %s\n", ts, lv[level < 0 || level > 4 ? 0 : level], tag, msg);
}

const char *esp_err_to_name(esp_err_t code)
{
    switch (code) {
    case ESP_OK:                   return "ESP_OK";
    case ESP_FAIL:                 return "ESP_FAIL";
    case ESP_ERR_NO_MEM:           return "ESP_ERR_NO_MEM";
    case ESP_ERR_INVALID_ARG:      return "ESP_ERR_INVALID_ARG";
    case ESP_ERR_INVALID_STATE:    return "ESP_ERR_INVALID_STATE";
    case ESP_ERR_INVALID_SIZE:     return "ESP_ERR_INVALID_SIZE";
    case ESP_ERR_NOT_FOUND:        return "ESP_ERR_NOT_FOUND";
    case ESP_ERR_NOT_SUPPORTED:    return "ESP_ERR_NOT_SUPPORTED";
    case ESP_ERR_TIMEOUT:          return "ESP_ERR_TIMEOUT";
    case ESP_ERR_INVALID_RESPONSE: return "ESP_ERR_INVALID_RESPONSE";
    case ESP_ERR_NVS_NOT_FOUND:    return "ESP_ERR_NVS_NOT_FOUND";
    default:                       return "ERROR";
    }
}

/* ---- memory ----------------------------------------------------------------
 *
 * "Internal RAM" is the WebAssembly memory (it grows up to the
 * browser's limit). The editor sizes its document buffer from the
 * "PSRAM" heap (three quarters of what is free after a 2 MB reserve):
 * a fixed DOC_BUDGET, so the largest document is the same in every
 * browser, about 22 MB. */
#define DOC_BUDGET ((size_t)32 * 1024 * 1024)

size_t heap_caps_get_total_size(uint32_t caps)
{
    if (caps & MALLOC_CAP_SPIRAM) return DOC_BUDGET;
    return emscripten_get_heap_max();
}

size_t heap_caps_get_free_size(uint32_t caps)
{
    if (caps & MALLOC_CAP_SPIRAM) return DOC_BUDGET;
    struct mallinfo mi = mallinfo();
    size_t max = emscripten_get_heap_max(), used = (size_t)mi.uordblks;
    return used < max ? max - used : 0;
}

size_t heap_caps_get_largest_free_block(uint32_t caps)
{
    return heap_caps_get_free_size(caps);
}

/* ---- identity ------------------------------------------------------------- */

/* A page cannot see network interfaces. */
esp_err_t esp_read_mac(uint8_t *mac, esp_mac_type_t type)
{
    (void)type;
    memset(mac, 0, 6);
    return ESP_ERR_NOT_SUPPORTED;
}

void esp_chip_info(esp_chip_info_t *out)
{
    memset(out, 0, sizeof(*out));
    int n = emscripten_num_logical_cores();
    out->cores = (uint8_t)(n > 0 && n < 256 ? n : 1);
}

esp_flash_t *esp_flash_default_chip = NULL;

/* No flash chip: the Hardware information page leaves the row out. */
esp_err_t esp_flash_get_size(esp_flash_t *chip, uint32_t *size)
{
    (void)chip; (void)size;
    return ESP_ERR_NOT_SUPPORTED;
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
        snprintf(desc.idf_ver, sizeof(desc.idf_ver), "web");
    }
    return &desc;
}

int64_t esp_timer_get_time(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

void esp_restart(void)
{
    linux_restart();
}
