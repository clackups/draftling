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
    /* Added after HE so that the bits of the layouts above keep their
     * positions in the persisted "Active layouts" mask. */
#ifdef CONFIG_KB_LAYOUT_ENABLE_DE_CH
    KB_LAYOUT_DE_CH,
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_FR_CH
    KB_LAYOUT_FR_CH,
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_IT
    KB_LAYOUT_IT,
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_PT
    KB_LAYOUT_PT,
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_ES
    KB_LAYOUT_ES,
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_GB
    KB_LAYOUT_GB,
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_SE_FI
    KB_LAYOUT_SE_FI,
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_NO
    KB_LAYOUT_NO,
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_DK
    KB_LAYOUT_DK,
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_PL
    KB_LAYOUT_PL,
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_CZ
    KB_LAYOUT_CZ,
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_SK
    KB_LAYOUT_SK,
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_HU
    KB_LAYOUT_HU,
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_RO
    KB_LAYOUT_RO,
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_HR
    KB_LAYOUT_HR,
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_TR
    KB_LAYOUT_TR,
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_FR_CA
    KB_LAYOUT_FR_CA,
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_VI
    KB_LAYOUT_VI,
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_KO
    KB_LAYOUT_KO,
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_JA
    KB_LAYOUT_JA,
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_ZH
    KB_LAYOUT_ZH,
#endif
    KB_LAYOUT_COUNT,
} kb_layout_id_t;

/* Input method attached to a layout. Layouts with an input method
 * produce keystrokes that the editor hands to the `ime` component,
 * which composes them into the final text (Vietnamese tone marks,
 * Hangul syllables, kana / kanji, hanzi). */
typedef enum {
    KB_IME_NONE = 0,
    KB_IME_TELEX,      /* Vietnamese Telex */
    KB_IME_HANGUL,     /* Korean 2-set (Dubeolsik) composition */
    KB_IME_KANA,       /* Japanese romaji -> kana -> kanji */
    KB_IME_PINYIN,     /* Chinese pinyin -> hanzi */
} kb_ime_t;

/* Input method of the active layout (KB_IME_NONE for plain layouts). */
kb_ime_t kb_layout_ime(void);

/* Returns the UTF-8 string for a given HID keycode + modifier.
 * The returned pointer is valid until the next call.
 * Returns NULL if the keycode does not produce a character.
 * Caps Lock (kb_layout_set_caps_lock()) is applied here.
 *
 * Dead keys (the accent keys of the European layouts) are handled
 * here too, so this call has state: a dead key returns NULL and is
 * remembered; the next character key returns the accented letter
 * ("a" after the acute dead key -> a-acute), the accent itself after
 * Space or the same dead key, or the accent followed by the
 * character when the two do not combine. A key that produces no
 * character drops a pending accent. Call it once per key press. */
const char *kb_layout_translate(uint8_t keycode, uint8_t modifier);

/* True while a dead key waits for the next character (the last
 * kb_layout_translate() call returned NULL for a dead key). */
bool kb_layout_dead_key_pending(void);

/* True when the active layout has an AltGr layer. Right Alt is then
 * AltGr (it types characters) and must not act as Alt for Alt+letter
 * shortcuts; on other layouts either Alt key works. */
bool kb_layout_has_altgr(void);

/* HID keycode of the Caps Lock key. */
#define KB_KEYCODE_CAPS_LOCK 0x39

/* Caps Lock state, as on a PC keyboard: while on, Shift is inverted on
 * letter keys only -- keys whose Shift output is an uppercase letter
 * (A-Z, accented Latin, Cyrillic). Digits, symbols and AltGr are not
 * affected. On the Hebrew layout, whose Shift layer is Latin capitals
 * (SI 1452), Caps Lock therefore types Latin capitals, like Windows.
 * The state is shared by all layouts and keyboards. */
void kb_layout_set_caps_lock(bool on);
bool kb_layout_get_caps_lock(void);

/* Get the active keyboard layout */
kb_layout_id_t kb_layout_get(void);

/* Get the display name for a layout (e.g. "US", "UA", "DE", "DE-CH") */
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
