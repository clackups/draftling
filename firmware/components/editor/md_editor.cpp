/* Markdown editing behaviour -- see editor_format.h. */

#include <cstring>
#include "editor_format.h"

static void md_scan_init(fmt_scan_t *sc)
{
    sc->in_code = false;
}

static void md_parse(fmt_scan_t *sc, const char *lt, size_t ll, md_line_info_t *mi)
{
    if (!mi) {
        if (md_is_code_fence(lt, ll)) sc->in_code = !sc->in_code;
        return;
    }
    md_parse_line(lt, ll, mi, sc->in_code);
    if (mi->type == MD_LINE_CODE_FENCE) sc->in_code = !sc->in_code;
}

static void hide_range(fmt_marks_t *m, size_t from, size_t to)
{
    if (to > from) memset(m->hide + from, 1, to - from);
}

static void md_mark(const md_line_info_t *mi, bool reveal, fmt_marks_t *m)
{
    switch (mi->type) {
    case MD_LINE_H1: case MD_LINE_H2: case MD_LINE_H3: case MD_LINE_H4:
        if (!reveal) hide_range(m, 0, m->content_off);
        for (size_t b = m->content_off; b < m->content_end; b++) m->rflags[b] |= DECO_BOLD;
        break;
    case MD_LINE_BLOCKQUOTE:
    case MD_LINE_CODE_FENCE:
        /* The quote bar / code box already mark these lines. */
        if (!reveal) hide_range(m, 0, m->content_off);
        break;
    case MD_LINE_BULLET:
        /* The "-" / "*" / "+" marker sits two bytes before the content;
         * it is replaced by a drawn bullet even on the cursor line (1:1,
         * so the mapping is unaffected). */
        if (m->content_off >= 2) m->rflags[m->content_off - 2] |= DECO_BULLET;
        m->bullet_level = (uint8_t)(mi->indent_level > 255 ? 255 : mi->indent_level);
        break;
    case MD_LINE_HR:
        if (!reveal) {
            hide_range(m, 0, m->ll);
            m->hr = true;
        }
        break;
    default:
        break;
    }
}

static void md_layout(lv_obj_t *label, md_line_type_t type, int32_t w)
{
    (void)label;
    (void)type;
    (void)w;
}

/* Single-line classification (no surrounding code-fence context):
 * only plain paragraphs and empty lines without inline spans or tabs
 * display their raw text verbatim. */
static bool md_line_is_plain(const char *lt, size_t ll)
{
    md_line_info_t mi;
    md_parse_line(lt, ll, &mi, false);
    if (mi.type != MD_LINE_PARAGRAPH && mi.type != MD_LINE_EMPTY) return false;
    if (mi.span_count > 0) return false;
    return memchr(lt, '\t', ll) == NULL;
}

static bool md_enter(bool shift)
{
    (void)shift;
    return false;
}

static bool md_tab(bool append_only)
{
    (void)append_only;
    return false;
}

extern const editor_format_t md_editor_format = {
    md_scan_init,
    md_parse,
    md_mark,
    md_layout,
    md_line_is_plain,
    md_enter,
    md_tab,
};
