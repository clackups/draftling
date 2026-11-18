#pragma once
#include "esp_err.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Online demo: saves the browser storage and reloads the page. */
__attribute__((noreturn)) void esp_restart(void);
#define ESP_SYSTEM_EVENT_TASK_STACK_SIZE 2304
#ifdef __cplusplus
}
#endif
