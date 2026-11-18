#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum { CHIP_ESP32S3 = 9 } esp_chip_model_t;
#define CHIP_FEATURE_EMB_FLASH (1 << 0)
#define CHIP_FEATURE_WIFI_BGN  (1 << 1)
#define CHIP_FEATURE_BLE       (1 << 4)
#define CHIP_FEATURE_EMB_PSRAM (1 << 6)
typedef struct {
    esp_chip_model_t model;
    uint32_t features;
    uint16_t revision;
    uint8_t cores;
} esp_chip_info_t;
void esp_chip_info(esp_chip_info_t *out);
#ifdef __cplusplus
}
#endif
