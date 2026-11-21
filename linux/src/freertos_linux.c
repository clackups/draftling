/*
 * Linux: the FreeRTOS calls of the editor and the Git client on POSIX
 * threads.
 *
 * On the device the LVGL loop is one task and the editor's workers
 * (WiFi connect / scan, Git sync, Draftbox, SD card format) are others;
 * they take the LVGL lock (draftling_lvgl_port_lock()) before touching
 * the UI. Here the main thread runs the LVGL loop and every task is a
 * detached thread, so the same code runs with the same concurrency.
 *
 * Ticks are milliseconds. A task handle carries the task's
 * notification counter (ulTaskNotifyTake() / xTaskNotifyGive(), used
 * by the Git sync worker). Critical sections are one process-wide
 * recursive mutex.
 */

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

static const char *TAG = "FreeRTOS";

#define MIN_STACK (1024u * 1024u)

struct linux_task {
    TaskFunction_t fn;
    void *arg;
    char name[24];
    pthread_mutex_t mu;
    pthread_cond_t cv;
    uint32_t notify;
};

static __thread struct linux_task *t_self;
static struct linux_task s_main_task = {
    .name = "main",
    .mu = PTHREAD_MUTEX_INITIALIZER,
    .cv = PTHREAD_COND_INITIALIZER,
};

static pthread_mutex_t s_critical;
static pthread_once_t s_critical_once = PTHREAD_ONCE_INIT;

static void critical_init(void)
{
    pthread_mutexattr_t a;
    pthread_mutexattr_init(&a);
    pthread_mutexattr_settype(&a, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&s_critical, &a);
    pthread_mutexattr_destroy(&a);
}

void linux_critical_enter(void)
{
    pthread_once(&s_critical_once, critical_init);
    pthread_mutex_lock(&s_critical);
}

void linux_critical_exit(void)
{
    pthread_mutex_unlock(&s_critical);
}

/* Absolute CLOCK_REALTIME deadline wait ms from now (for the
 * pthread_*_timedwait calls, which take that clock). */
static void deadline(struct timespec *ts, TickType_t wait)
{
    clock_gettime(CLOCK_REALTIME, ts);
    ts->tv_sec += wait / 1000;
    ts->tv_nsec += (long)(wait % 1000) * 1000000L;
    if (ts->tv_nsec >= 1000000000L) {
        ts->tv_sec++;
        ts->tv_nsec -= 1000000000L;
    }
}

static void *task_main(void *p)
{
    struct linux_task *t = (struct linux_task *)p;
    t_self = t;
    t->fn(t->arg);
    /* A task that returns instead of calling vTaskDelete(NULL). The
     * handle stays allocated: someone may still notify it. */
    return NULL;
}

static struct linux_task *task_start(TaskFunction_t fn, const char *name,
                                     uint32_t stack, void *arg)
{
    struct linux_task *t = (struct linux_task *)calloc(1, sizeof(*t));
    if (!t) return NULL;
    t->fn = fn;
    t->arg = arg;
    snprintf(t->name, sizeof(t->name), "%s", name ? name : "task");
    pthread_mutex_init(&t->mu, NULL);
    pthread_cond_init(&t->cv, NULL);

    pthread_attr_t a;
    pthread_attr_init(&a);
    pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);
    size_t sz = stack < MIN_STACK ? MIN_STACK : stack;
    pthread_attr_setstacksize(&a, sz);
    pthread_t th;
    int rc = pthread_create(&th, &a, task_main, t);
    pthread_attr_destroy(&a);
    if (rc != 0) {
        ESP_LOGE(TAG, "Cannot start task %s: %s", t->name, strerror(rc));
        pthread_cond_destroy(&t->cv);
        pthread_mutex_destroy(&t->mu);
        free(t);
        return NULL;
    }
#if defined(__linux__)
    {
        char n[16];
        snprintf(n, sizeof(n), "%s", t->name);
        pthread_setname_np(th, n);
    }
#endif
    return t;
}

BaseType_t xTaskCreatePinnedToCore(TaskFunction_t fn, const char *name,
                                   uint32_t stack, void *arg, UBaseType_t prio,
                                   TaskHandle_t *out, BaseType_t core)
{
    (void)prio; (void)core;
    struct linux_task *t = task_start(fn, name, stack, arg);
    if (out) *out = t;
    return t ? pdPASS : pdFAIL;
}

TaskHandle_t xTaskCreateStaticPinnedToCore(TaskFunction_t fn, const char *name,
                                           uint32_t stack, void *arg, UBaseType_t prio,
                                           StackType_t *stack_buf, StaticTask_t *tcb,
                                           BaseType_t core)
{
    (void)prio; (void)stack_buf; (void)tcb; (void)core;
    return task_start(fn, name, stack, arg);
}

void vTaskDelete(TaskHandle_t t)
{
    if (t == NULL || t == t_self) {
        if (t_self && t_self != &s_main_task) pthread_exit(NULL);
        ESP_LOGE(TAG, "vTaskDelete(NULL) on the main thread ignored");
        return;
    }
    ESP_LOGW(TAG, "vTaskDelete() of another task is not supported");
}

void vTaskDelay(TickType_t ticks)
{
    struct timespec ts = { (time_t)(ticks / 1000), (long)(ticks % 1000) * 1000000L };
    while (nanosleep(&ts, &ts) != 0 && errno == EINTR) { }
}

TickType_t xTaskGetTickCount(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (TickType_t)((uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u);
}

TaskHandle_t xTaskGetCurrentTaskHandle(void)
{
    return t_self ? t_self : &s_main_task;
}

uint32_t ulTaskNotifyTake(BaseType_t clear_on_exit, TickType_t wait)
{
    struct linux_task *t = xTaskGetCurrentTaskHandle();
    struct timespec ts;
    if (wait != portMAX_DELAY) deadline(&ts, wait);
    pthread_mutex_lock(&t->mu);
    while (t->notify == 0) {
        if (wait == 0) break;
        if (wait == portMAX_DELAY) {
            pthread_cond_wait(&t->cv, &t->mu);
        } else if (pthread_cond_timedwait(&t->cv, &t->mu, &ts) == ETIMEDOUT) {
            break;
        }
    }
    uint32_t v = t->notify;
    if (v) t->notify = clear_on_exit ? 0 : v - 1;
    pthread_mutex_unlock(&t->mu);
    return v;
}

BaseType_t xTaskNotifyGive(TaskHandle_t t)
{
    if (!t) return pdFAIL;
    pthread_mutex_lock(&t->mu);
    t->notify++;
    pthread_cond_signal(&t->cv);
    pthread_mutex_unlock(&t->mu);
    return pdPASS;
}

/* ---- queues -------------------------------------------------------------- */

struct linux_queue {
    pthread_mutex_t mu;
    pthread_cond_t not_empty, not_full;
    UBaseType_t len, size, head, count;
    uint8_t *buf;
};

QueueHandle_t xQueueCreate(UBaseType_t len, UBaseType_t item_size)
{
    struct linux_queue *q = (struct linux_queue *)calloc(1, sizeof(*q));
    if (!q) return NULL;
    q->buf = (uint8_t *)malloc((size_t)len * item_size);
    if (!q->buf) {
        free(q);
        return NULL;
    }
    q->len = len;
    q->size = item_size;
    pthread_mutex_init(&q->mu, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);
    return q;
}

/* Wait on cv until pred() holds or wait runs out; mu is held. */
static bool queue_wait(struct linux_queue *q, pthread_cond_t *cv, bool want_space,
                       TickType_t wait)
{
    struct timespec ts;
    if (wait != portMAX_DELAY) deadline(&ts, wait);
    for (;;) {
        bool ok = want_space ? q->count < q->len : q->count > 0;
        if (ok) return true;
        if (wait == 0) return false;
        if (wait == portMAX_DELAY) {
            pthread_cond_wait(cv, &q->mu);
        } else if (pthread_cond_timedwait(cv, &q->mu, &ts) == ETIMEDOUT) {
            return want_space ? q->count < q->len : q->count > 0;
        }
    }
}

BaseType_t xQueueSend(QueueHandle_t q, const void *item, TickType_t wait)
{
    if (!q) return pdFALSE;
    pthread_mutex_lock(&q->mu);
    if (!queue_wait(q, &q->not_full, true, wait)) {
        pthread_mutex_unlock(&q->mu);
        return pdFALSE;
    }
    UBaseType_t tail = (q->head + q->count) % q->len;
    memcpy(q->buf + (size_t)tail * q->size, item, q->size);
    q->count++;
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->mu);
    return pdTRUE;
}

BaseType_t xQueueReceive(QueueHandle_t q, void *item, TickType_t wait)
{
    if (!q) return pdFALSE;
    pthread_mutex_lock(&q->mu);
    if (!queue_wait(q, &q->not_empty, false, wait)) {
        pthread_mutex_unlock(&q->mu);
        return pdFALSE;
    }
    memcpy(item, q->buf + (size_t)q->head * q->size, q->size);
    q->head = (q->head + 1) % q->len;
    q->count--;
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->mu);
    return pdTRUE;
}

BaseType_t xQueueReset(QueueHandle_t q)
{
    if (!q) return pdFAIL;
    pthread_mutex_lock(&q->mu);
    q->head = q->count = 0;
    pthread_cond_broadcast(&q->not_full);
    pthread_mutex_unlock(&q->mu);
    return pdPASS;
}
