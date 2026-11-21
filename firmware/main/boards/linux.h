#pragma once
/* ----- Linux (native application) -----
 *
 * The editor as a native Linux program, built by the Makefile in
 * linux/ (not by idf.py): a Wayland or X11 window, with the
 * computer's own keyboard, a folder on the local file system in place
 * of the SD card and the host's IP network. There are no pins: linux/src/ stands in for the hardware.
 * The window can be resized (CONFIG_DRAFTLING_DISPLAY_RESIZABLE), so
 * the Kconfig panel size is only the initial window size.
 *
 * Included by main/app_config.h when CONFIG_DRAFTLING_MODEL_LINUX is
 * selected. See linux/README.md.
 */

#define BOARD_NAME          "Linux"

/* No wake button: "Sleep now" (Ctrl+P) saves and quits. */
#define WAKEUP_GPIO_NUM     -1

/* No ADC: the battery comes from /sys/class/power_supply. */
#define BATT_ADC_PIN        -1
#define BATT_EN_PIN         -1
#define BATT_DIVIDER        2
