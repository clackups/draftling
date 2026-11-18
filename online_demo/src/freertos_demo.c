/*
 * Online demo: the FreeRTOS calls the editor makes, on a single thread.
 *
 * The browser runs the firmware on its main thread only. A "task"
 * (xTaskCreatePinnedToCore) runs to completion from a one-shot LVGL
 * timer, i.e. right after the code that created it has returned to the
 * LVGL loop -- the same order of events the device shows when the task
 * starts on the other core. Every task in the editor ends with
 * vTaskDelete(NULL) and hands its results to the UI with
 * lv_async_call(), so nothing more is needed. vTaskDelay() cannot
 * block here and returns at once.
 */

#include <stdlib.h>
#include <string.h>

#include <emscripten.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "lvgl.h"

typedef struct {
    TaskFunction_t fn;
    void *arg;
} task_start_t;

static void task_timer_cb(lv_timer_t *t)
{
    task_start_t *s = (task_start_t *)lv_timer_get_user_data(t);
    lv_timer_delete(t);
    TaskFunction_t fn = s->fn;
    void *arg = s->arg;
    free(s);
    fn(arg);
}

BaseType_t xTaskCreatePinnedToCore(TaskFunction_t fn, const char *name,
                                   uint32_t stack, void *arg, UBaseType_t prio,
                                   TaskHandle_t *out, BaseType_t core)
{
    (void)name; (void)stack; (void)prio; (void)core;
    task_start_t *s = (task_start_t *)malloc(sizeof(*s));
    if (!s) return pdFAIL;
    s->fn = fn;
    s->arg = arg;
    lv_timer_t *t = lv_timer_create(task_timer_cb, 0, s);
    if (!t) {
        free(s);
        return pdFAIL;
    }
    if (out) *out = t;
    return pdPASS;
}

void vTaskDelete(TaskHandle_t t)
{
    (void)t;
}

void vTaskDelay(TickType_t ticks)
{
    (void)ticks;
}

TickType_t xTaskGetTickCount(void)
{
    return (TickType_t)emscripten_get_now();
}

struct demo_queue {
    UBaseType_t len, size, head, count;
    uint8_t *buf;
};

QueueHandle_t xQueueCreate(UBaseType_t len, UBaseType_t item_size)
{
    struct demo_queue *q = (struct demo_queue *)calloc(1, sizeof(*q));
    if (!q) return NULL;
    q->buf = (uint8_t *)malloc((size_t)len * item_size);
    if (!q->buf) {
        free(q);
        return NULL;
    }
    q->len = len;
    q->size = item_size;
    return q;
}

BaseType_t xQueueSend(QueueHandle_t q, const void *item, TickType_t wait)
{
    (void)wait;
    if (!q || q->count == q->len) return pdFALSE;
    UBaseType_t tail = (q->head + q->count) % q->len;
    memcpy(q->buf + (size_t)tail * q->size, item, q->size);
    q->count++;
    return pdTRUE;
}

BaseType_t xQueueReceive(QueueHandle_t q, void *item, TickType_t wait)
{
    (void)wait;
    if (!q || q->count == 0) return pdFALSE;
    memcpy(item, q->buf + (size_t)q->head * q->size, q->size);
    q->head = (q->head + 1) % q->len;
    q->count--;
    return pdTRUE;
}

BaseType_t xQueueReset(QueueHandle_t q)
{
    if (q) q->head = q->count = 0;
    return pdPASS;
}
