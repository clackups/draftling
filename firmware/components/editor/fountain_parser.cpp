#include <cstdint>
#include <cstring>
#include "fountain_parser.h"

static bool is_ws(char c)
{
    return c == ' ' || c == '\t';
}

static int skip_ws(const char *s, int len)
{
    int i = 0;
    while (i < len && is_ws(s[i])) i++;
    return i;
}

static int trim_end(const char *s, int len)
{
    while (len > 0 && is_ws(s[len - 1])) len--;
    return len;
}

/* Word character for the "_" intraword rule (see md_parser.cpp). */
static bool is_word(char c)
{
    unsigned char u = (unsigned char)c;
    return u >= 0x80 || (u >= '0' && u <= '9') ||
           (u >= 'a' && u <= 'z') || (u >= 'A' && u <= 'Z');
}

static int run_len(const char *s, int i, int to, char c)
{
    int n = 0;
    while (i + n < to && s[i + n] == c) n++;
    return n;
}

/* Index of the two-character sequence pat in s[from, to), or -1. */
static int find2(const char *s, int from, int to, const char *pat)
{
    for (int i = from; i + 1 < to; i++) {
        if (s[i] == pat[0] && s[i + 1] == pat[1]) return i;
    }
    return -1;
}

static void add_span(md_line_info_t *info, int start, int end, int mark,
                     bool bold, bool italic, bool underline, bool code,
                     bool strike)
{
    if (info->span_count >= MD_MAX_SPANS || end <= start) return;
    md_span_t *sp = &info->spans[info->span_count++];
    sp->start         = (size_t)start;
    sp->end           = (size_t)end;
    sp->mark_len      = (size_t)mark;
    sp->bold          = bold;
    sp->italic        = italic;
    sp->underline     = underline;
    sp->code          = code;
    sp->strikethrough = strike;
}

/* Emphasis in s[from, to): *italic*, **bold**, ***bold italic*** and
 * _underline_. Same simplified delimiter rules as the Markdown parser:
 * an opener must be followed and a closer preceded by non-whitespace,
 * "_" never opens or closes inside a word, a closer is a run of the
 * opener's length, and backslash escapes are skipped. Spans nest. */
static void parse_emphasis(const char *s, int from, int to, md_line_info_t *info)
{
    int i = from;
    while (i < to && info->span_count < MD_MAX_SPANS) {
        char c = s[i];
        if (c == '\\' && i + 1 < to) {
            i += 2;
            continue;
        }
        if (c != '*' && c != '_') {
            i++;
            continue;
        }

        int n = run_len(s, i, to, c);
        bool opener = (c == '_') ? (n == 1) : (n <= 3);
        if (opener && (i + n >= to || is_ws(s[i + n]))) opener = false;
        if (opener && c == '_' && i > 0 && is_word(s[i - 1])) opener = false;
        if (!opener) {
            i += n;
            continue;
        }

        int close = -1;
        int j = i + n;
        while (j < to) {
            if (s[j] == '\\') {
                j += 2;
                continue;
            }
            if (s[j] == c) {
                int m = run_len(s, j, to, c);
                if (m == n && !is_ws(s[j - 1]) &&
                    (c != '_' || j + m >= to || !is_word(s[j + m]))) {
                    close = j;
                    break;
                }
                j += m;
                continue;
            }
            j++;
        }
        if (close < 0) {
            i += n;
            continue;
        }

        bool under  = (c == '_');
        bool bold   = !under && n >= 2;
        bool italic = !under && n != 2;
        add_span(info, i + n, close, n, bold, italic, under, false, false);
        parse_emphasis(s, i + n, close, info);
        i = close + n;
    }
}

/* Inline syntax in s[from, to): notes ("[[...]]", boxed) and boneyard
 * comments (slash-star ... star-slash, struck through) are recognised
 * first, possibly continuing from or into neighbouring lines through
 * st; emphasis is parsed in the text between them. */
static void parse_inline(const char *s, int from, int to, ftn_state_t *st,
                         md_line_info_t *info)
{
    int i = from;
    int seg = from;
    while (i < to) {
        if (st->in_boneyard || st->in_note) {
            bool bone = st->in_boneyard;
            /* A comment opened on this line starts at i; one carried
             * over from the previous line starts at from. */
            int search = (i + 1 < to && s[i] == (bone ? '/' : '[') &&
                          s[i + 1] == (bone ? '*' : '[')) ? i + 2 : i;
            int close = find2(s, search, to, bone ? "*/" : "]]");
            int end = close < 0 ? to : close + 2;
            add_span(info, i, end, 0, false, false, false, !bone, bone);
            if (close < 0) return;
            if (bone) st->in_boneyard = false;
            else      st->in_note = false;
            i = seg = end;
            continue;
        }
        if (s[i] == '\\' && i + 1 < to) {
            i += 2;
            continue;
        }
        if (i + 1 < to && ((s[i] == '/' && s[i + 1] == '*') ||
                           (s[i] == '[' && s[i + 1] == '['))) {
            parse_emphasis(s, seg, i, info);
            if (s[i] == '/') st->in_boneyard = true;
            else             st->in_note = true;
            continue;
        }
        i++;
    }
    parse_emphasis(s, seg, to, info);
}

/* ---- Character-class helpers ---- */

/* Letter case of the UTF-8 character at s[*i] (advancing *i): +1
 * upper, -1 lower, 0 caseless or not a letter. Covers ASCII,
 * Latin-1 and basic Cyrillic -- the scripts the bundled fonts carry
 * with case; Hebrew has no case, so a Hebrew Character name must be
 * forced with "@". */
static int char_case(const char *s, int len, int *i)
{
    unsigned char c = (unsigned char)s[*i];
    if (c < 0x80) {
        (*i)++;
        if (c >= 'A' && c <= 'Z') return 1;
        if (c >= 'a' && c <= 'z') return -1;
        return 0;
    }
    int adv = (c & 0xE0) == 0xC0 ? 2 : (c & 0xF0) == 0xE0 ? 3 :
              (c & 0xF8) == 0xF0 ? 4 : 1;
    if (*i + adv > len) adv = len - *i;
    uint32_t cp = 0;
    if (adv == 2) {
        cp = ((uint32_t)(c & 0x1F) << 6) | ((unsigned char)s[*i + 1] & 0x3F);
    }
    *i += adv;
    if (cp >= 0xC0 && cp <= 0xDE && cp != 0xD7) return 1;
    if (cp >= 0xDF && cp <= 0xFF && cp != 0xF7) return -1;
    if (cp >= 0x400 && cp <= 0x42F) return 1;
    if (cp >= 0x430 && cp <= 0x45F) return -1;
    return 0;
}

/* True when s[0, len) has at least one upper-case letter and no
 * lower-case ones. */
static bool is_upper_text(const char *s, int len)
{
    bool upper = false;
    int i = 0;
    while (i < len) {
        int c = char_case(s, len, &i);
        if (c < 0) return false;
        if (c > 0) upper = true;
    }
    return upper;
}

static bool starts_with_ci(const char *s, int len, const char *pfx)
{
    int n = (int)strlen(pfx);
    if (len < n) return false;
    for (int i = 0; i < n; i++) {
        char c = s[i];
        if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
        if (c != pfx[i]) return false;
    }
    return true;
}

/* INT, EXT, EST, INT./EXT, INT/EXT or I/E, followed by "." or " ". */
static bool is_scene_prefix(const char *p, int rem)
{
    static const char *const prefixes[] = {
        "INT./EXT", "INT/EXT", "I/E", "INT", "EXT", "EST",
    };
    for (const char *pfx : prefixes) {
        int n = (int)strlen(pfx);
        if (starts_with_ci(p, rem, pfx)) {
            return n < rem && (p[n] == '.' || p[n] == ' ');
        }
    }
    return false;
}

/* "===" (three or more "=", nothing else but whitespace). */
static bool is_page_break(const char *p, int rem)
{
    int n = 0;
    for (int i = 0; i < rem; i++) {
        if (p[i] == '=') n++;
        else if (!is_ws(p[i])) return false;
    }
    return n >= 3;
}

/* Length of a title-page "Key:" prefix (including the colon), or 0. */
static int title_key_len(const char *s, int len)
{
    if (len < 2 || !((s[0] >= 'A' && s[0] <= 'Z') || (s[0] >= 'a' && s[0] <= 'z')))
        return 0;
    for (int i = 1; i < len; i++) {
        char c = s[i];
        if (c == ':') return i + 1;
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == ' '))
            return 0;
    }
    return 0;
}

/* Length of the Character cue in p[0, rem) with any dual-dialogue
 * caret and trailing whitespace dropped. */
static int character_len(const char *p, int rem)
{
    int e = trim_end(p, rem);
    if (e > 0 && p[e - 1] == '^') e = trim_end(p, e - 1);
    return e;
}

/* Upper-case name, optionally followed by a parenthesised extension
 * in any case: "MOM (O.S.)", "HANS (on the radio)". */
static bool is_character(const char *p, int clen)
{
    if (clen <= 0 || p[0] == '(') return false;
    int name_end = 0;
    while (name_end < clen && p[name_end] != '(') name_end++;
    return is_upper_text(p, name_end);
}

static void set_type(md_line_info_t *info, md_line_type_t type,
                     const char *line, int start, int end)
{
    info->type        = type;
    info->content     = line + start;
    info->content_len = (size_t)(end > start ? end - start : 0);
}

extern "C" void ftn_state_init(ftn_state_t *st)
{
    memset(st, 0, sizeof(*st));
    st->at_start   = true;
    st->prev_blank = true;
}

extern "C" void fountain_parse_line(const char *line, size_t len_,
                                    bool next_blank, ftn_state_t *st,
                                    md_line_info_t *info)
{
    /* Scratch output when the caller only needs the state advanced.
     * The editor UI is single-threaded (LVGL lock held). */
    static md_line_info_t s_scratch;
    if (!info) info = &s_scratch;
    info->indent_level = 0;
    info->span_count   = 0;

    int len = (int)len_;
    int sp = skip_ws(line, len);
    bool blank = sp >= len;
    bool two_spaces = len >= 2 && blank;   /* "  " keeps a block going */

    /* A boneyard comment or note carried over from an earlier line.
     * A note may not contain a truly empty line: one ends it. */
    if (st->in_note && blank && !two_spaces) st->in_note = false;
    if (st->in_boneyard || st->in_note) {
        bool bone = st->in_boneyard;
        int close = find2(line, 0, len, bone ? "*/" : "]]");
        if (close < 0 || close + 2 >= trim_end(line, len)) {
            set_type(info, bone ? MD_LINE_FTN_BONEYARD : MD_LINE_FTN_NOTE,
                     line, 0, len);
            parse_inline(line, 0, len, st, info);
            return;   /* comment lines leave the element state alone */
        }
    }

    /* A line that is nothing but a comment (possibly left open). */
    if (sp + 1 < len && ((line[sp] == '/' && line[sp + 1] == '*') ||
                         (line[sp] == '[' && line[sp + 1] == '['))) {
        bool bone = line[sp] == '/';
        int close = find2(line, sp + 2, len, bone ? "*/" : "]]");
        if (close < 0 || close + 2 >= trim_end(line, len)) {
            set_type(info, bone ? MD_LINE_FTN_BONEYARD : MD_LINE_FTN_NOTE,
                     line, 0, len);
            parse_inline(line, 0, len, st, info);
            return;
        }
    }

    bool at_start = st->at_start;
    st->at_start = false;

    if (blank) {
        if (st->in_dialogue && two_spaces) {
            set_type(info, MD_LINE_FTN_DIALOGUE, line, 0, len);
            st->prev_blank = false;
            return;
        }
        set_type(info, MD_LINE_EMPTY, line, 0, len);
        st->in_dialogue = false;
        st->title_page  = false;
        st->prev_blank  = true;
        return;
    }

    bool prev_blank = st->prev_blank;
    st->prev_blank = false;

    /* Title page: "Key: value" lines at the very top, with indented
     * continuation values, up to the first blank line. */
    if (st->title_page || (at_start && title_key_len(line, len) > 0)) {
        st->title_page = true;
        set_type(info, MD_LINE_FTN_TITLE, line, 0, len);
        int k = title_key_len(line, len);
        add_span(info, 0, k, 0, true, false, false, false, false);
        parse_inline(line, k, len, st, info);
        return;
    }

    const char *p = line + sp;
    int rem = len - sp;
    int end = trim_end(line, len);

    if (st->in_dialogue) {
        md_line_type_t t = MD_LINE_FTN_DIALOGUE;
        int start = sp;
        if (p[0] == '(') {
            t = MD_LINE_FTN_PARENTHETICAL;
        } else if (p[0] == '~') {
            t = MD_LINE_FTN_LYRIC;
            start = sp + 1;
        }
        set_type(info, t, line, start, end);
        parse_inline(info->content, 0, (int)info->content_len, st, info);
        return;
    }

    if (is_page_break(p, rem)) {
        set_type(info, MD_LINE_HR, line, sp, end);
        return;
    }

    /* Section: "#", "##", "###"... (deeper levels share the H3 look). */
    if (p[0] == '#') {
        int h = run_len(p, 0, rem, '#');
        int lvl = h > 3 ? 3 : h;
        set_type(info, (md_line_type_t)(MD_LINE_H1 + lvl - 1), line,
                 sp + h + skip_ws(p + h, rem - h), end);
        parse_inline(info->content, 0, (int)info->content_len, st, info);
        return;
    }

    md_line_type_t t = MD_LINE_FTN_ACTION;
    int start = sp, stop = end;
    bool ends_to = end - sp >= 3 && end == len &&
                   strncmp(line + len - 3, "TO:", 3) == 0;
    bool dialogue_follows = false;

    if (p[0] == '=') {
        t = MD_LINE_FTN_SYNOPSIS;
        start = sp + 1 + skip_ws(p + 1, rem - 1);
    } else if (p[0] == '!') {
        /* Forced Action keeps its own indentation. */
        start = sp + 1;
    } else if (p[0] == '.' && rem > 1 && is_word(p[1])) {
        /* Forced Scene Heading: "." followed by an alphanumeric, so
         * an ellipsis ("...") stays Action. */
        t = MD_LINE_FTN_SCENE;
        start = sp + 1;
    } else if (p[0] == '>') {
        if (end - sp >= 2 && line[end - 1] == '<') {
            t = MD_LINE_FTN_CENTERED;
            start = sp + 1 + skip_ws(p + 1, rem - 1);
            stop = trim_end(line, end - 1);
        } else {
            t = MD_LINE_FTN_TRANSITION;
            start = sp + 1 + skip_ws(p + 1, rem - 1);
        }
    } else if (p[0] == '~') {
        t = MD_LINE_FTN_LYRIC;
        start = sp + 1;
    } else if (p[0] == '@') {
        t = MD_LINE_FTN_CHARACTER;
        start = sp + 1;
        stop = sp + character_len(p, rem);
        dialogue_follows = true;
    } else if (prev_blank && is_scene_prefix(p, rem)) {
        t = MD_LINE_FTN_SCENE;
    } else if (prev_blank && ends_to && next_blank && is_upper_text(p, rem)) {
        t = MD_LINE_FTN_TRANSITION;
    } else if (prev_blank && !next_blank && !ends_to &&
               is_character(p, character_len(p, rem))) {
        /* "CUT TO:" is never a Character cue, even while the line
         * after it is still being typed. */
        t = MD_LINE_FTN_CHARACTER;
        stop = sp + character_len(p, rem);
        dialogue_follows = true;
    } else {
        /* Plain Action keeps its indentation (tabs and spaces are
         * significant there) and its trailing whitespace. */
        start = 0;
        stop = len;
    }

    set_type(info, t, line, start, stop);
    parse_inline(info->content, 0, (int)info->content_len, st, info);
    st->in_dialogue = dialogue_follows;
}
