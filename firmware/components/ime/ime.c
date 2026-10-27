/*
 * Input-method dispatcher and the helpers shared by the engines:
 * composition tracking, UTF-8, candidate pages and dictionary lookup.
 */

#include <string.h>
#include <stdio.h>
#include "ime_internal.h"

static const ime_host_t *s_host;
static const ime_engine_t *s_engine;   /* engine of the open composition */

ime_dict_t g_ja_dict;
ime_dict_t g_zh_dict;
int g_status_cells = 40;

char   g_comp[IME_COMP_MAX];
size_t g_comp_len;

/* ---- Host access ---- */

size_t ime_peek(char *buf, size_t max)
{
    if (!s_host || !s_host->peek_before) return 0;
    return s_host->peek_before(buf, max);
}

bool ime_replace(size_t del, const char *text)
{
    if (!s_host || !s_host->replace_before) return false;
    return s_host->replace_before(del, text);
}

/* ---- Composition tracking ---- */

bool comp_valid(void)
{
    if (g_comp_len == 0) return true;
    char buf[IME_COMP_MAX];
    size_t n = ime_peek(buf, g_comp_len);
    return n == g_comp_len && memcmp(buf, g_comp, n) == 0;
}

bool comp_show(const char *text)
{
    size_t len = strlen(text);
    if (len >= IME_COMP_MAX) return false;
    if (len == g_comp_len && memcmp(text, g_comp, len) == 0) return true;
    if (!ime_replace(g_comp_len, text)) return false;
    memcpy(g_comp, text, len);
    g_comp[len] = '\0';
    g_comp_len = len;
    return true;
}

void comp_forget(void)
{
    g_comp_len = 0;
    g_comp[0] = '\0';
}

void comp_drop_front(size_t n)
{
    if (n > g_comp_len) n = g_comp_len;
    memmove(g_comp, g_comp + n, g_comp_len - n + 1);
    g_comp_len -= n;
}

/* ---- Strings ---- */

void ime_copy(char *out, size_t size, const char *src)
{
    if (size == 0) return;
    size_t n = strlen(src);
    if (n >= size) n = size - 1;
    memmove(out, src, n);
    out[n] = '\0';
}

void ime_join(char *out, size_t size, const char *a, const char *b)
{
    if (size == 0) return;
    size_t la = strlen(a), lb = strlen(b);
    if (la >= size) la = size - 1;
    if (la + lb >= size) lb = size - 1 - la;
    memmove(out, a, la);
    memmove(out + la, b, lb);
    out[la + lb] = '\0';
}

/* ---- UTF-8 ---- */

uint32_t utf8_decode(const char *s, size_t len, size_t *n)
{
    const uint8_t *u = (const uint8_t *)s;
    if (len == 0) { *n = 0; return 0; }
    uint8_t c = u[0];
    size_t need;
    uint32_t cp;
    if (c < 0x80)      { *n = 1; return c; }
    else if (c < 0xE0) { need = 2; cp = c & 0x1F; }
    else if (c < 0xF0) { need = 3; cp = c & 0x0F; }
    else               { need = 4; cp = c & 0x07; }
    if (need > len) { *n = 1; return 0xFFFD; }
    for (size_t i = 1; i < need; i++) {
        if ((u[i] & 0xC0) != 0x80) { *n = 1; return 0xFFFD; }
        cp = (cp << 6) | (u[i] & 0x3F);
    }
    *n = need;
    return cp;
}

size_t utf8_encode(uint32_t cp, char *out)
{
    if (cp < 0x80) { out[0] = (char)cp; return 1; }
    if (cp < 0x800) {
        out[0] = (char)(0xC0 | (cp >> 6));
        out[1] = (char)(0x80 | (cp & 0x3F));
        return 2;
    }
    if (cp < 0x10000) {
        out[0] = (char)(0xE0 | (cp >> 12));
        out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[2] = (char)(0x80 | (cp & 0x3F));
        return 3;
    }
    out[0] = (char)(0xF0 | (cp >> 18));
    out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
    out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
    out[3] = (char)(0x80 | (cp & 0x3F));
    return 4;
}

size_t utf8_last_len(const char *s, size_t len)
{
    size_t i = len;
    while (i > 0) {
        i--;
        if (((uint8_t)s[i] & 0xC0) != 0x80) break;
    }
    return len - i;
}

bool is_wide_cp(uint32_t cp)
{
    return (cp >= 0x2E80 && cp <= 0x2FDF) ||
           (cp >= 0x3000 && cp <= 0x33FF) ||
           (cp >= 0x3400 && cp <= 0x4DBF) ||
           (cp >= 0x4E00 && cp <= 0x9FFF) ||
           (cp >= 0xAC00 && cp <= 0xD7A3) ||
           (cp >= 0xF900 && cp <= 0xFAFF) ||
           (cp >= 0xFE30 && cp <= 0xFE4F) ||
           (cp >= 0xFF01 && cp <= 0xFF60) ||
           (cp >= 0xFFE0 && cp <= 0xFFE6);
}

int ime_text_cells(const char *s, size_t len)
{
    int cells = 0;
    size_t i = 0;
    while (i < len) {
        size_t n;
        uint32_t cp = utf8_decode(s + i, len - i, &n);
        if (n == 0) break;
        cells += is_wide_cp(cp) ? 2 : 1;
        i += n;
    }
    return cells;
}

/* ---- Candidates ---- */

void cands_clear(ime_cands_t *cl)
{
    cl->count = 0;
    cl->sel = 0;
    cl->pages = 0;
}

bool cands_add(ime_cands_t *cl, const char *text, size_t len, uint8_t span)
{
    if (len == 0 || len >= IME_CAND_TEXT) return true;   /* skip, not full */
    for (int i = 0; i < cl->count; i++) {
        if (strlen(cl->c[i].text) == len && memcmp(cl->c[i].text, text, len) == 0)
            return true;
    }
    if (cl->count >= IME_CAND_MAX) return false;
    memcpy(cl->c[cl->count].text, text, len);
    cl->c[cl->count].text[len] = '\0';
    cl->c[cl->count].span = span;
    cl->count++;
    return true;
}

void cands_paginate(ime_cands_t *cl, int prefix_cells)
{
    int avail = g_status_cells - prefix_cells;
    if (avail < 8) avail = 8;
    cl->pages = 0;
    int i = 0;
    while (i < cl->count) {
        cl->page_start[cl->pages++] = i;
        int used = 0, on_page = 0;
        while (i < cl->count && on_page < 9) {
            const char *t = cl->c[i].text;
            int w = 3 + ime_text_cells(t, strlen(t));   /* "1." + text + gap */
            if (on_page > 0 && used + w > avail) break;
            used += w;
            on_page++;
            i++;
        }
    }
    cl->page_start[cl->pages] = cl->count;
}

int cands_page_of(const ime_cands_t *cl, int idx)
{
    for (int p = 0; p < cl->pages; p++) {
        if (idx < cl->page_start[p + 1]) return p;
    }
    return cl->pages > 0 ? cl->pages - 1 : 0;
}

/* Append printf-formatted text at *pos, never past out_size. */
static void append_fmt(char *out, size_t out_size, size_t *pos, const char *fmt, int a, const char *b)
{
    if (*pos + 1 >= out_size) return;
    int n = b ? snprintf(out + *pos, out_size - *pos, fmt, a, b)
              : snprintf(out + *pos, out_size - *pos, fmt, a);
    if (n < 0) return;
    *pos += (size_t)n;
    if (*pos >= out_size) *pos = out_size - 1;
}

void cands_format(const ime_cands_t *cl, const char *prefix, char *out, size_t out_size)
{
    if (out_size == 0) return;
    ime_copy(out, out_size, prefix ? prefix : "");
    size_t pos = strlen(out);
    if (cl->count == 0) return;
    int p = cands_page_of(cl, cl->sel);
    for (int i = cl->page_start[p]; i < cl->page_start[p + 1]; i++) {
        int num = i - cl->page_start[p] + 1;
        append_fmt(out, out_size, &pos, i == cl->sel ? " %d[%s]" : " %d.%s", num, cl->c[i].text);
    }
    if (cl->pages > 1) {
        char pg[16];
        snprintf(pg, sizeof(pg), "%d", cl->pages);
        append_fmt(out, out_size, &pos, " %d/%s", p + 1, pg);
    }
}

int cands_digit(const ime_cands_t *cl, int d)
{
    if (cl->count == 0 || d < 1 || d > 9) return -1;
    int p = cands_page_of(cl, cl->sel);
    int idx = cl->page_start[p] + d - 1;
    return idx < cl->page_start[p + 1] ? idx : -1;
}

/* ---- Dictionaries ---- */

static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

bool dict_open(ime_dict_t *d, const uint8_t *blob)
{
    memset(d, 0, sizeof(*d));
    if (!blob || memcmp(blob, "DIME", 4) != 0 || blob[4] != 1) return false;
    d->count = rd32(blob + 8);
    d->offs = blob + 12;
    d->data = d->offs + 4 * (d->count + 1);
    return true;
}

bool dict_lookup(const ime_dict_t *d, const char *key, size_t key_len,
                 uint16_t *cost, uint8_t *ncand, const char **cands)
{
    if (!d || d->count == 0) return false;
    int32_t lo = 0, hi = (int32_t)d->count - 1;
    while (lo <= hi) {
        int32_t mid = (lo + hi) >> 1;
        const char *k = (const char *)d->data + rd32(d->offs + 4 * mid);
        size_t kl = strlen(k);
        size_t m = kl < key_len ? kl : key_len;
        int c = memcmp(k, key, m);
        if (c == 0) c = (kl < key_len) ? -1 : (kl > key_len ? 1 : 0);
        if (c == 0) {
            const uint8_t *p = (const uint8_t *)k + kl + 1;
            if (cost)  *cost = (uint16_t)(p[0] | (p[1] << 8));
            if (ncand) *ncand = p[2];
            if (cands) *cands = (const char *)p + 3;
            return true;
        }
        if (c < 0) lo = mid + 1;
        else hi = mid - 1;
    }
    return false;
}

/* ---- Dispatcher ---- */

void ime_init(const ime_host_t *host, const uint8_t *ja_dict, const uint8_t *zh_dict)
{
    s_host = host;
    dict_open(&g_ja_dict, ja_dict);
    dict_open(&g_zh_dict, zh_dict);
}

static const ime_engine_t *engine_for(kb_ime_t kind)
{
    switch (kind) {
    case KB_IME_TELEX:  return &ime_telex_engine;
    case KB_IME_HANGUL: return &ime_hangul_engine;
    case KB_IME_KANA:   return &ime_kana_engine;
    case KB_IME_PINYIN: return &ime_pinyin_engine;
    default:            return NULL;
    }
}

void ime_commit(void)
{
    if (s_engine) s_engine->commit();
    s_engine = NULL;
    comp_forget();
}

bool ime_handle_key(kb_ime_t kind, uint8_t keycode, uint8_t modifier, const char *text)
{
    const ime_engine_t *e = engine_for(kind);
    if (s_engine && s_engine != e) ime_commit();   /* layout changed */
    if (!e) return false;
    /* The cursor moved away from the composition (or the text around
     * it was edited): keep what is there and start afresh. */
    if (s_engine && !comp_valid()) ime_commit();
    s_engine = e;
    bool used = e->key(keycode, modifier, text);
    if (!e->composing()) {
        s_engine = NULL;
        comp_forget();
    }
    return used;
}

bool ime_composing(void)
{
    return s_engine && s_engine->composing();
}

const char *ime_status(void)
{
    return s_engine ? s_engine->status() : NULL;
}

void ime_set_status_cells(int cells)
{
    g_status_cells = cells > 10 ? cells : 10;
}
