#include <cstring>
#include "md_parser.h"

static int skip_spaces(const char *s, int len)
{
    int i = 0;
    while (i < len && (s[i] == ' ' || s[i] == '\t')) i++;
    return i;
}

static int count_char(const char *s, int len, char c)
{
    int n = 0;
    while (n < len && s[n] == c) n++;
    return n;
}

static bool is_hr_line(const char *s, int len)
{
    int sp = skip_spaces(s, len);
    if (sp >= len) return false;
    char c = s[sp];
    if (c != '-' && c != '*' && c != '_') return false;
    int count = 0;
    for (int i = sp; i < len; i++) {
        if (s[i] == c) count++;
        else if (s[i] != ' ') return false;
    }
    return count >= 3;
}

static bool is_ws(char c)
{
    return c == ' ' || c == '\t';
}

/* Word character for the "_" intraword rule. Any non-ASCII byte counts
 * as a word character, so "_" inside Cyrillic / Hebrew words is left
 * alone just like inside Latin ones. */
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

/* Find the closing run of exactly n backticks for a code span opened
 * at from (the first byte after the opening run). Returns its index,
 * or -1 if the span is unterminated on this line. */
static int find_code_close(const char *s, int from, int to, int n)
{
    int j = from;
    while (j < to) {
        if (s[j] == '`') {
            int m = run_len(s, j, to, '`');
            if (m == n) return j;
            j += m;
        } else {
            j++;
        }
    }
    return -1;
}

static void add_span(md_line_info_t *info, int start, int end, int mark,
                     bool bold, bool italic, bool code, bool strike)
{
    md_span_t *sp = &info->spans[info->span_count++];
    sp->start         = (size_t)start;
    sp->end           = (size_t)end;
    sp->mark_len      = (size_t)mark;
    sp->bold          = bold;
    sp->italic        = italic;
    sp->code          = code;
    sp->strikethrough = strike;
}

/* Recognise inline spans in s[from, to): `code` (any backtick-run
 * length), *italic* / _italic_, **bold** / __bold__, ***both*** /
 * ___both___ and ~~strikethrough~~. A simplified subset of the
 * CommonMark delimiter rules keeps ordinary text from being styled by
 * accident:
 *  - an opener must be followed, and a closer preceded, by a
 *    non-whitespace character ("2 * 3 * 4" stays plain);
 *  - "_" never opens or closes inside a word (snake_case_names);
 *  - a closer must be a run of the same length as the opener;
 *  - backslash-escaped characters and code spans are skipped.
 * Emphasis spans are parsed recursively so they can nest. */
static void parse_range(const char *s, int from, int to, md_line_info_t *info)
{
    int i = from;
    while (i < to && info->span_count < MD_MAX_SPANS) {
        char c = s[i];
        if (c == '\\' && i + 1 < to) {
            i += 2;
            continue;
        }
        if (c == '`') {
            int n = run_len(s, i, to, '`');
            int close = find_code_close(s, i + n, to, n);
            if (close > i + n) {
                add_span(info, i + n, close, n, false, false, true, false);
                i = close + n;
            } else {
                i += n;
            }
            continue;
        }
        if (c != '*' && c != '_' && c != '~') {
            i++;
            continue;
        }

        int n = run_len(s, i, to, c);
        bool opener = (c == '~') ? (n == 2) : (n <= 3);
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
            if (s[j] == '`') {
                int m = run_len(s, j, to, '`');
                int cc = find_code_close(s, j + m, to, m);
                j = (cc > 0) ? cc + m : j + m;
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

        bool strike = (c == '~');
        bool bold   = !strike && n >= 2;
        bool italic = !strike && n != 2;
        add_span(info, i + n, close, n, bold, italic, false, strike);
        parse_range(s, i + n, close, info);
        i = close + n;
    }
}

static void parse_inline_spans(const char *s, int len, md_line_info_t *info)
{
    info->span_count = 0;
    parse_range(s, 0, len, info);
}

extern "C" bool md_is_code_fence(const char *line, size_t len)
{
    int sp = skip_spaces(line, (int)len);
    if (sp + 2 >= (int)len) return false;
    return (line[sp] == '`' && line[sp+1] == '`' && line[sp+2] == '`');
}

extern "C" void md_parse_line(const char *line, size_t len, md_line_info_t *info, bool in_code_block)
{
    memset(info, 0, sizeof(*info));
    info->content = line;
    info->content_len = len;

    int sp = skip_spaces(line, (int)len);
    info->indent_level = sp / 4;

    /* Empty line */
    if (sp >= (int)len) {
        info->type = MD_LINE_EMPTY;
        return;
    }

    /* Code fence */
    if (md_is_code_fence(line, len)) {
        info->type = MD_LINE_CODE_FENCE;
        info->content = line + sp + 3;
        info->content_len = len - sp - 3;
        return;
    }

    /* Inside code block */
    if (in_code_block) {
        info->type = MD_LINE_CODE_CONTENT;
        return;
    }

    const char *p = line + sp;
    int rem = (int)len - sp;

    /* Headings */
    int hashes = count_char(p, rem, '#');
    if (hashes >= 1 && hashes <= 4 && hashes < rem && p[hashes] == ' ') {
        info->type = (md_line_type_t)(MD_LINE_H1 + hashes - 1);
        info->content = p + hashes + 1;
        info->content_len = rem - hashes - 1;
        parse_inline_spans(info->content, (int)info->content_len, info);
        return;
    }

    /* Horizontal rule */
    if (is_hr_line(line, (int)len)) {
        info->type = MD_LINE_HR;
        return;
    }

    /* Bullet list */
    if (rem >= 2 && (p[0] == '-' || p[0] == '*' || p[0] == '+') && p[1] == ' ') {
        info->type = MD_LINE_BULLET;
        info->content = p + 2;
        info->content_len = rem - 2;
        parse_inline_spans(info->content, (int)info->content_len, info);
        return;
    }

    /* Numbered list */
    int d = 0;
    while (d < rem && p[d] >= '0' && p[d] <= '9') d++;
    if (d > 0 && d + 1 < rem && p[d] == '.' && p[d+1] == ' ') {
        info->type = MD_LINE_NUMBERED;
        info->content = p + d + 2;
        info->content_len = rem - d - 2;
        parse_inline_spans(info->content, (int)info->content_len, info);
        return;
    }

    /* Blockquote */
    if (rem >= 2 && p[0] == '>' && p[1] == ' ') {
        info->type = MD_LINE_BLOCKQUOTE;
        info->content = p + 2;
        info->content_len = rem - 2;
        parse_inline_spans(info->content, (int)info->content_len, info);
        return;
    }

    /* Normal paragraph */
    info->type = MD_LINE_PARAGRAPH;
    info->content = p;
    info->content_len = rem;
    parse_inline_spans(info->content, (int)info->content_len, info);
}
