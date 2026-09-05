#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "sdkconfig.h"

/*
 * Layout enum entries are defined only for layouts enabled in Kconfig.
 * KB_LAYOUT_COUNT always equals the number of enabled layouts.
 */
typedef enum {
#ifdef CONFIG_KB_LAYOUT_ENABLE_US
    KB_LAYOUT_US,
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_UA
    KB_LAYOUT_UA,
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_DE
    KB_LAYOUT_DE,
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_FR
    KB_LAYOUT_FR,
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_HE
    KB_LAYOUT_HE,
#endif
    KB_LAYOUT_COUNT,
} kb_layout_id_t;

/* Returns the UTF-8 string for a given HID keycode + modifier.
 * The returned pointer is valid until the next call.
 * Returns NULL if the keycode does not produce a character. */
const char *kb_layout_translate(uint8_t keycode, uint8_t modifier);

/* Set the active keyboard layout */
void kb_layout_set(kb_layout_id_t layout);

/* Get the active keyboard layout */
kb_layout_id_t kb_layout_get(void);

/* Get the display name for a layout (e.g. "US", "UA", "DE", "FR") */
const char *kb_layout_name(kb_layout_id_t layout);

/* Cycle to the next layout and return its id */
kb_layout_id_t kb_layout_next(void);

/* True when the active layout produces right-to-left script (Hebrew) */
bool kb_layout_is_rtl(void);

#ifdef __cplusplus
}
#endif
