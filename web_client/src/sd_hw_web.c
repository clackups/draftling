/*
 * Web client: the "SD card" under the firmware's own sd_card.cpp is a
 * folder in the browser's origin-private file system (OPFS), mounted
 * by main_web.cpp at /opfs/documents; the program runs in that folder
 * and the card is mounted at ".", as in the Linux program
 * (linux/src/sd_hw_linux.c). The capacity and free space are the
 * browser's quota and use for the site (navigator.storage.estimate(),
 * reported by the page). "Format SD card" empties the folder.
 */

#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "esp_log.h"
#include "linux_host.h"
#include "linux_sd_hw.h"
#include "web_host.h"

static const char *TAG = "Storage";

static sdmmc_card_t s_card;
static bool s_mounted;

esp_err_t gpio_config(const gpio_config_t *cfg) { (void)cfg; return ESP_OK; }
esp_err_t gpio_set_level(gpio_num_t gpio, uint32_t level) { (void)gpio; (void)level; return ESP_OK; }
esp_err_t spi_bus_initialize(spi_host_device_t host, const spi_bus_config_t *cfg, int dma)
{
    (void)host; (void)cfg; (void)dma;
    return ESP_OK;
}
esp_err_t spi_bus_free(spi_host_device_t host) { (void)host; return ESP_OK; }

static esp_err_t mount(const char *base, sdmmc_card_t **out_card)
{
    struct stat st;
    if (stat(base, &st) != 0 || !S_ISDIR(st.st_mode)) {
        ESP_LOGE(TAG, "%s is not a folder", linux_data_dir);
        return ESP_FAIL;
    }
    memset(&s_card, 0, sizeof(s_card));
    double quota, usage;
    web_storage_get(&quota, &usage);
    s_card.ocr = SD_OCR_SDHC_CAP;
    memcpy(s_card.cid.name, "OPFS", 4);
    /* Hardware information shows capacity * sector size. */
    double sectors = quota / 512;
    s_card.csd.capacity = sectors > 0x7fffffff ? 0x7fffffff : (int)sectors;
    s_card.csd.sector_size = 512;
    s_card.real_freq_khz = 0;   /* no bus: no "Interface" row */
    s_mounted = true;
    *out_card = &s_card;
    return ESP_OK;
}

esp_err_t esp_vfs_fat_sdmmc_mount(const char *base, const sdmmc_host_t *host,
                                  const void *slot,
                                  const esp_vfs_fat_mount_config_t *cfg,
                                  sdmmc_card_t **out_card)
{
    (void)host; (void)slot; (void)cfg;
    return mount(base, out_card);
}

esp_err_t esp_vfs_fat_sdspi_mount(const char *base, const sdmmc_host_t *host,
                                  const sdspi_device_config_t *slot,
                                  const esp_vfs_fat_mount_config_t *cfg,
                                  sdmmc_card_t **out_card)
{
    (void)host; (void)slot; (void)cfg;
    return mount(base, out_card);
}

esp_err_t esp_vfs_fat_sdcard_unmount(const char *base, sdmmc_card_t *card)
{
    (void)base; (void)card;
    s_mounted = false;
    return ESP_OK;
}

/* Delete everything below dir, but not dir itself. */
static void empty_dir(const char *dir)
{
    DIR *d = opendir(dir);
    if (!d) return;
    struct dirent *ent;
    char path[4096];
    while ((ent = readdir(d)) != NULL) {
        if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) continue;
        snprintf(path, sizeof(path), "%s/%s", dir, ent->d_name);
        struct stat st;
        if (lstat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
            empty_dir(path);
            rmdir(path);
        } else {
            unlink(path);
        }
    }
    closedir(d);
}

esp_err_t esp_vfs_fat_sdcard_format_cfg(const char *base, sdmmc_card_t *card,
                                        const esp_vfs_fat_mount_config_t *cfg)
{
    (void)card; (void)cfg;
    ESP_LOGW(TAG, "Formatting (emptying) %s", linux_data_dir);
    empty_dir(base);
    return ESP_OK;
}

esp_err_t esp_vfs_fat_info(const char *base, uint64_t *total, uint64_t *free_bytes)
{
    (void)base;
    double quota, usage;
    web_storage_get(&quota, &usage);
    if (quota <= 0) return ESP_FAIL;
    if (total) *total = (uint64_t)quota;
    if (free_bytes) *free_bytes = usage < quota ? (uint64_t)(quota - usage) : 0;
    return ESP_OK;
}

uint8_t ff_diskio_get_pdrv_card(const sdmmc_card_t *card)
{
    (void)card;
    return s_mounted ? 0 : 0xFF;
}

esp_err_t sdmmc_get_status(sdmmc_card_t *card)
{
    (void)card;
    return s_mounted ? ESP_OK : ESP_ERR_INVALID_STATE;
}

void sdmmc_card_print_info(FILE *stream, const sdmmc_card_t *card)
{
    (void)stream; (void)card;
}

esp_err_t sdmmc_read_sectors(sdmmc_card_t *card, void *dst, size_t start, size_t n)
{
    (void)card; (void)start;
    memset(dst, 0, n * 512);
    return ESP_OK;
}
