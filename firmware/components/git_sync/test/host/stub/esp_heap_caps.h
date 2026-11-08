#pragma once
#include <stdlib.h>
#define MALLOC_CAP_SPIRAM 1
#define MALLOC_CAP_DEFAULT 2
#define MALLOC_CAP_INTERNAL 4
#define MALLOC_CAP_8BIT 8
static inline void *heap_caps_malloc(size_t n, int c) { (void)c; return malloc(n); }
static inline void *heap_caps_realloc(void *p, size_t n, int c) { (void)c; return realloc(p, n); }
static inline void heap_caps_free(void *p) { free(p); }
static inline size_t heap_caps_get_free_size(int c) { (void)c; return 1u << 30; }
