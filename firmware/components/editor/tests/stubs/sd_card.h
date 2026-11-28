#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <esp_err.h>

typedef struct {
    char name[256];
    bool is_dir;
    uint64_t size;
} sd_card_file_entry_t;

const char *sd_card_get_mount_point(void);
int sd_card_list_dir(const char *path, sd_card_file_entry_t *out, int max);
bool sd_card_file_exists(const char *path);
bool sd_card_is_dir(const char *path);
esp_err_t sd_card_mkdir(const char *path);
esp_err_t sd_card_remove_tree(const char *path);
esp_err_t sd_card_rename(const char *src, const char *dst);
esp_err_t sd_card_read_file(const char *path, char **out, size_t *len);
esp_err_t sd_card_write_file(const char *path, const char *data, size_t len);
esp_err_t sd_card_delete_file(const char *path);
