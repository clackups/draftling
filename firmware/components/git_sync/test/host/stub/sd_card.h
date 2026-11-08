#pragma once
#include <stddef.h>
#include <stdbool.h>
#include "esp_err.h"
typedef struct { char name[256]; bool is_dir; size_t size; } sd_card_file_entry_t;
#ifdef __cplusplus
extern "C" {
#endif
bool sd_card_is_ready(void);
esp_err_t sd_card_read_file(const char *path, char **out, size_t *len);
esp_err_t sd_card_write_file(const char *path, const char *d, size_t n);
bool sd_card_file_exists(const char *path);
esp_err_t sd_card_replace_with(const char *tmp, const char *path);
esp_err_t sd_card_delete_file(const char *path);
esp_err_t sd_card_list_dir(const char *path, sd_card_file_entry_t **out, int *count);
void sd_card_free_list(sd_card_file_entry_t *l);
#ifdef __cplusplus
}
#endif
