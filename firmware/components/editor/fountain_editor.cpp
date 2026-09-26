/* Fountain screenplay editing behaviour -- see editor_format.h and,
 * for the line classifier, fountain_parser.h. */

#include <cctype>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include "editor_format.h"
#include "editor_ui.h"

/* ---- Line classification ---- */

/* Whether the line after lt[0, ll) is blank, as fountain_parse_line()
 * wants to know. An empty line that is the last one or holds the
 * cursor counts as not blank: it is the line being typed, so e.g. a
 * Character cue keeps its formatting while the user writes its first
 * line of Dialogue. */
static bool next_line_blank(const fmt_scan_t *sc, const char *lt, size_t ll)
{
    size_t p = (size_t)(lt - sc->flat) + ll;
    if (p >= sc->flat_len) return true;   /* no next line */
    p++;                                  /* skip the '\n' */
    size_t q = p;
    while (q < sc->flat_len && sc->flat[q] != '\n') q++;
    if (q == p) return !(q == sc->flat_len || sc->cursor == p);
    for (size_t i = p; i < q; i++) {
        if (sc->flat[i] != ' ' && sc->flat[i] != '\t') return false;
    }
    return true;
}

static void ftn_scan_init(fmt_scan_t *sc)
{
    ftn_state_init(&sc->ftn);
}

static void ftn_parse(fmt_scan_t *sc, const char *lt, size_t ll, md_line_info_t *mi)
{
    fountain_parse_line(lt, ll, next_line_blank(sc, lt, ll), &sc->ftn, mi);
}

static void ftn_mark(const md_line_info_t *mi, bool reveal, fmt_marks_t *m)
{
    switch (mi->type) {
    case MD_LINE_FTN_SCENE:
        for (size_t b = m->content_off; b < m->content_end; b++) m->rflags[b] |= DECO_BOLD;
        break;
    case MD_LINE_FTN_LYRIC:
    case MD_LINE_FTN_SYNOPSIS:
        for (size_t b = m->content_off; b < m->content_end; b++) m->rflags[b] |= DECO_ITALIC;
        break;
    case MD_LINE_FTN_ACTION:
    case MD_LINE_FTN_CHARACTER:
    case MD_LINE_FTN_DIALOGUE:
    case MD_LINE_FTN_PARENTHETICAL:
    case MD_LINE_FTN_TRANSITION:
    case MD_LINE_FTN_CENTERED:
        break;
    default:
        /* Sections (MD_LINE_H1..H3) and page breaks (MD_LINE_HR) look
         * like their Markdown counterparts. */
        md_editor_format.mark_line(mi, reveal, m);
        return;
    }
    /* Forcing characters ("!", ".", "@", ">", "~", "="), leading
     * indentation (ignored outside Action), the dual-dialogue "^" and
     * the centering "<" sit outside content; the element's layout
     * shows what they mean. */
    if (!reveal) {
        if (m->content_off) memset(m->hide, 1, m->content_off);
        if (m->ll > m->content_end) memset(m->hide + m->content_end, 1, m->ll - m->content_end);
    }
}

/* Screenplay page layout, scaled to the label width w. On a standard
 * 6-inch text column Dialogue (and Lyrics) run from 1.0" to 4.5",
 * Parentheticals from 1.6" to 4.0" and Character cues start at 2.2";
 * Transitions are flush right and centered text is centered. */
static void ftn_layout(lv_obj_t *label, md_line_type_t type, int32_t w)
{
    switch (type) {
    case MD_LINE_FTN_CHARACTER:
        lv_obj_set_style_pad_left(label, w * 37 / 100, 0);
        break;
    case MD_LINE_FTN_DIALOGUE:
    case MD_LINE_FTN_LYRIC:
        lv_obj_set_style_pad_left(label, w * 17 / 100, 0);
        lv_obj_set_style_pad_right(label, w * 25 / 100, 0);
        break;
    case MD_LINE_FTN_PARENTHETICAL:
        lv_obj_set_style_pad_left(label, w * 27 / 100, 0);
        lv_obj_set_style_pad_right(label, w * 33 / 100, 0);
        break;
    case MD_LINE_FTN_TRANSITION:
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_RIGHT, 0);
        break;
    case MD_LINE_FTN_CENTERED:
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
        break;
    default:
        break;
    }
}

/* Never: a Fountain line's element -- and so its indent and styling --
 * depends on the neighbouring lines, which typing can change. */
static bool ftn_line_is_plain(const char *lt, size_t ll)
{
    (void)lt;
    (void)ll;
    return false;
}

/* ---- Editing helpers ---- */

/* Run the classifier over lines 0..last of the active document (an
 * element depends on everything above it), calling cb for each. */
typedef void (*line_cb_t)(int idx, const md_line_info_t *mi, void *ctx);

static void walk_lines(int last, line_cb_t cb, void *ctx)
{
    static md_line_info_t mi;   /* UI task only; too big for the stack */
    fmt_scan_t sc;
    editor_format_scan_begin(&fountain_editor_format, &sc);
    int total = editor_get_line_count();
    if (last < 0 || last >= total) last = total - 1;
    for (int i = 0; i <= last; i++) {
        size_t ll;
        const char *lt = editor_get_line(i, &ll);
        ftn_parse(&sc, lt, ll, &mi);
        cb(i, &mi, ctx);
    }
}

static bool text_is_blank(const char *s, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        if (s[i] != ' ' && s[i] != '\t') return false;
    }
    return true;
}

/* The cursor line, if the cursor sits at its end. */
static bool cursor_at_line_end(int *line, const char **lt, size_t *ll)
{
    int col;
    editor_get_cursor_pos(line, &col);
    *lt = editor_get_line(*line, ll);
    int chars = 0;
    for (size_t i = 0; i < *ll; i++) {
        if (((*lt)[i] & 0xC0) != 0x80) chars++;
    }
    return col == chars;
}

typedef struct {
    int            line;
    md_line_type_t type;
} type_query_t;

static void type_cb(int idx, const md_line_info_t *mi, void *ctx)
{
    type_query_t *q = (type_query_t *)ctx;
    if (idx == q->line) q->type = mi->type;
}

/* Enter at the end of a Scene Heading or Transition also inserts the
 * blank line that has to follow it, the way Slugline adds "the correct
 * number of carriage returns". Shift+Enter inserts a single newline. */
static bool ftn_enter(bool shift)
{
    if (shift || editor_selection_active()) return false;
    int line;
    const char *lt;
    size_t ll;
    if (!cursor_at_line_end(&line, &lt, &ll) || text_is_blank(lt, ll)) return false;
    if (line + 1 < editor_get_line_count()) {
        size_t nl;
        const char *nt = editor_get_line(line + 1, &nl);
        if (text_is_blank(nt, nl)) return false;   /* blank line already there */
    }
    type_query_t q = { line, MD_LINE_EMPTY };
    walk_lines(line, type_cb, &q);
    if (q.type != MD_LINE_FTN_SCENE && q.type != MD_LINE_FTN_TRANSITION) return false;
    if (!editor_insert_text("\n\n", 2))
        editor_ui_set_status("Buffer full -- increase editor size in menuconfig");
    return true;
}

/* ---- Tab completion ----
 *
 * As in Slugline, Tab offers names already used in the script. On an
 * empty line it offers the Character cues, most recent first except
 * that the one who spoke before the last speaker comes first (the
 * usual back-and-forth of a conversation). On a partly typed line it
 * offers the Character names, Scene Headings and Transitions -- plus
 * the standard INT. / EXT. prefixes -- that start with the typed text,
 * ignoring case. Each further Tab replaces the line with the next
 * candidate, ending with the originally typed text; any other key
 * keeps the current one. */
static std::vector<std::string> s_tab_cands;
static size_t s_tab_idx;
static size_t s_tab_line_start;

typedef std::vector<std::pair<int, std::string>> indexed_names_t;

typedef struct {
    int             cur_line;
    indexed_names_t names;    /* Character cues */
    indexed_names_t others;   /* Scene Headings, Transitions */
} collect_t;

static void collect_cb(int idx, const md_line_info_t *mi, void *ctx)
{
    collect_t *c = (collect_t *)ctx;
    if (idx == c->cur_line) return;
    size_t n = mi->content_len;
    if (mi->type == MD_LINE_FTN_CHARACTER) {
        /* The name without its extension: "MOM (O.S.)" -> "MOM". */
        const char *paren = (const char *)memchr(mi->content, '(', n);
        if (paren) n = (size_t)(paren - mi->content);
    } else if (mi->type != MD_LINE_FTN_SCENE && mi->type != MD_LINE_FTN_TRANSITION) {
        return;
    }
    while (n > 0 && (mi->content[n - 1] == ' ' || mi->content[n - 1] == '\t')) n--;
    if (n == 0) return;
    std::string v(mi->content, n);
    if (mi->type == MD_LINE_FTN_CHARACTER) c->names.emplace_back(idx, v);
    else                                   c->others.emplace_back(idx, v);
}

/* Append v's names to out, nearest-before-the-cursor first, then the
 * ones after it, skipping duplicates. */
static void append_by_recency(indexed_names_t &v, int cur, std::vector<std::string> &out)
{
    auto key = [cur](int idx) { return idx < cur ? (long)(cur - idx) : 1000000L + idx; };
    std::stable_sort(v.begin(), v.end(),
        [&key](const std::pair<int, std::string> &a, const std::pair<int, std::string> &b) {
            return key(a.first) < key(b.first);
        });
    for (const auto &e : v) {
        if (std::find(out.begin(), out.end(), e.second) == out.end()) out.push_back(e.second);
    }
}

static bool longer_with_prefix_nocase(const std::string &s, const std::string &pfx)
{
    if (s.size() <= pfx.size()) return false;
    for (size_t i = 0; i < pfx.size(); i++) {
        if (toupper((unsigned char)s[i]) != toupper((unsigned char)pfx[i])) return false;
    }
    return true;
}

static bool ftn_tab(bool append_only)
{
    if (append_only || editor_selection_active()) return false;
    int line;
    const char *lt;
    size_t ll;
    if (!cursor_at_line_end(&line, &lt, &ll)) return false;
    size_t text_len;
    size_t ls = (size_t)(lt - editor_get_text(&text_len));
    std::string cur(lt, ll);

    if (!s_tab_cands.empty() && ls == s_tab_line_start &&
        cur == s_tab_cands[s_tab_idx]) {
        s_tab_idx = (s_tab_idx + 1) % s_tab_cands.size();
    } else {
        collect_t c;
        c.cur_line = line;
        walk_lines(-1, collect_cb, &c);

        std::vector<std::string> cands;
        size_t sp = 0;
        while (sp < cur.size() && (cur[sp] == ' ' || cur[sp] == '\t')) sp++;
        std::string prefix = cur.substr(sp);
        if (prefix.empty()) {
            append_by_recency(c.names, line, cands);
            if (cands.size() >= 2) std::swap(cands[0], cands[1]);
        } else {
            std::vector<std::string> pool;
            append_by_recency(c.names, line, pool);
            append_by_recency(c.others, line, pool);
            static const char *const builtin[] = {
                "INT. ", "EXT. ", "INT./EXT. ", "EST. ", "CUT TO:",
            };
            for (const char *b : builtin) pool.push_back(b);
            for (const auto &e : pool) {
                if (longer_with_prefix_nocase(e, prefix) &&
                    std::find(cands.begin(), cands.end(), e) == cands.end()) {
                    cands.push_back(e);
                }
            }
        }
        if (cands.empty()) return false;
        cands.push_back(cur);   /* cycle back to what was typed */
        s_tab_cands.swap(cands);
        s_tab_idx = 0;
        s_tab_line_start = ls;
    }
    if (editor_replace_range(ls, ls + ll, s_tab_cands[s_tab_idx].c_str()) != ESP_OK)
        editor_ui_set_status("Buffer full -- increase editor size in menuconfig");
    return true;
}

static const fmt_help_row_t ftn_help[] = {
    { "INT. / EXT.",  "Scene heading after a blank line; force with a leading ." },
    { "NAME",         "Character: all caps after a blank line; force with @" },
    { "(line below)", "Dialogue follows the character" },
    { "(beat)",       "Parenthetical inside dialogue" },
    { "NAME ^",       "Dual dialogue" },
    { "CUT TO:",      "Transition; force with >" },
    { ">text<",       "Centered text" },
    { "!text",        "Force action" },
    { "~text",        "Lyrics" },
    { "= text",       "Synopsis" },
    { "# Act",        "Section (## and ### nest)" },
    { "===",          "Page break" },
    { "Title:",       "Title page key at the top of the script" },
    { "*i* **b**",    "Italic, bold (***both***)" },
    { "_text_",       "Underline" },
    { "[[note]]",     "Note" },
    { "/* .. */",     "Boneyard: text left out of the script" },
    { "Tab",          "Complete a character name or scene heading" },
    { "Enter",        "After a scene heading or transition, also adds the blank line" },
    { "Shift+Enter",  "Single new line" },
    { NULL, NULL },
};

extern const editor_format_t fountain_editor_format = {
    ftn_scan_init,
    ftn_parse,
    ftn_mark,
    ftn_layout,
    ftn_line_is_plain,
    ftn_enter,
    ftn_tab,
    "Fountain formatting",
    ftn_help,
};
