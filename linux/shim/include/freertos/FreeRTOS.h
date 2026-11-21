/* Linux: the FreeRTOS API the editor and the Git client use, on POSIX
 * threads (src/freertos_linux.c). A task is a detached thread, a tick
 * is a millisecond, queues and task notifications block for real, and
 * a critical section is one process-wide recursive mutex. */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <assert.h>  /* ESP-IDF's FreeRTOS.h pulls it in too */
#ifdef __cplusplus
extern "C" {
#endif
typedef int BaseType_t;
typedef unsigned int UBaseType_t;
typedef uint32_t TickType_t;
typedef uint8_t StackType_t;
typedef struct { int unused; } StaticTask_t;
#define pdTRUE  1
#define pdFALSE 0
#define pdPASS  1
#define pdFAIL  0
#define portMAX_DELAY ((TickType_t)0xffffffffUL)
#define configTICK_RATE_HZ 1000
#define portTICK_PERIOD_MS 1
#define pdMS_TO_TICKS(ms) ((TickType_t)(ms))
typedef struct { int unused; } portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED { 0 }
void linux_critical_enter(void);
void linux_critical_exit(void);
#define taskENTER_CRITICAL(m) do { (void)(m); linux_critical_enter(); } while (0)
#define taskEXIT_CRITICAL(m)  do { (void)(m); linux_critical_exit(); } while (0)
#define portENTER_CRITICAL(m) taskENTER_CRITICAL(m)
#define portEXIT_CRITICAL(m)  taskEXIT_CRITICAL(m)
#ifdef __cplusplus
}
#endif
