#pragma once
#include <stdbool.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
static inline bool esp_psram_is_initialized(void) { return true; }
static inline size_t esp_psram_get_size(void) { return 8u * 1024 * 1024; }
#ifdef __cplusplus
}
#endif
