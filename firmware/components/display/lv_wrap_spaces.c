/*
 * Spaces at a word-wrap point in LVGL 9 (still the behaviour of LVGL
 * master as of 9.6).
 *
 * lv_text_get_next_line() lets the spaces after the last word of a row
 * hang over the right edge, and at an automatic wrap it also skips
 * every space that follows, so the next row starts with the next word.
 * In the editor that made Space look dead at the end of a full row: the
 * spaces went past the edge, the cursor with them (off screen), and the
 * next letter appeared at the start of the next row as if no space had
 * been typed.
 *
 * The linker wraps lv_text_get_next_line() (-Wl,--wrap, see
 * CMakeLists.txt). When a row returned by LVGL ends in a run of spaces
 * (an automatic wrap, or the end of the text), the wrapper keeps the
 * spaces that fit within the width, and at least one -- the space the
 * row breaks at, which may hang as before -- and ends the row there,
 * so the rest start the next row, where they are visible. Rows that end
 * at a '\n' / '\r' and rows without trailing spaces are unchanged.
 *
 * lv_text_get_size_attributes(), which sizes a label, calls
 * lv_text_get_next_line() inside lv_text.c, where --wrap does not
 * reach, so it is wrapped too and redone here with the wrapped row
 * breaking (same code as LVGL's); otherwise a label whose text ends in
 * spaces carried to a new row stays one row short.
 */

#include "lvgl.h"
#include "src/misc/lv_text_private.h"

uint32_t __real_lv_text_get_next_line(const char *txt, uint32_t len,
                                      const lv_font_t *font, int32_t *used_width,
                                      lv_text_attributes_t *attributes);

uint32_t __wrap_lv_text_get_next_line(const char *txt, uint32_t len,
                                      const lv_font_t *font, int32_t *used_width,
                                      lv_text_attributes_t *attributes)
{
    uint32_t n = __real_lv_text_get_next_line(txt, len, font, used_width, attributes);
    if (!txt || !font || !attributes || n == 0) return n;
    if (attributes->text_flags & (LV_TEXT_FLAG_EXPAND | LV_TEXT_FLAG_FIT)) return n;
    if (txt[n - 1] != ' ') return n;

    uint32_t k = n;                    /* start of the trailing space run */
    while (k > 0 && txt[k - 1] == ' ') k--;
    uint32_t spaces = n - k;
    if (spaces < 2) return n;

    int32_t max_w = attributes->max_width;
    int32_t sw = lv_font_get_glyph_width(font, ' ', ' ') + attributes->letter_space;
    int32_t w = k ? lv_text_get_width(txt, k, font, attributes) + attributes->letter_space : 0;
    if (sw <= 0) return n;

    uint32_t keep = 1;
    while (keep < spaces && w + (int32_t)(keep + 1) * sw <= max_w) keep++;
    if (keep >= spaces) return n;

    uint32_t end = k + keep;
    if (used_width) *used_width = lv_text_get_width(txt, end, font, attributes);
    return end;
}

void __wrap_lv_text_get_size_attributes(lv_point_t *size_res, const char *text,
                                        const lv_font_t *font,
                                        lv_text_attributes_t *attributes)
{
    uint32_t line_start = 0;
    uint32_t new_line_start = 0;
    size_res->x = 0;
    size_res->y = 0;
    if (!attributes || !font || !text) return;

    int32_t letter_height = lv_font_get_line_height(font);
    if (attributes->text_flags & LV_TEXT_FLAG_EXPAND) {
        attributes->max_width = LV_COORD_MAX;
    }

    while (text[line_start] != '\0') {
        new_line_start += __wrap_lv_text_get_next_line(
            &text[line_start], LV_TEXT_LEN_MAX, font, NULL, attributes);
        if ((unsigned long)size_res->y + (unsigned long)letter_height +
                (unsigned long)attributes->line_space > LV_MAX_OF(int32_t)) {
            return;
        }
        size_res->y += letter_height + attributes->line_space;
        int32_t w = lv_text_get_line_width(&text[line_start],
                                           new_line_start - line_start,
                                           font, attributes);
        size_res->x = LV_MAX(w, size_res->x);
        line_start = new_line_start;
    }

    /* One line taller if the last character is '\n' or '\r'. */
    if (line_start != 0 &&
        (text[line_start - 1] == '\n' || text[line_start - 1] == '\r')) {
        size_res->y += letter_height + attributes->line_space;
    }

    if (size_res->y == 0) size_res->y = letter_height;
    else size_res->y -= attributes->line_space;
}
