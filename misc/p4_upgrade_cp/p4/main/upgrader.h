/*
 * C6 firmware upgrade task (see upgrader.c).
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Starts the upgrade in its own task; progress goes to the screen. */
void upgrader_start(void);

#ifdef __cplusplus
}
#endif
