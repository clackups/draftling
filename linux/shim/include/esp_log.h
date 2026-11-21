/* Linux: ESP_LOGx go to stderr, filtered by the level -v / -q set
 * (src/esp_linux.c; warnings and errors by default). */
#pragma once
#include <stdio.h>
#include <stdint.h>
#include "esp_err.h"
#ifdef __cplusplus
extern "C" {
#endif
enum { LINUX_LOG_ERROR = 1, LINUX_LOG_WARN, LINUX_LOG_INFO, LINUX_LOG_DEBUG };
extern int linux_log_level;
void linux_log(int level, const char *tag, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));
#ifdef __cplusplus
}
#endif
#define LINUX_LOG_(lvl, tag, fmt, ...) \
    do { if (linux_log_level >= (lvl)) linux_log((lvl), (tag), fmt, ##__VA_ARGS__); } while (0)
#define ESP_LOGE(tag, fmt, ...) LINUX_LOG_(LINUX_LOG_ERROR, tag, fmt, ##__VA_ARGS__)
#define ESP_LOGW(tag, fmt, ...) LINUX_LOG_(LINUX_LOG_WARN,  tag, fmt, ##__VA_ARGS__)
#define ESP_LOGI(tag, fmt, ...) LINUX_LOG_(LINUX_LOG_INFO,  tag, fmt, ##__VA_ARGS__)
#define ESP_LOGD(tag, fmt, ...) LINUX_LOG_(LINUX_LOG_DEBUG, tag, fmt, ##__VA_ARGS__)
#define ESP_LOGV(tag, fmt, ...) do { } while (0)
