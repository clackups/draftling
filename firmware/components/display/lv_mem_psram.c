/*
 * LVGL custom memory allocator that routes lv_malloc/lv_free through
 * ESP heap_caps allocators with MALLOC_CAP_SPIRAM, so LVGL widgets,
 * styles, and other small allocations land in PSRAM instead of the
 * scarce internal DRAM.
 *
 * Activated by CONFIG_LV_USE_CUSTOM_MALLOC=y in sdkconfig.defaults.
 * When that Kconfig is set, LVGL compiles its built-in TLSF allocator
 * out (#if LV_USE_STDLIB_MALLOC == LV_STDLIB_BUILTIN) and links these
 * symbols instead. Without this file the build would fail with
 * undefined references to lv_malloc_core / lv_free_core / etc.
 *
 * This translation unit is injected directly into the lvgl__lvgl
 * managed-component library by components/display/CMakeLists.txt
 * (idf_component_get_property + target_sources) so that the symbols
 * end up in the same static archive (liblvgl__lvgl.a) as the
 * lv_mem.c references. Compiling it as part of libdisplay.a turned
 * out to be fragile: with archives in a single --start-group the
 * linker is supposed to find it, but in practice the display archive
 * was sometimes processed before lvgl__lvgl's unresolved lv_*_core
 * references were scanned, leaving them undefined at final link.
 *
 * Why this matters
 * ----------------
 * On the M5Stack PaperS3 (8 MB PSRAM) the epdiy framebuffers,
 * Bluedroid host, WiFi static RX buffers, and LVGL all compete for
 * ~190 KB of internal DRAM. Before this change LVGL kept its own 64 KB
 * TLSF pool as a static array in .bss (DRAM); after this change that
 * pool is gone and every lv_malloc call goes straight to PSRAM.
 *
 * heap_caps_realloc with a NULL pointer behaves like heap_caps_malloc,
 * and heap_caps_free of NULL is a no-op, matching the malloc/free
 * contract LVGL expects.
 */

#include <stddef.h>
#include <string.h>

#include "esp_heap_caps.h"

#include "lvgl.h"

#define DRAFTLING_LVGL_HEAP_CAPS  (MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)

void lv_mem_init(void)
{
    /* Nothing to set up: heap_caps is initialized by ESP-IDF startup
     * long before LVGL calls us. */
}

void lv_mem_deinit(void)
{
    /* No private state to tear down. */
}

void *lv_malloc_core(size_t size)
{
    return heap_caps_malloc(size, DRAFTLING_LVGL_HEAP_CAPS);
}

void lv_free_core(void *p)
{
    heap_caps_free(p);
}

void *lv_realloc_core(void *p, size_t new_size)
{
    return heap_caps_realloc(p, new_size, DRAFTLING_LVGL_HEAP_CAPS);
}

void lv_mem_monitor_core(lv_mem_monitor_t *mon_p)
{
    if (mon_p == NULL) return;
    memset(mon_p, 0, sizeof(*mon_p));

    /* Report the current PSRAM heap state as a coarse approximation of
     * the LVGL pool state. We do not own a private pool, so per-LVGL
     * usage counters (used_cnt, max_used) cannot be filled in. */
    size_t total = heap_caps_get_total_size(DRAFTLING_LVGL_HEAP_CAPS);
    size_t free_bytes = heap_caps_get_free_size(DRAFTLING_LVGL_HEAP_CAPS);
    size_t largest = heap_caps_get_largest_free_block(DRAFTLING_LVGL_HEAP_CAPS);

    mon_p->total_size = total;
    mon_p->free_size = free_bytes;
    mon_p->free_biggest_size = largest;
    if (total > 0 && total >= free_bytes) {
        mon_p->used_pct = (uint8_t)(((total - free_bytes) * 100U) / total);
    }
}

lv_result_t lv_mem_test_core(void)
{
    /* heap_caps maintains its own integrity; nothing meaningful to
     * test here without owning a pool. */
    return LV_RESULT_OK;
}

lv_mem_pool_t lv_mem_add_pool(void *mem, size_t bytes)
{
    /* External pools are not supported because we delegate to
     * heap_caps. Returning NULL matches LVGL's "failed to add pool"
     * convention. */
    (void)mem;
    (void)bytes;
    return NULL;
}

void lv_mem_remove_pool(lv_mem_pool_t pool)
{
    (void)pool;
}
