#pragma once
#include "FreeRTOS.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef void *TaskHandle_t;
typedef void (*TaskFunction_t)(void *);
BaseType_t xTaskCreatePinnedToCore(TaskFunction_t fn, const char *name,
                                   uint32_t stack, void *arg, UBaseType_t prio,
                                   TaskHandle_t *out, BaseType_t core);
#define xTaskCreate(fn, name, stack, arg, prio, out) \
    xTaskCreatePinnedToCore(fn, name, stack, arg, prio, out, 0)
/* vTaskDelete(NULL) ends every editor task as its last statement, so
 * returning from it simply ends the task. */
void vTaskDelete(TaskHandle_t t);
/* Cannot block on the browser's main thread: returns at once. */
void vTaskDelay(TickType_t ticks);
TickType_t xTaskGetTickCount(void);
#ifdef __cplusplus
}
#endif
