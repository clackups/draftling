/*
 * Online demo: the simulated MicroSD card under the firmware's
 * sd_card.cpp (see shim/include/demo_sd_hw.h). Mounting always works;
 * "formatting" empties the folder; the reported capacity is that of a
 * 4 GB card.
 */

#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "demo_host.h"
#include "demo_sd_hw.h"
#include "esp_log.h"

static const char *TAG = "SDCardDemo";

#define CARD_SECTORS (7741440)   /* a typical "4 GB" card */

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
    mkdir(base, 0755);
    memset(&s_card, 0, sizeof(s_card));
    s_card.ocr = SD_OCR_SDHC_CAP;
    memcpy(s_card.cid.name, "DEMO", 4);
    s_card.csd.capacity = CARD_SECTORS;
    s_card.csd.sector_size = 512;
    s_card.real_freq_khz = 40000;
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

/* Delete everything below dir, but not dir itself (the mount point). */
static void empty_dir(const char *dir)
{
    DIR *d = opendir(dir);
    if (!d) return;
    struct dirent *ent;
    char path[512];
    while ((ent = readdir(d)) != NULL) {
        if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) continue;
        snprintf(path, sizeof(path), "%s/%s", dir, ent->d_name);
        struct stat st;
        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
            empty_dir(path);
            rmdir(path);
        } else {
            remove(path);
        }
    }
    closedir(d);
}

esp_err_t esp_vfs_fat_sdcard_format_cfg(const char *base, sdmmc_card_t *card,
                                        const esp_vfs_fat_mount_config_t *cfg)
{
    (void)card; (void)cfg;
    ESP_LOGW(TAG, "Formatting (emptying) %s", base);
    empty_dir(base);
    demo_host_storage_dirty();
    return ESP_OK;
}

static uint64_t used_bytes(const char *dir)
{
    uint64_t sum = 0;
    DIR *d = opendir(dir);
    if (!d) return 0;
    struct dirent *ent;
    char path[512];
    while ((ent = readdir(d)) != NULL) {
        if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) continue;
        snprintf(path, sizeof(path), "%s/%s", dir, ent->d_name);
        struct stat st;
        if (stat(path, &st) != 0) continue;
        /* Whole 16 KB clusters, as FAT allocates them. */
        if (S_ISDIR(st.st_mode)) sum += 16384 + used_bytes(path);
        else sum += ((uint64_t)st.st_size + 16383) / 16384 * 16384;
    }
    closedir(d);
    return sum;
}

esp_err_t esp_vfs_fat_info(const char *base, uint64_t *total, uint64_t *free_bytes)
{
    uint64_t t = (uint64_t)CARD_SECTORS * 512 - 4u * 1024 * 1024;
    uint64_t u = used_bytes(base);
    if (total) *total = t;
    if (free_bytes) *free_bytes = u < t ? t - u : 0;
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
