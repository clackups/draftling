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

static void parse_inline_spans(const char *s, int len, md_line_info_t *info)
{
    info->span_count = 0;
    int i = 0;
    while (i < len && info->span_count < 16) {
        /* Inline code */
        if (s[i] == '`') {
            int start = i + 1;
            int end = start;
            while (end < len && s[end] != '`') end++;
            if (end < len) {
                md_span_t *sp = &info->spans[info->span_count++];
                sp->start = start;
                sp->end   = end;
                sp->bold  = false; sp->italic = false;
                sp->code  = true;  sp->strikethrough = false;
                i = end + 1;
                continue;
            }
        }
        /* Bold **text** */
        if (i + 1 < len && s[i] == '*' && s[i+1] == '*') {
            int start = i + 2;
            int end = start;
            while (end + 1 < len && !(s[end] == '*' && s[end+1] == '*')) end++;
            if (end + 1 < len) {
                md_span_t *sp = &info->spans[info->span_count++];
                sp->start = start;
                sp->end   = end;
                sp->bold  = true; sp->italic = false;
                sp->code  = false; sp->strikethrough = false;
                i = end + 2;
                continue;
            }
        }
        /* Italic *text* */
        if (s[i] == '*' && (i + 1 >= len || s[i+1] != '*')) {
            int start = i + 1;
            int end = start;
            while (end < len && s[end] != '*') end++;
            if (end < len) {
                md_span_t *sp = &info->spans[info->span_count++];
                sp->start = start;
                sp->end   = end;
                sp->bold  = false; sp->italic = true;
                sp->code  = false; sp->strikethrough = false;
                i = end + 1;
                continue;
            }
        }
        /* Strikethrough ~~text~~ */
        if (i + 1 < len && s[i] == '~' && s[i+1] == '~') {
            int start = i + 2;
            int end = start;
            while (end + 1 < len && !(s[end] == '~' && s[end+1] == '~')) end++;
            if (end + 1 < len) {
                md_span_t *sp = &info->spans[info->span_count++];
                sp->start = start;
                sp->end   = end;
                sp->bold  = false; sp->italic = false;
                sp->code  = false; sp->strikethrough = true;
                i = end + 2;
                continue;
            }
        }
        i++;
    }
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
