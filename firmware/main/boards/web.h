#pragma once
/* ----- Web browser (WebAssembly application) -----
 *
 * The editor as a web application, built to WebAssembly by the
 * Makefile in web_client/ (not by idf.py): a canvas filling the
 * browser window, the computer's own keyboard, the browser's
 * origin-private file system in place of the SD card, the browser's
 * network connection and the system clipboard. There are no pins:
 * web_client/src/ (and the Linux program's stand-ins it shares) stand
 * in for the hardware. The screen follows the window's size
 * (CONFIG_DRAFTLING_DISPLAY_RESIZABLE), so the Kconfig panel size is
 * only the initial size.
 *
 * Included by main/app_config.h when CONFIG_DRAFTLING_MODEL_WEB is
 * selected. See web_client/README.md.
 */

#define BOARD_NAME          "Web browser"

/* No wake button: a key or click turns the screen on again. */
#define WAKEUP_GPIO_NUM     -1

/* No battery. */
#define BATT_ADC_PIN        -1
#define BATT_EN_PIN         -1
#define BATT_DIVIDER        2
