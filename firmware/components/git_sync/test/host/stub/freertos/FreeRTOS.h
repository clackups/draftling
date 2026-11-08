#pragma once
#include <stdint.h>
typedef uint8_t StackType_t;
typedef struct { int x; } StaticTask_t;
typedef void *TaskHandle_t;
typedef int BaseType_t;
typedef uint32_t TickType_t;
#define pdMS_TO_TICKS(x) (x)
#define portMAX_DELAY 0xffffffffu
#define pdTRUE 1
#define pdPASS 1
