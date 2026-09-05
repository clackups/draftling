#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <esp_err.h>
#include <stddef.h>
#include <stdbool.h>

/* Initialize the SD card on a 1-bit SDMMC slot. */
esp_err_t sd_card_init(int clk_pin, int cmd_pin, int d0_pin, const char *mount_point);

/*
 * Initialize the SD card on an SPI bus.
 *
 *   spi_host  -- ESP-IDF SPI host id (e.g. SPI2_HOST). The bus may be
 *                shared with other devices; if it has not been initialized
 *                yet, this function will initialize it for you.
 *   miso/mosi/sck -- bus pins (set sck/mosi to -1 if the bus is already
 *                    initialized by another driver, e.g. the e-paper).
 *   cs        -- chip-select GPIO for the SD card.
 *   enable_gpio -- optional power-enable GPIO (driven HIGH), or -1.
 *   mount_point -- VFS mount point, e.g. "/sdcard".
 */
esp_err_t sd_card_init_spi(int spi_host, int miso, int mosi, int sck,
                           int cs, int enable_gpio, const char *mount_point);
esp_err_t sd_card_deinit(void);
bool sd_card_is_ready(void);
const char *sd_card_get_mount_point(void);

/* Read entire file; caller must free(*out_buf) */
esp_err_t sd_card_read_file(const char *path, char **out_buf, size_t *out_len);
esp_err_t sd_card_write_file(const char *path, const char *data, size_t len);
esp_err_t sd_card_append_file(const char *path, const char *data, size_t len);
bool sd_card_file_exists(const char *path);
esp_err_t sd_card_delete_file(const char *path);
esp_err_t sd_card_mkdir(const char *path);

typedef struct {
    char name[256];
    bool is_dir;
    size_t size;
} sd_card_file_entry_t;

int sd_card_list_dir(const char *path, sd_card_file_entry_t *entries, int max_entries);
long sd_card_file_size(const char *path);
esp_err_t sd_card_rename(const char *old_path, const char *new_path);

#ifdef __cplusplus
}
#endif
