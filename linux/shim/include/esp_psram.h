#pragma once
#include <stdbool.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* A computer has no PSRAM (the Hardware information page says so);
 * the editor's document budget comes from heap_caps_* instead. */
static inline bool esp_psram_is_initialized(void) { return false; }
static inline size_t esp_psram_get_size(void) { return 0; }
#ifdef __cplusplus
}
#endif
