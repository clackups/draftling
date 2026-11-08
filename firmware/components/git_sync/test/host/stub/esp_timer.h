#pragma once
#include <stdint.h>
#include <time.h>
static inline int64_t esp_timer_get_time(void) { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return ts.tv_sec * 1000000LL + ts.tv_nsec / 1000; }
