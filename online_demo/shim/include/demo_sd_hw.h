/*
 * Online demo: the SDMMC / SDSPI / FatFs / GPIO declarations the
 * firmware's own components/sd_card/sd_card.cpp needs, so the demo
 * runs that file unchanged. The "card" is the folder DEMO_SD_MOUNT of
 * Emscripten's file system, which web/demo.js keeps in IndexedDB; the
 * functions are in src/sd_hw_demo.c.
 */
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "esp_err.h"
#include "sdmmc_cmd.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef int gpio_num_t;
typedef enum { GPIO_INTR_DISABLE = 0 } gpio_int_type_t;
typedef enum { GPIO_MODE_DISABLE = 0, GPIO_MODE_INPUT, GPIO_MODE_OUTPUT } gpio_mode_t;
typedef enum { GPIO_PULLUP_DISABLE = 0, GPIO_PULLUP_ENABLE } gpio_pullup_t;
typedef enum { GPIO_PULLDOWN_DISABLE = 0, GPIO_PULLDOWN_ENABLE } gpio_pulldown_t;
typedef struct {
    uint64_t pin_bit_mask;
    gpio_mode_t mode;
    gpio_pullup_t pull_up_en;
    gpio_pulldown_t pull_down_en;
    gpio_int_type_t intr_type;
} gpio_config_t;
esp_err_t gpio_config(const gpio_config_t *cfg);
esp_err_t gpio_set_level(gpio_num_t gpio, uint32_t level);

typedef int spi_host_device_t;
#define SPI_DMA_CH_AUTO 3
typedef struct {
    int mosi_io_num, miso_io_num, sclk_io_num, quadwp_io_num, quadhd_io_num;
    int max_transfer_sz;
} spi_bus_config_t;
esp_err_t spi_bus_initialize(spi_host_device_t host, const spi_bus_config_t *cfg, int dma);
esp_err_t spi_bus_free(spi_host_device_t host);

typedef struct {
    int slot;
    int max_freq_khz;
} sdmmc_host_t;
#define SDMMC_HOST_DEFAULT() ((sdmmc_host_t){ 1, 40000 })
#define SDSPI_HOST_DEFAULT() ((sdmmc_host_t){ 1, 20000 })
typedef struct {
    int width;
    gpio_num_t clk, cmd, d0;
} sdmmc_slot_config_t;
#define SDMMC_SLOT_CONFIG_DEFAULT() ((sdmmc_slot_config_t){ 1, -1, -1, -1 })
typedef struct {
    gpio_num_t gpio_cs;
    spi_host_device_t host_id;
} sdspi_device_config_t;
#define SDSPI_DEVICE_CONFIG_DEFAULT() ((sdspi_device_config_t){ -1, 1 })

typedef struct {
    bool format_if_mount_failed;
    int max_files;
    size_t allocation_unit_size;
} esp_vfs_fat_mount_config_t;
typedef esp_vfs_fat_mount_config_t esp_vfs_fat_sdmmc_mount_config_t;

esp_err_t esp_vfs_fat_sdmmc_mount(const char *base, const sdmmc_host_t *host,
                                  const void *slot,
                                  const esp_vfs_fat_mount_config_t *cfg,
                                  sdmmc_card_t **out_card);
esp_err_t esp_vfs_fat_sdspi_mount(const char *base, const sdmmc_host_t *host,
                                  const sdspi_device_config_t *slot,
                                  const esp_vfs_fat_mount_config_t *cfg,
                                  sdmmc_card_t **out_card);
esp_err_t esp_vfs_fat_sdcard_unmount(const char *base, sdmmc_card_t *card);
esp_err_t esp_vfs_fat_sdcard_format_cfg(const char *base, sdmmc_card_t *card,
                                        const esp_vfs_fat_mount_config_t *cfg);
esp_err_t esp_vfs_fat_info(const char *base, uint64_t *total, uint64_t *free_bytes);
uint8_t ff_diskio_get_pdrv_card(const sdmmc_card_t *card);
esp_err_t sdmmc_get_status(sdmmc_card_t *card);
void sdmmc_card_print_info(FILE *stream, const sdmmc_card_t *card);
esp_err_t sdmmc_read_sectors(sdmmc_card_t *card, void *dst, size_t start, size_t n);

#ifdef __cplusplus
}
#endif
