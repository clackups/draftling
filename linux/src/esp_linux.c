/*
 * Linux: the ESP-IDF system calls the editor and the Git client make,
 * answered for the computer the program runs on (the Hardware
 * information page shows them).
 */

#include <dirent.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

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

void linux_log(int level, const char *tag, const char *fmt, ...)
{
    static const char lv[] = "?EWID";
    char ts[16];
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    struct tm tm;
    localtime_r(&t.tv_sec, &tm);
    strftime(ts, sizeof(ts), "%H:%M:%S", &tm);
    flockfile(stderr);
    fprintf(stderr, "%s %c %s: ", ts, lv[level < 0 || level > 4 ? 0 : level], tag);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    funlockfile(stderr);
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
 * "Internal RAM" is the computer's memory (/proc/meminfo). The editor
 * sizes its document buffer from the "PSRAM" heap (three quarters of
 * what is free after a 2 MB reserve): a fixed DOC_BUDGET, so the
 * largest document does not depend on the computer. The buffer is
 * allocated untouched, so only what a document uses becomes resident. */
#define DOC_BUDGET ((size_t)64 * 1024 * 1024)

static size_t meminfo_kb(const char *key)
{
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) return 0;
    char line[128];
    size_t klen = strlen(key), v = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, key, klen) == 0 && line[klen] == ':') {
            v = (size_t)strtoull(line + klen + 1, NULL, 10);
            break;
        }
    }
    fclose(f);
    return v;
}

size_t heap_caps_get_total_size(uint32_t caps)
{
    if (caps & MALLOC_CAP_SPIRAM) return DOC_BUDGET;
    size_t kb = meminfo_kb("MemTotal");
    if (kb) return kb * 1024;
    long pages = sysconf(_SC_PHYS_PAGES), psz = sysconf(_SC_PAGESIZE);
    return pages > 0 && psz > 0 ? (size_t)pages * (size_t)psz : 0;
}

size_t heap_caps_get_free_size(uint32_t caps)
{
    if (caps & MALLOC_CAP_SPIRAM) return DOC_BUDGET;
    size_t kb = meminfo_kb("MemAvailable");
    return kb * 1024;
}

size_t heap_caps_get_largest_free_block(uint32_t caps)
{
    return heap_caps_get_free_size(caps);
}

/* ---- identity ------------------------------------------------------------- */

static bool read_mac_file(const char *iface, uint8_t *mac)
{
    char path[300];
    snprintf(path, sizeof(path), "/sys/class/net/%s/address", iface);
    FILE *f = fopen(path, "r");
    if (!f) return false;
    unsigned b[6];
    int n = fscanf(f, "%x:%x:%x:%x:%x:%x", &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]);
    fclose(f);
    if (n != 6) return false;
    unsigned any = 0;
    for (int i = 0; i < 6; i++) {
        mac[i] = (uint8_t)b[i];
        any |= b[i];
    }
    return any != 0;
}

/* The wireless interface's address for ESP_MAC_WIFI_STA (the first
 * interface with a "wireless" directory), else the first interface
 * that has one. */
esp_err_t esp_read_mac(uint8_t *mac, esp_mac_type_t type)
{
    DIR *d = opendir("/sys/class/net");
    if (!d) return ESP_ERR_NOT_SUPPORTED;
    struct dirent *e;
    char fallback[256] = "";
    bool found = false;
    while ((e = readdir(d)) != NULL) {
        if (e->d_name[0] == '.' || strcmp(e->d_name, "lo") == 0) continue;
        char wpath[300];
        snprintf(wpath, sizeof(wpath), "/sys/class/net/%s/wireless", e->d_name);
        if (type == ESP_MAC_WIFI_STA && access(wpath, F_OK) == 0 &&
            read_mac_file(e->d_name, mac)) {
            found = true;
            break;
        }
        if (!fallback[0]) snprintf(fallback, sizeof(fallback), "%s", e->d_name);
    }
    closedir(d);
    if (found) return ESP_OK;
    if (fallback[0] && read_mac_file(fallback, mac)) return ESP_OK;
    memset(mac, 0, 6);
    return ESP_ERR_NOT_FOUND;
}

void esp_chip_info(esp_chip_info_t *out)
{
    memset(out, 0, sizeof(*out));
    long n = sysconf(_SC_NPROCESSORS_ONLN);
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
        snprintf(desc.idf_ver, sizeof(desc.idf_ver), "linux");
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
