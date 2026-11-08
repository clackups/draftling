#pragma once
#include "FreeRTOS.h"
#include <unistd.h>
#ifdef __cplusplus
extern "C" {
#endif
static inline void vTaskDelay(TickType_t t) { (void)t; }
void vTaskDelete(TaskHandle_t t);
TaskHandle_t xTaskCreateStaticPinnedToCore(void (*fn)(void *), const char *n, uint32_t depth,
                                           void *arg, int prio, StackType_t *stack,
                                           StaticTask_t *tcb, int core);
#ifdef __cplusplus
}
#endif
static inline uint32_t ulTaskNotifyTake(int clear, uint32_t t) { (void)clear; (void)t; return 1; }
static inline void xTaskNotifyGive(TaskHandle_t t) { (void)t; }
