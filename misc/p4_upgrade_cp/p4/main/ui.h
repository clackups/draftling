/*
 * Upgrade progress on the board's screen (LVGL; board.h starts the
 * display and provides its lock). All functions take the display lock themselves and may be
 * called from any task.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UI_STATE_WORKING,
    UI_STATE_OK,
    UI_STATE_ERROR,
} ui_state_t;

/* Starts the display and builds the screen. */
void ui_init(const char *title);

/* Large status line (current phase). */
void ui_phase(ui_state_t state, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

/* Smaller lines under it: what was found on the C6. */
void ui_detail(int line, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

/* Progress bar, 0..100; -1 hides it. */
void ui_progress(int percent);

/* Appends a line to the on-screen log (also printed to the console). */
void ui_log(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

/* Shows two buttons and waits for a tap; true for `yes`. Returns
 * `dflt` after timeout_ms (0 = wait forever). */
bool ui_ask(const char *yes, const char *no, uint32_t timeout_ms, bool dflt);

#ifdef __cplusplus
}
#endif
