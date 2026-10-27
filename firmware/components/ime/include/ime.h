#pragma once

/*
 * Input methods of the Vietnamese, Korean, Japanese and Chinese
 * language packs.
 *
 *   KB_IME_TELEX   Vietnamese Telex (aa -> a-circumflex, s/f/r/x/j tones)
 *   KB_IME_HANGUL  Korean 2-set: jamo composed into Hangul syllables
 *   KB_IME_KANA    Japanese: romaji -> hiragana, Space converts to kanji
 *   KB_IME_PINYIN  Chinese: pinyin letters -> hanzi candidates
 *
 * The text being composed lives in the document itself, right before
 * the cursor; the input method edits it through the host callbacks
 * below and remembers what it inserted. If the text before the cursor
 * no longer matches (the cursor moved, the user clicked elsewhere),
 * the composition is silently dropped and the text stays as it is.
 * Candidate lists are shown through ime_status().
 *
 * The engines are plain C with no ESP-IDF dependency apart from the
 * embedded dictionaries, so they can be tested on a host.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "kb_layout.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    /* Copy up to `max` bytes that immediately precede the cursor into
     * buf, stopping at a line break (which is not copied). Returns the
     * number of bytes copied; they are the last bytes before the
     * cursor, in text order. */
    size_t (*peek_before)(char *buf, size_t max);
    /* Replace the `del` bytes before the cursor with `text`; the cursor
     * ends up after the inserted text. Returns false when the document
     * is full (nothing changed then). */
    bool (*replace_before)(size_t del, const char *text);
} ime_host_t;

/* Dictionary blobs (DIME format, see tools/gen_ime_dicts.py). The
 * firmware passes the embedded blobs; NULL disables conversion. */
void ime_init(const ime_host_t *host, const uint8_t *ja_dict, const uint8_t *zh_dict);

/* The dictionaries compiled into the firmware (NULL when the layout is
 * not enabled), for ime_init(). */
const uint8_t *ime_builtin_ja_dict(void);
const uint8_t *ime_builtin_zh_dict(void);

/* Handle a key press for input method `kind`. `text` is what the
 * active layout produced for the key (kb_layout_translate()), or NULL.
 * Keys with Ctrl, Alt or GUI held must not be passed in (call
 * ime_commit() for them instead). Returns true when the key was
 * consumed; otherwise the caller handles it as usual (the composition
 * has then already been finished). */
bool ime_handle_key(kb_ime_t kind, uint8_t keycode, uint8_t modifier, const char *text);

/* Finish the current composition, keeping the text as it is shown.
 * Call before anything the input method does not see: shortcuts,
 * clicks, pane or document switches, layout changes. */
void ime_commit(void);

/* True while a composition or conversion is open. */
bool ime_composing(void);

/* Status-bar text while composing or choosing a candidate, else NULL.
 * The pointer stays valid until the next ime_* call. */
const char *ime_status(void);

/* Width of the status bar in monospace cells (a CJK character takes
 * two); candidate pages are cut to fit. Default 40. */
void ime_set_status_cells(int cells);

/* Display width in monospace cells of a UTF-8 string, counting East
 * Asian wide characters twice. */
int ime_text_cells(const char *s, size_t len);

#ifdef __cplusplus
}
#endif
