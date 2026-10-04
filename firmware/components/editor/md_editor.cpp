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

/* Raw byte offset, within the line, of the cell that shows a hard line
 * break at the end of content [off, end): an unescaped trailing
 * backslash, or the last of two or more trailing spaces after some
 * text. -1 if the content does not end with one. */
static long hard_break_cell(const char *lt, size_t off, size_t end)
{
    size_t bs = 0;
    while (end - bs > off && lt[end - 1 - bs] == '\\') bs++;
    if (bs & 1) return (long)end - 1;
    size_t sp = 0;
    while (end - sp > off && lt[end - 1 - sp] == ' ') sp++;
    if (sp >= 2 && end - sp > off) return (long)end - 1;
    return -1;
}

static void md_mark(const md_line_info_t *mi, bool reveal, fmt_marks_t *m)
{
    /* A hard line break -- the next line continues the same paragraph
     * on a new row -- is shown as a return arrow in place of its
     * marker, even on the cursor line (1:1, like the bullet). */
    if (mi->type == MD_LINE_PARAGRAPH || mi->type == MD_LINE_BLOCKQUOTE ||
        mi->type == MD_LINE_BULLET || mi->type == MD_LINE_NUMBERED) {
        const char *lt = mi->content - m->content_off;
        long cell = hard_break_cell(lt, m->content_off, m->content_end);
        if (cell >= 0) m->rflags[cell] |= DECO_BREAK;
    }

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
 * only plain paragraphs and empty lines without inline spans, tabs or
 * a hard line break display their raw text verbatim. */
static bool md_line_is_plain(const char *lt, size_t ll)
{
    md_line_info_t mi;
    md_parse_line(lt, ll, &mi, false);
    if (mi.type != MD_LINE_PARAGRAPH && mi.type != MD_LINE_EMPTY) return false;
    if (mi.span_count > 0) return false;
    if (mi.type == MD_LINE_PARAGRAPH &&
        hard_break_cell(lt, (size_t)(mi.content - lt), ll) >= 0) return false;
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

static const fmt_help_row_t md_help[] = {
    { "# Title",      "Heading 1" },
    { "## .. ####",   "Headings 2 to 4" },
    { "**bold**",     "Bold (also __bold__)" },
    { "*italic*",     "Italic (also _italic_)" },
    { "***both***",   "Bold italic" },
    { "~~text~~",     "Strikethrough" },
    { "`code`",       "Inline code" },
    { "- item",       "Bullet list (also * or +); indent to nest" },
    { "1. item",      "Numbered list" },
    { "> text",       "Blockquote" },
    { "```",          "Start / end a code block" },
    { "---",          "Horizontal rule" },
    { "\\*",          "Backslash: type a marker literally" },
    { "text\\",       "Line break (also two spaces at the end)" },
    { NULL, NULL },
};

extern const editor_format_t md_editor_format = {
    md_scan_init,
    md_parse,
    md_mark,
    md_layout,
    md_line_is_plain,
    md_enter,
    md_tab,
    "Markdown formatting",
    md_help,
};
