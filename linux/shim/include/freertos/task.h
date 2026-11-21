#pragma once
#include "FreeRTOS.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct linux_task *TaskHandle_t;
typedef void (*TaskFunction_t)(void *);
/* The stack size is in bytes, as in ESP-IDF; threads get at least
 * 1 MB, since host code paths (libc, OpenSSL) need more than the
 * device does. The core and the priority are ignored. */
BaseType_t xTaskCreatePinnedToCore(TaskFunction_t fn, const char *name,
                                   uint32_t stack, void *arg, UBaseType_t prio,
                                   TaskHandle_t *out, BaseType_t core);
#define xTaskCreate(fn, name, stack, arg, prio, out) \
    xTaskCreatePinnedToCore(fn, name, stack, arg, prio, out, 0)
/* The caller's stack buffer is not used (the thread gets its own). */
TaskHandle_t xTaskCreateStaticPinnedToCore(TaskFunction_t fn, const char *name,
                                           uint32_t stack, void *arg, UBaseType_t prio,
                                           StackType_t *stack_buf, StaticTask_t *tcb,
                                           BaseType_t core);
/* vTaskDelete(NULL) ends the calling task's thread. Deleting another
 * task is not supported (the editor never does). */
void vTaskDelete(TaskHandle_t t);
void vTaskDelay(TickType_t ticks);
TickType_t xTaskGetTickCount(void);
TaskHandle_t xTaskGetCurrentTaskHandle(void);
uint32_t ulTaskNotifyTake(BaseType_t clear_on_exit, TickType_t wait);
BaseType_t xTaskNotifyGive(TaskHandle_t t);
#ifdef __cplusplus
}
#endif
