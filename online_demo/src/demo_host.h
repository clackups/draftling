/*
 * Online demo: calls from the firmware side into the web page
 * (web/demo.js, which installs the Module.demo* hooks), and the board
 * facts the stubs share.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Simulated SD card mount point, kept in the browser's IndexedDB. */
#define DEMO_SD_MOUNT "/sdcard"

/* Size of the LVGL canvas (after margins, orientation and 2x scale)
 * and whether the picture is turned 180 degrees. */
void demo_host_set_screen(int w, int h, int scale, bool upside_down);
/* Copy an RGB565 rectangle of the frame buffer to the page's canvas. */
void demo_host_present(const uint16_t *fb, int fb_w, int x, int y, int w, int h);
void demo_host_set_backlight(int percent);
/* Persist the browser storage and reload the page ("restart"). */
void demo_host_restart(void);
/* Persist the browser storage and show the "asleep" overlay; a key or
 * click reloads the page, as the wake button resets the board. */
void demo_host_sleep(void);
/* Persist the browser storage soon (after file writes). */
void demo_host_storage_dirty(void);

/* Stop running the firmware for good: used by esp_restart() and deep
 * sleep, which never return on the device. */
__attribute__((noreturn)) void demo_halt(void);

#ifdef __cplusplus
}
#endif
