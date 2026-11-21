#pragma once
#include "esp_err.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Linux: restores the terminal and starts the program again
 * (src/main_linux.cpp). */
__attribute__((noreturn)) void esp_restart(void);
#define ESP_SYSTEM_EVENT_TASK_STACK_SIZE 2304
#ifdef __cplusplus
}
#endif
