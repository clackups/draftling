/*
 * HarfBuzz allocator (hb_*_impl, config/hb_draftling_config.h): PSRAM
 * first, internal RAM only when PSRAM is full, so the shaper does not
 * eat the internal RAM the radio needs.
 */
#include <stddef.h>
#include "esp_heap_caps.h"

#define PREFER 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT, MALLOC_CAP_8BIT

void *draftling_hb_malloc(size_t size)
{
    return heap_caps_malloc_prefer(size, PREFER);
}

void *draftling_hb_calloc(size_t nmemb, size_t size)
{
    return heap_caps_calloc_prefer(nmemb, size, PREFER);
}

void *draftling_hb_realloc(void *ptr, size_t size)
{
    return heap_caps_realloc_prefer(ptr, size, PREFER);
}

void draftling_hb_free(void *ptr)
{
    heap_caps_free(ptr);
}
