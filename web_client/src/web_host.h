/*
 * Web client: what its parts share, besides the Linux program's
 * linux_host.h (whose LVGL port, keyboard, clipboard, standby, NVS and
 * FreeRTOS stand-ins it reuses).
 */
#pragma once

#include "standby.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Events the page (web/client.js) queues with web_input(type, a, b, c);
 * keep the numbers in step with WEB_EV there. */
enum {
    WEB_EV_KEY = 1,         /* a = Linux key code, b = pressed, c = held modifiers (LINUX_MOD_*) */
    WEB_EV_POINTER = 2,     /* a, b = canvas pixels, c = primary button down */
    WEB_EV_WHEEL = 3,       /* a = steps, > 0 down */
    WEB_EV_RESIZE = 4,      /* a, b = canvas size in device pixels */
    WEB_EV_RELEASE_ALL = 5, /* the page lost the keyboard focus */
    WEB_EV_PASTE = 6,       /* a = malloc()ed UTF-8 text (0: none), b = its length */
    WEB_EV_SAVE = 7,        /* the page is being hidden: save every document */
    WEB_EV_DAMAGE = 8,      /* the canvas was cleared: draw it all again */
    WEB_EV_FILES = 9,       /* a = a Files panel request (files_web.c) */
    WEB_EV_CORNERS = 10,    /* a = EDITOR_CORNER_* bits of the canvas at rounded screen
                               corners, b = device pixels per CSS pixel x 1000 */
};

/* Queue an event (the page's thread; io_web.c). */
void web_input(int type, int a, int b, int c);

/* Run a Files panel request on the LVGL loop (files_web.c). */
void web_files_run(void *req);

/* The last navigator.storage.estimate() the page reported (0: none). */
void web_storage_get(double *quota, double *usage);

/* standby_linux.c: the pre-sleep hook (save every open document). */
extern standby_pre_sleep_cb_t linux_pre_sleep_cb;

#ifdef __cplusplus
}
#endif
