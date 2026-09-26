/* Plain-text editing behaviour -- see editor_format.h. Every line is a
 * paragraph (or empty) with no markers, spans or decorations: the text
 * is shown exactly as typed. */

#include <cstring>
#include "editor_format.h"

static void txt_scan_init(fmt_scan_t *sc)
{
    (void)sc;
}

static void txt_parse(fmt_scan_t *sc, const char *lt, size_t ll, md_line_info_t *mi)
{
    (void)sc;
    if (!mi) return;
    mi->type = ll ? MD_LINE_PARAGRAPH : MD_LINE_EMPTY;
    mi->content = lt;
    mi->content_len = ll;
    mi->indent_level = 0;
    mi->span_count = 0;
}

static void txt_mark(const md_line_info_t *mi, bool reveal, fmt_marks_t *m)
{
    (void)mi;
    (void)reveal;
    (void)m;
}

static void txt_layout(lv_obj_t *label, md_line_type_t type, int32_t w)
{
    (void)label;
    (void)type;
    (void)w;
}

/* Tabs still expand to four display cells, which the fast path's
 * one-cell-per-character math does not model. */
static bool txt_line_is_plain(const char *lt, size_t ll)
{
    return memchr(lt, '\t', ll) == NULL;
}

static bool txt_enter(bool shift)
{
    (void)shift;
    return false;
}

static bool txt_tab(bool append_only)
{
    (void)append_only;
    return false;
}

static const fmt_help_row_t txt_help[] = {
    { "(none)", "Plain text has no formatting: the text is shown exactly as typed" },
    { NULL, NULL },
};

extern const editor_format_t text_editor_format = {
    txt_scan_init,
    txt_parse,
    txt_mark,
    txt_layout,
    txt_line_is_plain,
    txt_enter,
    txt_tab,
    "Plain text",
    txt_help,
};
