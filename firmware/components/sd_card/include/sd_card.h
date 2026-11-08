#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <esp_err.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <sdmmc_cmd.h>

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

/* The sdmmc_card_t handle behind the currently mounted card, or NULL
 * if none is mounted. Used by the usb_msc component to expose the
 * same card's raw sectors to a USB host (tinyusb_msc_new_storage_sdmmc()
 * reads/writes sectors directly against this handle -- it never goes
 * through the FatFs mount sd_card.cpp keeps registered at
 * sd_card_get_mount_point(), so local firmware code must simply stay
 * off the card, rather than unmount it, while USB MSC owns it. */
sdmmc_card_t *sd_card_get_handle(void);

/* Size and free space of the mounted FAT file system, in bytes.
 * Returns ESP_ERR_INVALID_STATE when no card is mounted. */
esp_err_t sd_card_get_fs_space(uint64_t *total_bytes, uint64_t *free_bytes);

/* Erase the mounted card by creating a fresh FAT file system on it
 * (every file and directory is lost), then mount it again at the same
 * mount point. Blocking: can take several seconds on a large card.
 * Nothing else may touch the card while this runs. Returns
 * ESP_ERR_INVALID_STATE when no card is mounted; after any other
 * failure the card may be left unmounted (sd_card_is_ready() false). */
esp_err_t sd_card_format(void);

/* Read entire file; caller must free(*out_buf) */
esp_err_t sd_card_read_file(const char *path, char **out_buf, size_t *out_len);
/* Overwrite a file atomically: the bytes are written to a temporary
 * file and flushed to the card, then renamed over `path`. A crash or
 * power loss mid-write leaves the previous contents of `path` intact. */
esp_err_t sd_card_write_file(const char *path, const char *data, size_t len);
/* Like sd_card_write_file(), the content being a[0, alen) followed by
 * b[0, blen) (the editor saves the two halves of its gap buffer). */
esp_err_t sd_card_write_file_parts(const char *path, const char *a, size_t alen,
                                   const char *b, size_t blen);
esp_err_t sd_card_append_file(const char *path, const char *data, size_t len);
bool sd_card_file_exists(const char *path);

/* Replace path with the complete, synced file tmp (same folder) so
 * that a power loss at any point leaves either the old or the new
 * contents, recovered at the next mount if need be. tmp should be
 * named "<dir>/.<name>.dtmp". */
esp_err_t sd_card_replace_with(const char *tmp, const char *path);
esp_err_t sd_card_delete_file(const char *path);
esp_err_t sd_card_mkdir(const char *path);

typedef struct {
    char name[256];
    bool is_dir;
    size_t size;
} sd_card_file_entry_t;

/* List every entry of a directory except hidden (dot) files, sorted:
 * folders first, then files, by name ignoring case. *out is allocated
 * (PSRAM) and must be released with sd_card_free_list(); it is NULL
 * when the folder is empty. Returns ESP_FAIL when the folder cannot be
 * read and ESP_ERR_NO_MEM when the list does not fit in memory -- never
 * a partial list. */
esp_err_t sd_card_list_dir(const char *path, sd_card_file_entry_t **out, int *count);
void sd_card_free_list(sd_card_file_entry_t *list);
long sd_card_file_size(const char *path);
esp_err_t sd_card_rename(const char *old_path, const char *new_path);

/* True if path names an existing directory. */
bool sd_card_is_dir(const char *path);

/* Copy a regular file. Fails with ESP_ERR_INVALID_STATE if dst exists. */
esp_err_t sd_card_copy_file(const char *src, const char *dst);

/* Delete a directory with everything in it, hidden files included
 * (e.g. a project folder with its .git history). */
esp_err_t sd_card_remove_tree(const char *path);

#ifdef __cplusplus
}
#endif
