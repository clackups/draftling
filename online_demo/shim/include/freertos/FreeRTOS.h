/* Online demo: a single-threaded stand-in for the FreeRTOS API the
 * editor uses (src/freertos_demo.c). WebAssembly here runs on the
 * browser's main thread only: a "task" runs to completion from a
 * one-shot LVGL timer right after the code that created it returns,
 * locks never block, and critical sections are no-ops. */
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
#define taskENTER_CRITICAL(m) do { (void)(m); } while (0)
#define taskEXIT_CRITICAL(m)  do { (void)(m); } while (0)
#define portENTER_CRITICAL(m) do { (void)(m); } while (0)
#define portEXIT_CRITICAL(m)  do { (void)(m); } while (0)
#ifdef __cplusplus
}
#endif
