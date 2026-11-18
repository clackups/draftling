#pragma once
#include "FreeRTOS.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct demo_queue *QueueHandle_t;
QueueHandle_t xQueueCreate(UBaseType_t len, UBaseType_t item_size);
BaseType_t xQueueSend(QueueHandle_t q, const void *item, TickType_t wait);
BaseType_t xQueueReceive(QueueHandle_t q, void *item, TickType_t wait);
BaseType_t xQueueReset(QueueHandle_t q);
#ifdef __cplusplus
}
#endif
