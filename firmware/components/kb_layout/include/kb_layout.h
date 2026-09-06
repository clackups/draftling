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

/* Cycle to the next active layout (see kb_layout_set_active()) and
 * return its id. Layouts excluded from the active set are skipped. */
kb_layout_id_t kb_layout_next(void);

/* True when the active layout produces right-to-left script (Hebrew) */
bool kb_layout_is_rtl(void);

/* Resolves a HID keycode to the lowercase ASCII letter that Ctrl-combo
 * keyboard shortcuts should match against, given the active layout:
 *  - Latin layouts (US, DE, FR, ...) resolve to whichever letter that
 *    physical key actually produces on the national layout's normal
 *    (unshifted) layer -- not limited to the classic 26-key US letter
 *    block, since a layout can move a Latin letter onto a key that is
 *    a symbol on US. E.g. Ctrl+Z lands on the key printed Z on a
 *    German (QWERTZ) keyboard, not the US Z position; French AZERTY
 *    puts "m" on the semicolon key, so Ctrl+M on a French keyboard is
 *    that key, not the US M position. A key that produces a
 *    non-letter under this layout has no Ctrl-shortcut letter.
 *  - Non-Latin layouts (Ukrainian, Hebrew, ...) fall back to the
 *    classic 26-key US physical letter block (0x04..0x1D), since
 *    there is no national Latin letter to remap the shortcut to --
 *    Ctrl+N keeps working exactly as if the US layout were active.
 * Returns 0 when the keycode has no shortcut letter under these rules. */
char kb_layout_shortcut_char(uint8_t keycode);

/* ------------------------------------------------------------------
 * Active layout set -- the "Active language layouts" user setting.
 *
 * A layout must be compiled in (Kconfig) before it can be part of the
 * active set. kb_layout_next() (Ctrl+L / Win+Space) only cycles through
 * layouts in this set, so users who only need one or two layouts do
 * not have to page through every compiled-in layout to get back to
 * the one they want.
 * ------------------------------------------------------------------ */

/* True when a layout is in the active (rotated) set. */
bool kb_layout_is_active(kb_layout_id_t layout);

/* Number of layouts currently in the active set. Always >= 1. */
int kb_layout_active_count(void);

/* Add or remove a layout from the active set. Refuses to remove the
 * last remaining active layout (returns false, no change) so the
 * set is never empty. If the currently selected layout is removed,
 * switches immediately to the next active layout. */
bool kb_layout_set_active(kb_layout_id_t layout, bool active);

/* Bitmask of the active set (bit N = layout id N), for persistence. */
uint32_t kb_layout_get_active_mask(void);

/* Restores an active-set bitmask (e.g. loaded from NVS). Bits for
 * layouts that are not compiled in are ignored. If the result would
 * be empty, falls back to the built-in default set. */
void kb_layout_set_active_mask(uint32_t mask);

#ifdef __cplusplus
}
#endif
