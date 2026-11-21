#pragma once
#include <stdint.h>
#include "esp_err.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct esp_flash_t esp_flash_t;
extern esp_flash_t *esp_flash_default_chip;
esp_err_t esp_flash_get_size(esp_flash_t *chip, uint32_t *size);
esp_err_t esp_flash_get_physical_size(esp_flash_t *chip, uint32_t *size);
#ifdef __cplusplus
}
#endif
