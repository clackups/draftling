/*
 * Fix for right-aligned right-to-left text in LVGL 9 (still present in
 * LVGL master as of 9.6).
 *
 * lv_draw_label() and lv_label_get_letter_pos() / _get_letter_on()
 * right-align a row with lv_text_get_line_width(), which leaves out
 * the row's trailing ("hanging") spaces: the row is placed as if they
 * were not there. That suits left-to-right text, where those spaces
 * sit at the right end and may hang over the edge. In a right-to-left
 * row the bidi reordering puts them at the LEFT end, before the first
 * drawn glyph, so the whole row is pushed right by their width and its
 * last letters land past the right edge. In the editor this hit every
 * Hebrew line just after Space was typed, and every soft-wrapped row of
 * a Hebrew paragraph (the space at a wrap point stays on the row).
 *
 * The linker wraps lv_text_get_line_width() (-Wl,--wrap, see
 * CMakeLists.txt). For a row that reads right to left the wrapper
 * counts the trailing spaces as well, so the row is placed with its
 * last letter at the right edge and the spaces to its left. Calls from
 * inside lv_text.c itself (lv_text_get_size()) are not wrapped; they
 * only size content-width labels.
 *
 * The row's direction uses LVGL's own rule (lv_bidi_detect_base_dir():
 * right to left if any strong right-to-left character follows), applied
 * from the row's start to the end of the label text. A Latin-only row
 * after the last Hebrew word of a paragraph therefore still counts as
 * left to right; it keeps LVGL's behaviour, off by its trailing space.
 */

#include "lvgl.h"
#include "src/misc/lv_text_private.h"
#include "src/misc/lv_bidi_private.h"

int32_t __real_lv_text_get_line_width(const char *txt, uint32_t length,
                                      const lv_font_t *font,
                                      const lv_text_attributes_t *attributes);

int32_t __wrap_lv_text_get_line_width(const char *txt, uint32_t length,
                                      const lv_font_t *font,
                                      const lv_text_attributes_t *attributes)
{
#if LV_USE_BIDI
    if (txt && font) {
        uint32_t len = 0;
        while (len < length && txt[len] != '\0') len++;
        /* Only rows ending in a space differ; '\n' / '\r' are not drawn. */
        uint32_t end = len;
        while (end > 0 && (txt[end - 1] == '\n' || txt[end - 1] == '\r')) end--;
        if (end > 0 && txt[end - 1] == ' ' &&
            lv_bidi_detect_base_dir(txt) == LV_BASE_DIR_RTL) {
            return lv_text_get_width(txt, end, font, attributes);
        }
    }
#endif
    return __real_lv_text_get_line_width(txt, length, font, attributes);
}
