/* Draftling's HarfBuzz configuration overrides, included after
 * hb-config.hh (HB_CONFIG_OVERRIDE_LAST_H, see CMakeLists.txt). */
#pragma once

/* HB_TINY turns on HB_NO_ERRNO, which replaces errno with a
 * thread-local variable of its own; that clashes with the errno of
 * the ESP-IDF C library. Use the real one. */
#undef HB_NO_ERRNO

/* Allocate from PSRAM (draftling_alloc.c): HarfBuzz builds its shape
 * plans and table caches from many small blocks -- some 20 KB per
 * script -- which ESP-IDF would put in internal RAM (blocks below
 * CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL). On the ESP32-C61 that starved
 * the BLE controller and dropped the keyboard link. Not in the host
 * tests, which use the C library. */
#ifdef ESP_PLATFORM
#define hb_malloc_impl  draftling_hb_malloc
#define hb_calloc_impl  draftling_hb_calloc
#define hb_realloc_impl draftling_hb_realloc
#define hb_free_impl    draftling_hb_free
#endif
