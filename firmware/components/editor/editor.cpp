#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <strings.h>
#include <esp_log.h>
#include <esp_heap_caps.h>

#include "editor.h"
#include "sd_card.h"

static const char *TAG = "Editor";

/* ---- Per-document state ----
 *
 * Everything that used to be a file-scope global now lives in an
 * editor_doc_t so the engine can hold several open documents at once
 * (one per editor pane). A small fixed pool of these is kept in s_docs[]
 * and the public API operates on whichever one s_active points at.
 *
 * The clipboard is intentionally NOT part of this struct: it is a single
 * system clipboard shared across every document (see s_clipboard below).
 */
struct editor_doc_s {
    bool   in_use;        /* slot is allocated */
    int    refcount;      /* number of panes referencing this doc */

    char  *buf;           /* gap buffer (NULL until first use) */
    size_t gap_start;     /* gap start == cursor position */
    size_t gap_end;
    size_t buf_size;      /* allocated size of buf */
    /* editor_get_line() copies a line that the gap splits (only ever
     * the cursor line) here; every other line is returned in place. */
    char  *line_buf;
    size_t line_buf_cap;
    /* Cursor saved by editor_text_begin(), restored by _end(). */
    size_t text_cursor;

    char   path[256];     /* backing file path, "" when untitled */
    bool   modified;
    editor_doc_format_t format; /* Markdown, Fountain or plain text */
    editor_mode_t mode;
    int    scroll_line;
    int    sel_anchor;    /* logical byte offset, -1 = no selection */

    /* Newlines before the gap (= the cursor line) and in the whole
     * document, kept up to date by every gap move and edit, so the
     * line count and the cursor line cost nothing to read. */
    int    nl_before;
    int    nl_total;
    /* Line-lookup cache used by editor_get_line(): line line_cache_num
     * starts at byte line_cache_offset. Dropped by an edit before that
     * offset. -1 = empty. */
    int    line_cache_num;
    size_t line_cache_offset;
    /* Lowest byte offset changed since editor_change_mark_reset(i);
     * SIZE_MAX = unchanged. Lets the UI keep per-pane caches. */
    size_t change_mark[EDITOR_CHANGE_MARKS];
};

/* Maximum number of simultaneously open documents. Two is enough for the
 * side-by-side editor panes; bump this if more panes are ever added. */
#define EDITOR_MAX_DOCS 2

static editor_doc_t  s_docs[EDITOR_MAX_DOCS];
static editor_doc_t *s_active = NULL;   /* the document the public API acts on */

/* Per-document buffer size, computed once from free PSRAM in
 * editor_init() and reused for every pool slot's lazy allocation. */
static size_t s_doc_buf_size = 0;

/* The rest of this file was written against file-scope globals named
 * s_buf, s_gap_start, ... Rather than thread an editor_doc_t* through
 * dozens of small helpers, alias each former global to the matching
 * field of the active document. Every existing function body is then
 * unchanged and automatically operates on s_active. */
#define s_buf               (s_active->buf)
#define s_gap_start         (s_active->gap_start)
#define s_gap_end           (s_active->gap_end)
#define s_buf_size          (s_active->buf_size)
#define s_path              (s_active->path)
#define s_modified          (s_active->modified)
#define s_format            (s_active->format)
#define s_mode              (s_active->mode)
#define s_scroll_line       (s_active->scroll_line)
#define s_sel_anchor        (s_active->sel_anchor)
#define s_nl_before         (s_active->nl_before)
#define s_nl_total          (s_active->nl_total)
#define s_line_cache_num    (s_active->line_cache_num)
#define s_line_cache_offset (s_active->line_cache_offset)

/* Shared system clipboard (not per-document). */
static char  *s_clipboard  = NULL;
static size_t s_clip_len   = 0;
/* A selection too large for a PSRAM block is kept in this hidden file
 * instead (s_clip_in_file); paste reads it straight into the gap. */
static bool   s_clip_in_file = false;
#ifndef CLIPBOARD_FILE
#define CLIPBOARD_FILE "/sdcard/.clipboard"
#endif

/* Content length (excluding gap) */
static inline size_t content_len(void) { return s_buf_size - (s_gap_end - s_gap_start); }

/* Character at logical position p (0..content_len-1) */
static inline char char_at(size_t p)
{
    return (p < s_gap_start) ? s_buf[p] : s_buf[p + (s_gap_end - s_gap_start)];
}

static int count_nl(const char *p, size_t n)
{
    int c = 0;
    const char *end = p + n;
    while (p < end) {
        p = (const char *)memchr(p, '\n', (size_t)(end - p));
        if (!p) break;
        c++;
        p++;
    }
    return c;
}

/* The whole document was replaced (open, new, close, discard): drop
 * every cache and recount the newlines. */
static void doc_invalidate_caches(editor_doc_t *d)
{
    d->line_cache_num    = -1;
    d->line_cache_offset = 0;
    for (int i = 0; i < EDITOR_CHANGE_MARKS; i++) d->change_mark[i] = 0;
    if (d->buf) {
        d->nl_before = count_nl(d->buf, d->gap_start);
        d->nl_total  = d->nl_before +
                       count_nl(d->buf + d->gap_end, d->buf_size - d->gap_end);
    } else {
        d->nl_before = d->nl_total = 0;
    }
}

static void invalidate_caches(void)
{
    doc_invalidate_caches(s_active);
}

/* The text changed at logical byte offset pos and after it; everything
 * before pos is untouched. The newline counters are updated by the
 * caller. */
static void content_changed(size_t pos)
{
    if (s_line_cache_num >= 0 && s_line_cache_offset > pos) {
        s_line_cache_num = -1;
        s_line_cache_offset = 0;
    }
    for (int i = 0; i < EDITOR_CHANGE_MARKS; i++) {
        if (s_active->change_mark[i] > pos) s_active->change_mark[i] = pos;
    }
    s_modified = true;
}

/* Drop the bytes between the new and the old gap start / end, keeping
 * the newline counters in step. */
static void remove_before_gap(size_t new_start)
{
    int n = count_nl(s_buf + new_start, s_gap_start - new_start);
    s_nl_before -= n;
    s_nl_total  -= n;
    s_gap_start = new_start;
}

static void remove_after_gap(size_t new_end)
{
    s_nl_total -= count_nl(s_buf + s_gap_end, new_end - s_gap_end);
    s_gap_end = new_end;
}

/* Copy text into the gap at the cursor. The caller checked the size. */
static void insert_at_gap(const char *text, size_t len)
{
    memcpy(s_buf + s_gap_start, text, len);
    int n = count_nl(text, len);
    s_nl_before += n;
    s_nl_total  += n;
    s_gap_start += len;
}

static void ensure_gap(size_t need)
{
    size_t gap_sz = s_gap_end - s_gap_start;
    if (gap_sz >= need) return;
    /* Should not happen with 256K buffer and reasonable docs */
    ESP_LOGE(TAG, "Gap buffer full");
}

/* ---- Document pool management ---- */

/* Reset a document slot to an empty, untitled buffer. The slot's
 * buffers must already be allocated (doc_ensure_buffers). */
static void doc_reset_empty(editor_doc_t *d)
{
    d->gap_start   = 0;
    d->gap_end     = d->buf_size;
    d->path[0]     = '\0';
    d->modified    = false;
    d->format      = EDITOR_DOC_MARKDOWN;
    d->mode        = EDITOR_MODE_NORMAL;
    d->scroll_line = 0;
    d->sel_anchor  = -1;
    doc_invalidate_caches(d);
}

/* Compute the per-document gap-buffer size from the PSRAM that is free
 * right now. */
static size_t compute_doc_buf_size(void)
{
    /* ---- Dynamic sizing from free PSRAM ----
     *
     * Above a 2 MB reserve, three quarters of the PSRAM that is still
     * free go to the gap buffer. The rest is for the system that
     * initializes after the editor and for runtime allocations:
     * BLE/Bluedroid (CONFIG_BT_ALLOCATION_FROM_SPIRAM_FIRST), the
     * WiFi/LWIP dynamic pools (CONFIG_SPIRAM_TRY_ALLOCATE_WIFI_LWIP),
     * TLS, the LVGL widget heap growth, the clipboard and cursor-line
     * copy, and the Git client: its object data goes through the SD
     * card, so it only needs fixed buffers plus a merge's per-line
     * tables (a merge that does not fit becomes a whole-file conflict).
     * Before the flat text copy was dropped, the gap buffer and the copy
     * took all of it, leaving only the reserve.
     *
     * Min 64 KB keeps editor_open_file() useful on a tight PSRAM. */
    const size_t MIN_BUF = 64  * 1024;
    const size_t RUNTIME_RESERVE = 2 * 1024 * 1024;
    const size_t ALIGN   = 4 * 1024;

    size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t usable;
    if (free_psram > RUNTIME_RESERVE) {
        usable = free_psram - RUNTIME_RESERVE;
    } else {
        /* PSRAM is tight: fall back to using half of what is free. The
         * min/max clamp below still guarantees we ask for at least
         * MIN_BUF -- if that allocation fails we abort via assert(),
         * which is the same behaviour as before. */
        usable = free_psram / 2;
    }
    size_t per_buf = usable / 4 * 3;
    per_buf &= ~(ALIGN - 1);
    if (per_buf < MIN_BUF) per_buf = MIN_BUF;
    return per_buf;
}

/* Lazily allocate a slot's gap buffer. The first slot is
 * sized from the PSRAM free at editor_init() time (so a single document
 * keeps the historical maximum size); a second concurrently-opened
 * document is sized from whatever PSRAM remains free when it is first
 * acquired, which is the trade-off called out in the split-screen plan
 * ("two large docs must fit"). Returns false on allocation failure. */
static bool doc_ensure_buffers(editor_doc_t *d)
{
    if (d->buf) return true;
    /* For a second/third document, re-derive the size from the PSRAM
     * that is free now -- the first document and the rest of the system
     * have already taken their share -- but never exceed the primary
     * size so editor_get_max_doc_size() stays an upper bound. */
    size_t sz = s_doc_buf_size;
    size_t now = compute_doc_buf_size();
    if (now < sz) sz = now;
    d->buf = (char *)heap_caps_malloc(sz, MALLOC_CAP_SPIRAM);
    if (!d->buf) return false;
    d->buf_size = sz;
    ESP_LOGI(TAG, "Document buffer allocated (%u KB, PSRAM; %u KB free remaining)",
             (unsigned)(sz / 1024),
             (unsigned)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024));
    return true;
}

/* Find an in-use pool slot already bound to path, or NULL. */
static editor_doc_t *doc_find_by_path(const char *path)
{
    if (!path || path[0] == '\0') return NULL;
    for (int i = 0; i < EDITOR_MAX_DOCS; i++) {
        if (s_docs[i].in_use && strcmp(s_docs[i].path, path) == 0)
            return &s_docs[i];
    }
    return NULL;
}

/* Claim a free pool slot, allocate its buffers, and mark it in use.
 * Returns NULL if the pool is full or allocation fails. */
static editor_doc_t *doc_alloc_slot(void)
{
    for (int i = 0; i < EDITOR_MAX_DOCS; i++) {
        if (!s_docs[i].in_use) {
            editor_doc_t *d = &s_docs[i];
            if (!doc_ensure_buffers(d)) return NULL;
            d->in_use   = true;
            d->refcount = 0;
            doc_reset_empty(d);
            return d;
        }
    }
    ESP_LOGW(TAG, "Document pool exhausted (%d slots)", EDITOR_MAX_DOCS);
    return NULL;
}

extern "C" void editor_init(void)
{
    /* Size the per-document buffers exactly once, from the PSRAM free at
     * this point in boot. */
    if (s_doc_buf_size == 0) {
        s_doc_buf_size = compute_doc_buf_size();
        ESP_LOGI(TAG, "Editor initialized (per-doc buffer %u KB; %u KB free PSRAM)",
                 (unsigned)(s_doc_buf_size / 1024),
                 (unsigned)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024));
    }

    /* Ensure there is always an active (empty, untitled) document so the
     * rest of the public API and the single-pane UI keep working without
     * any awareness of the pool. Idempotent: a subsequent call (e.g. the
     * file-browser "open" handler used to call editor_init() to reset)
     * just resets the active document back to empty. */
    if (s_active == NULL) {
        editor_doc_t *d = doc_alloc_slot();
        assert(d && "editor_init: failed to allocate primary document");
        d->refcount = 1;
        s_active = d;
    }
    doc_reset_empty(s_active);
}

extern "C" size_t editor_get_max_doc_size(void)
{
    return s_doc_buf_size;
}

/* ---- Per-file metadata sidecar ----
 *
 * Each opened document gets a tiny key=value text sidecar saved next
 * to it that records the cursor position and visible scroll line so
 * the editor can resume exactly where the user left off. The sidecar
 * file name is derived by prefixing the basename with a dot and
 * appending ".meta" -- e.g. /sdcard/notes.md -> /sdcard/.notes.md.meta.
 *
 * The leading dot keeps the sidecar hidden from sd_card_list_dir()
 * (which already filters dotfiles), and the .meta extension keeps it
 * out of the git_sync commits (which only match documents). The format
 * is line-oriented "key=value" so future metadata fields can be
 * appended without breaking older sidecars.
 */
static void meta_path_for(const char *file_path, char *out, size_t out_sz)
{
    if (!file_path || !out || out_sz == 0) { if (out_sz) out[0] = '\0'; return; }
    const char *slash = strrchr(file_path, '/');
    if (slash) {
        size_t dir_len = (size_t)(slash - file_path) + 1; /* include '/' */
        if (dir_len + 1 + strlen(slash + 1) + 5 + 1 > out_sz) {
            out[0] = '\0';
            return;
        }
        memcpy(out, file_path, dir_len);
        snprintf(out + dir_len, out_sz - dir_len, ".%s.meta", slash + 1);
    } else {
        snprintf(out, out_sz, ".%s.meta", file_path);
    }
}

static void editor_load_meta(void)
{
    if (s_path[0] == '\0') return;
    char meta[320];
    meta_path_for(s_path, meta, sizeof(meta));
    if (meta[0] == '\0') return;
    if (!sd_card_file_exists(meta)) return;

    char *data = NULL;
    size_t len = 0;
    if (sd_card_read_file(meta, &data, &len) != ESP_OK || !data) return;

    long cursor = -1;
    long scroll = -1;
    /* Parse line-oriented key=value pairs in-place. */
    size_t i = 0;
    while (i < len) {
        size_t line_start = i;
        while (i < len && data[i] != '\n' && data[i] != '\r') i++;
        size_t line_end = i;
        while (i < len && (data[i] == '\n' || data[i] == '\r')) i++;
        if (line_end == line_start) continue;
        /* Find '=' separator within the line. */
        size_t eq = line_start;
        while (eq < line_end && data[eq] != '=') eq++;
        if (eq >= line_end) continue;
        size_t key_len = eq - line_start;
        const char *val = data + eq + 1;
        size_t val_len = line_end - eq - 1;
        char vbuf[32];
        if (val_len >= sizeof(vbuf)) val_len = sizeof(vbuf) - 1;
        memcpy(vbuf, val, val_len);
        vbuf[val_len] = '\0';
        if (key_len == 6 && memcmp(data + line_start, "cursor", 6) == 0) {
            cursor = strtol(vbuf, NULL, 10);
        } else if (key_len == 6 && memcmp(data + line_start, "scroll", 6) == 0) {
            scroll = strtol(vbuf, NULL, 10);
        }
    }
    free(data);

    size_t doc_len = content_len();
    if (cursor >= 0) {
        if ((size_t)cursor > doc_len) cursor = (long)doc_len;
        editor_set_cursor((size_t)cursor);
    }
    if (scroll >= 0) {
        /* Clamp against the current document line count in case the
         * file shrank since the sidecar was written (e.g. via a
         * git pull). */
        int lc = editor_get_line_count();
        if (lc <= 0) lc = 1;
        if (scroll > lc - 1) scroll = lc - 1;
        s_scroll_line = (int)scroll;
    }
}

extern "C" void editor_save_meta(void)
{
    if (s_path[0] == '\0') return;
    char meta[320];
    meta_path_for(s_path, meta, sizeof(meta));
    if (meta[0] == '\0') return;

    char body[96];
    int n = snprintf(body, sizeof(body),
                     "cursor=%zu\nscroll=%d\n",
                     s_gap_start, s_scroll_line);
    if (n <= 0) return;
    if ((size_t)n >= sizeof(body)) n = (int)sizeof(body) - 1;
    sd_card_write_file(meta, body, (size_t)n);
}

/* Append one Unicode codepoint as UTF-8 into dst[*dpos..dst_cap).
 * Returns false if the codepoint does not fit. Codepoints in the
 * UTF-16 surrogate range (0xD800..0xDFFF) and codepoints above
 * 0x10FFFF are mapped to U+FFFD so callers do not need to filter. */
static bool append_utf8_cp(char *dst, size_t dst_cap, size_t *dpos, uint32_t cp)
{
    if (cp >= 0xD800 && cp <= 0xDFFF) cp = 0xFFFD;
    if (cp > 0x10FFFF) cp = 0xFFFD;

    size_t need;
    if      (cp < 0x80)    need = 1;
    else if (cp < 0x800)   need = 2;
    else if (cp < 0x10000) need = 3;
    else                   need = 4;

    if (*dpos + need > dst_cap) return false;

    char *p = dst + *dpos;
    switch (need) {
    case 1:
        p[0] = (char)cp;
        break;
    case 2:
        p[0] = (char)(0xC0 | (cp >> 6));
        p[1] = (char)(0x80 | (cp & 0x3F));
        break;
    case 3:
        p[0] = (char)(0xE0 | (cp >> 12));
        p[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        p[2] = (char)(0x80 | (cp & 0x3F));
        break;
    case 4:
        p[0] = (char)(0xF0 | (cp >> 18));
        p[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
        p[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
        p[3] = (char)(0x80 | (cp & 0x3F));
        break;
    }
    *dpos += need;
    return true;
}

/* Reads UTF-16 code units from a file through a small buffer, with
 * one unit of push-back (a high surrogate not followed by a low one). */
typedef struct {
    FILE    *f;
    bool     le;
    uint8_t  buf[1024];
    size_t   pos, have;
    int      pushed;          /* -1 = none */
} u16_reader_t;

static int u16_byte(u16_reader_t *r)
{
    if (r->pos == r->have) {
        r->have = fread(r->buf, 1, sizeof(r->buf), r->f);
        r->pos = 0;
        if (r->have == 0) return -1;
    }
    return r->buf[r->pos++];
}

/* Next code unit, or -1 at the end (an odd trailing byte is dropped). */
static int u16_unit(u16_reader_t *r)
{
    if (r->pushed >= 0) { int u = r->pushed; r->pushed = -1; return u; }
    int a = u16_byte(r);
    if (a < 0) return -1;
    int b = u16_byte(r);
    if (b < 0) return -1;
    return r->le ? (a | (b << 8)) : ((a << 8) | b);
}

/* Transcode the rest of a UTF-16 file (LE or BE) to UTF-8 in dst.
 * Surrogate pairs are decoded; unpaired surrogates become U+FFFD; CR
 * (U+000D) is dropped, since the editor stores LF-only lines. Returns
 * the number of bytes written, or (size_t)-1 if they would not fit. */
static size_t transcode_utf16_file(FILE *f, bool little_endian,
                                   char *dst, size_t dst_cap)
{
    u16_reader_t *r = (u16_reader_t *)malloc(sizeof(u16_reader_t));
    if (!r) return (size_t)-1;
    r->f = f;
    r->le = little_endian;
    r->pos = r->have = 0;
    r->pushed = -1;

    size_t out = 0;
    int u;
    while ((u = u16_unit(r)) >= 0) {
        uint32_t cp;
        if (u >= 0xD800 && u <= 0xDBFF) {
            /* High surrogate -- combine with the following low surrogate. */
            int lo = u16_unit(r);
            if (lo >= 0xDC00 && lo <= 0xDFFF) {
                cp = 0x10000u + ((uint32_t)(u - 0xD800) << 10)
                              + (uint32_t)(lo - 0xDC00);
            } else {
                cp = 0xFFFD;        /* lone high surrogate */
                r->pushed = lo;     /* -1 (end) stays "none" */
            }
        } else if (u >= 0xDC00 && u <= 0xDFFF) {
            cp = 0xFFFD;            /* lone low surrogate */
        } else {
            cp = (uint32_t)u;
        }
        if (cp == 0x000D) continue;
        if (!append_utf8_cp(dst, dst_cap, &out, cp)) { out = (size_t)-1; break; }
    }
    free(r);
    return out;
}

extern "C" esp_err_t editor_open_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) { ESP_LOGE(TAG, "Open failed: %s", path); return ESP_ERR_NOT_FOUND; }
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (fsize < 0) { fclose(f); return ESP_FAIL; }

    /* Detect a Unicode BOM. The editor stores text as UTF-8; files
     * saved as UTF-16 (Windows Notepad's "Unicode") are transcoded,
     * otherwise they would render as random Latin-1 bytes. A UTF-8
     * BOM is dropped so it does not show as a stray glyph. */
    uint8_t bom[3] = { 0, 0, 0 };
    size_t nb = fread(bom, 1, 3, f);
    enum { ENC_UTF8, ENC_UTF16LE, ENC_UTF16BE } enc = ENC_UTF8;
    long skip = 0;
    if (nb >= 2 && bom[0] == 0xFF && bom[1] == 0xFE)      { enc = ENC_UTF16LE; skip = 2; }
    else if (nb >= 2 && bom[0] == 0xFE && bom[1] == 0xFF) { enc = ENC_UTF16BE; skip = 2; }
    else if (nb >= 3 && bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF) skip = 3;
    fseek(f, skip, SEEK_SET);
    size_t payload = (size_t)(fsize - skip);

    /* Same limit as editing: the buffer less a 64-byte margin. A UTF-16
     * file's UTF-8 size is only known once transcoded. */
    size_t cap = s_buf_size - 64;
    if (enc == ENC_UTF8 && payload > cap) {
        ESP_LOGW(TAG, "Refusing to open %s: %ld bytes > editor buffer (%u KB)",
                 path, fsize, (unsigned)(s_buf_size / 1024));
        fclose(f);
        return ESP_ERR_NO_MEM;
    }

    /* The file is read straight into the gap buffer, with no temporary
     * copy. When the gap can hold it, it goes into the gap, so the
     * current text survives a failure; otherwise the document is
     * emptied first (its text is being replaced anyway) and, should
     * the read fail, left empty and untitled. */
    size_t gap = s_gap_end - s_gap_start;
    size_t need = (enc == ENC_UTF8) ? payload : payload / 2 * 3;
    bool in_gap = gap >= need && gap >= 1;
    char *dst = in_gap ? s_buf + s_gap_start : s_buf;
    size_t room = in_gap ? (gap < cap ? gap : cap) : cap;
    if (!in_gap) {
        s_gap_start = 0;
        s_gap_end = s_buf_size;
        s_active->modified = false;
    }

    size_t written;
    if (enc == ENC_UTF8) {
        written = fread(dst, 1, payload, f) == payload ? payload : (size_t)-1;
    } else {
        written = transcode_utf16_file(f, enc == ENC_UTF16LE, dst, room);
    }
    bool read_error = ferror(f);
    fclose(f);
    if (written == (size_t)-1 || read_error) {
        esp_err_t err = (written == (size_t)-1 && !read_error && enc != ENC_UTF8)
                        ? ESP_ERR_NO_MEM : ESP_FAIL;
        ESP_LOGW(TAG, "Open %s failed (%s)", path,
                 err == ESP_ERR_NO_MEM ? "too large" : "read error");
        if (!in_gap) doc_reset_empty(s_active);
        return err;
    }
    if (in_gap) memmove(s_buf, dst, written);
    s_gap_start = written;
    s_gap_end   = s_buf_size;

    strncpy(s_path, path, sizeof(s_path) - 1);
    s_path[sizeof(s_path) - 1] = '\0';
    s_modified = false;
    s_format = EDITOR_DOC_MARKDOWN;
    editor_path_format(path, &s_format);
    s_scroll_line = 0;
    s_sel_anchor = -1;
    invalidate_caches();
    s_mode = EDITOR_MODE_EDITING;

    /* Restore cursor position and scroll line from the metadata
     * sidecar (no-op if the file was never opened before). */
    editor_load_meta();

    ESP_LOGI(TAG, "Opened: %s (%ld bytes on disk, %zu stored)",
             path, fsize, s_gap_start);
    return ESP_OK;
}

extern "C" esp_err_t editor_save_file(void)
{
    if (s_path[0] == '\0') return ESP_ERR_INVALID_STATE;
    esp_err_t ret = sd_card_write_file_parts(s_path, s_buf, s_gap_start,
                                             s_buf + s_gap_end, s_buf_size - s_gap_end);
    if (ret == ESP_OK) { s_modified = false; ESP_LOGI(TAG, "Saved: %s", s_path); }
    return ret;
}

extern "C" esp_err_t editor_save_file_as(const char *path)
{
    strncpy(s_path, path, sizeof(s_path) - 1);
    s_path[sizeof(s_path) - 1] = '\0';
    /* The extension picks the format; any other name keeps the mode
     * the document was written in. */
    editor_path_format(path, &s_format);
    return editor_save_file();
}

static bool has_ext(const char *path, size_t n, const char *ext)
{
    size_t el = strlen(ext);
    return n > el && strcasecmp(path + n - el, ext) == 0;
}

extern "C" bool editor_path_format(const char *path, editor_doc_format_t *out)
{
    size_t n = path ? strlen(path) : 0;
    editor_doc_format_t f;
    if      (has_ext(path, n, ".md"))       f = EDITOR_DOC_MARKDOWN;
    else if (has_ext(path, n, ".fountain")) f = EDITOR_DOC_FOUNTAIN;
    else if (has_ext(path, n, ".txt"))      f = EDITOR_DOC_TEXT;
    else return false;
    if (out) *out = f;
    return true;
}

extern "C" const char *editor_format_ext(editor_doc_format_t f)
{
    switch (f) {
    case EDITOR_DOC_FOUNTAIN: return "fountain";
    case EDITOR_DOC_TEXT:     return "txt";
    default:                  return "md";
    }
}

extern "C" editor_doc_format_t editor_get_format(void)
{
    return s_active ? s_format : EDITOR_DOC_MARKDOWN;
}

extern "C" void editor_set_format(editor_doc_format_t f)
{
    if (s_active) s_format = f;
}

extern "C" esp_err_t editor_delete_file(const char *path)
{
    if (!path || !path[0]) return ESP_ERR_INVALID_ARG;
    /* A document still open (e.g. in the other split pane) would be
     * written straight back by the next save or auto-save. */
    for (int i = 0; i < EDITOR_MAX_DOCS; i++) {
        if (s_docs[i].in_use && strcmp(s_docs[i].path, path) == 0)
            return ESP_ERR_INVALID_STATE;
    }
    esp_err_t err = sd_card_delete_file(path);
    if (err != ESP_OK) return err;

    char meta[320];
    meta_path_for(path, meta, sizeof(meta));
    if (meta[0] && sd_card_file_exists(meta)) sd_card_delete_file(meta);
    ESP_LOGI(TAG, "Deleted: %s", path);
    return ESP_OK;
}

extern "C" esp_err_t editor_rename_file(const char *old_path, const char *new_path)
{
    if (!old_path || !new_path || !old_path[0] || !new_path[0])
        return ESP_ERR_INVALID_ARG;
    if (strlen(new_path) >= sizeof(s_docs[0].path)) return ESP_ERR_INVALID_SIZE;
    /* A case-only change names the same file on FAT. */
    if (strcasecmp(old_path, new_path) != 0 && sd_card_file_exists(new_path))
        return ESP_ERR_INVALID_STATE;
    esp_err_t err = sd_card_rename(old_path, new_path);
    if (err != ESP_OK) return err;

    /* The metadata sidecar follows its file. Best effort: a missing
     * or unmovable sidecar only loses the resume position. */
    char old_meta[320], new_meta[320];
    meta_path_for(old_path, old_meta, sizeof(old_meta));
    meta_path_for(new_path, new_meta, sizeof(new_meta));
    if (old_meta[0] && new_meta[0] && sd_card_file_exists(old_meta)) {
        if (sd_card_file_exists(new_meta)) sd_card_delete_file(new_meta);
        if (sd_card_rename(old_meta, new_meta) != ESP_OK)
            ESP_LOGW(TAG, "Rename: could not move %s", old_meta);
    }

    /* An open document keeps editing the renamed file, in the format
     * its new extension names. */
    for (int i = 0; i < EDITOR_MAX_DOCS; i++) {
        editor_doc_t *d = &s_docs[i];
        if (!d->in_use || strcmp(d->path, old_path) != 0) continue;
        strncpy(d->path, new_path, sizeof(d->path) - 1);
        d->path[sizeof(d->path) - 1] = '\0';
        editor_path_format(new_path, &d->format);
    }
    ESP_LOGI(TAG, "Renamed: %s -> %s", old_path, new_path);
    return ESP_OK;
}

/* True if path lies inside dir ("<dir>/..."). */
static bool path_in_dir(const char *path, const char *dir)
{
    size_t n = strlen(dir);
    return n && strncmp(path, dir, n) == 0 && path[n] == '/';
}

extern "C" bool editor_dir_has_open_doc(const char *dir)
{
    if (!dir || !dir[0]) return false;
    for (int i = 0; i < EDITOR_MAX_DOCS; i++) {
        if (s_docs[i].in_use && s_docs[i].path[0] && path_in_dir(s_docs[i].path, dir))
            return true;
    }
    return false;
}

extern "C" esp_err_t editor_rename_dir(const char *old_dir, const char *new_dir)
{
    if (!old_dir || !new_dir || !old_dir[0] || !new_dir[0])
        return ESP_ERR_INVALID_ARG;
    /* Every open path must still fit after the change. */
    size_t ol = strlen(old_dir), nl = strlen(new_dir);
    for (int i = 0; i < EDITOR_MAX_DOCS; i++) {
        const editor_doc_t *d = &s_docs[i];
        if (d->in_use && path_in_dir(d->path, old_dir) &&
            strlen(d->path) - ol + nl >= sizeof(d->path))
            return ESP_ERR_INVALID_SIZE;
    }
    if (strcasecmp(old_dir, new_dir) != 0 && sd_card_file_exists(new_dir))
        return ESP_ERR_INVALID_STATE;
    esp_err_t err = sd_card_rename(old_dir, new_dir);
    if (err != ESP_OK) return err;
    for (int i = 0; i < EDITOR_MAX_DOCS; i++) {
        editor_doc_t *d = &s_docs[i];
        if (!d->in_use || !path_in_dir(d->path, old_dir)) continue;
        char tail[sizeof(d->path)];
        strlcpy(tail, d->path + ol, sizeof(tail));
        snprintf(d->path, sizeof(d->path), "%s%s", new_dir, tail);
    }
    ESP_LOGI(TAG, "Renamed folder: %s -> %s", old_dir, new_dir);
    return ESP_OK;
}

extern "C" void editor_new_file(void)
{
    s_gap_start = 0;
    s_gap_end   = s_buf_size;
    s_path[0]   = '\0';
    s_modified  = false;
    s_format    = EDITOR_DOC_MARKDOWN;
    s_scroll_line = 0;
    s_sel_anchor = -1;
    invalidate_caches();
    s_mode = EDITOR_MODE_EDITING;
}

extern "C" void editor_close_file(void)
{
    /* Persist cursor/scroll metadata before we drop the path so the
     * next open can resume at the same position. */
    editor_save_meta();
    s_gap_start = 0;
    s_gap_end   = s_buf_size;
    s_path[0]   = '\0';
    s_modified  = false;
    s_scroll_line = 0;
    s_sel_anchor = -1;
    invalidate_caches();
    s_mode = EDITOR_MODE_NORMAL;
}

extern "C" const char *editor_get_file_path(void) { return s_path[0] ? s_path : NULL; }
extern "C" bool editor_is_modified(void) { return s_modified; }

/* ---- Document pool public API ----
 *
 * These let a multi-pane UI hold more than one open document. The rest
 * of the public API above always operates on whichever document is
 * active (editor_set_active / editor_get_active). */

extern "C" editor_doc_t *editor_get_active(void) { return s_active; }

extern "C" void editor_set_active(editor_doc_t *doc) { s_active = doc; }

extern "C" const char *editor_doc_get_path(const editor_doc_t *doc)
{
    return (doc && doc->path[0]) ? doc->path : NULL;
}

extern "C" bool editor_doc_is_modified(const editor_doc_t *doc)
{
    return doc && doc->modified;
}

/* First in-use slot, used as a fallback active document. */
static editor_doc_t *pick_any_active(void)
{
    for (int i = 0; i < EDITOR_MAX_DOCS; i++) {
        if (s_docs[i].in_use) return &s_docs[i];
    }
    return NULL;
}

extern "C" editor_doc_t *editor_doc_acquire(const char *path)
{
    /* Two panes opening the same path share one editor_doc_t (and one
     * underlying buffer): hand back the existing handle, bump its
     * reference count, and make it active. */
    editor_doc_t *existing = doc_find_by_path(path);
    if (existing) {
        existing->refcount++;
        s_active = existing;
        return existing;
    }

    editor_doc_t *slot = doc_alloc_slot();
    if (!slot) return NULL;
    slot->refcount = 1;
    s_active = slot;   /* editor_open_file / editor_new_file act on active */

    if (path && path[0]) {
        if (editor_open_file(path) != ESP_OK) {
            /* Roll back the slot allocation and restore a valid active. */
            slot->in_use   = false;
            slot->refcount = 0;
            slot->path[0]  = '\0';
            s_active = pick_any_active();
            return NULL;
        }
    } else {
        editor_new_file();
    }
    return slot;
}

extern "C" void editor_doc_release(editor_doc_t *doc)
{
    if (!doc || !doc->in_use) return;
    if (doc->refcount > 0) doc->refcount--;
    if (doc->refcount > 0) return;   /* still referenced by another pane */

    /* Last reference: persist resume metadata, then free the slot. The
     * gap buffer is kept allocated for reuse by a future
     * acquire so we do not thrash large PSRAM allocations. */
    editor_doc_save_meta(doc);
    doc->in_use   = false;
    doc->path[0]  = '\0';
    doc->modified = false;
    if (s_active == doc) s_active = pick_any_active();
}

extern "C" esp_err_t editor_doc_save(editor_doc_t *doc)
{
    if (!doc || doc->path[0] == '\0') return ESP_ERR_INVALID_STATE;
    editor_doc_t *prev = s_active;
    s_active = doc;
    esp_err_t ret = editor_save_file();
    s_active = prev;
    return ret;
}

extern "C" void editor_doc_save_meta(editor_doc_t *doc)
{
    if (!doc || doc->path[0] == '\0') return;
    editor_doc_t *prev = s_active;
    s_active = doc;
    editor_save_meta();
    s_active = prev;
}

extern "C" void editor_doc_foreach(void (*cb)(editor_doc_t *doc, void *ctx),
                                   void *ctx)
{
    if (!cb) return;
    for (int i = 0; i < EDITOR_MAX_DOCS; i++) {
        if (s_docs[i].in_use) cb(&s_docs[i], ctx);
    }
}

extern "C" void editor_discard_all_changes(void)
{
    editor_doc_t *prev = s_active;
    for (int i = 0; i < EDITOR_MAX_DOCS; i++) {
        editor_doc_t *d = &s_docs[i];
        if (!d->in_use || !d->modified) continue;
        s_active = d;
        if (d->path[0]) {
            /* editor_open_file() reads d->path (aliased as s_path) and
             * rewrites the buffer from disk, clearing d->modified. Copy
             * the path first: on success the string is unchanged, but
             * keeping a local copy is clearer than relying on that. */
            char path[sizeof(d->path)];
            strncpy(path, d->path, sizeof(path));
            path[sizeof(path) - 1] = '\0';
            if (editor_open_file(path) != ESP_OK) {
                ESP_LOGW(TAG, "Discard: could not reload %s -- keeping edits",
                         path);
            }
        } else {
            editor_doc_format_t format = d->format;
            editor_new_file();
            d->format = format;
        }
    }
    s_active = (prev && prev->in_use) ? prev : pick_any_active();
    invalidate_caches();
}

static void move_gap_to(size_t pos);

/* ---- Reading the text ----
 *
 * The text lives only in the gap buffer: [0, gap_start) before the gap
 * and [gap_end, buf_size) after it. Logical offsets skip the gap. */

/* Pointer to the byte at logical offset p (p < content_len()). */
static inline const char *phys(size_t p)
{
    return (p < s_gap_start) ? s_buf + p : s_buf + p + (s_gap_end - s_gap_start);
}

/* Logical offset of the first byte c at or after from, or the content
 * length when there is none. */
static size_t find_byte_fwd(size_t from, char c)
{
    size_t len = content_len();
    if (from < s_gap_start) {
        const char *q = (const char *)memchr(s_buf + from, c, s_gap_start - from);
        if (q) return (size_t)(q - s_buf);
        from = s_gap_start;
    }
    if (from >= len) return len;
    const char *base = s_buf + s_gap_end - s_gap_start;   /* base + p = phys(p) */
    const char *q = (const char *)memchr(base + from, c, len - from);
    return q ? (size_t)(q - base) : len;
}

/* Copy the logical range [from, to) to dst. */
static void copy_range(size_t from, size_t to, char *dst)
{
    if (from < s_gap_start) {
        size_t e = to < s_gap_start ? to : s_gap_start;
        memcpy(dst, s_buf + from, e - from);
        dst += e - from;
        from = e;
    }
    if (from < to) memcpy(dst, phys(from), to - from);
}

extern "C" size_t editor_get_length(void)
{
    return s_active && s_buf ? content_len() : 0;
}

extern "C" char editor_char_at(size_t off)
{
    if (off >= content_len()) return '\0';
    return char_at(off);
}

extern "C" const char *editor_text_begin(size_t *out_len)
{
    s_active->text_cursor = s_gap_start;
    size_t len = content_len();
    move_gap_to(len);
    if (out_len) *out_len = len;
    return s_buf;
}

extern "C" void editor_text_end(void)
{
    size_t c = s_active->text_cursor;
    if (c > content_len()) c = content_len();
    move_gap_to(c);
}

extern "C" size_t editor_get_cursor(void) { return s_gap_start; }

static size_t find_line_start(size_t pos);

extern "C" void editor_get_cursor_pos(int *line, int *col)
{
    if (line) *line = s_nl_before;
    if (col) {
        /* Count only the first byte of each UTF-8 character.
         * Continuation bytes have the form 10xxxxxx (0x80..0xBF). */
        int c = 0;
        for (size_t i = find_line_start(s_gap_start); i < s_gap_start; i++) {
            if ((s_buf[i] & 0xC0) != 0x80) c++;
        }
        *col = c;
    }
}

extern "C" int editor_get_line_count(void)
{
    return s_nl_total + 1;
}

extern "C" const char *editor_get_line_off(int line_num, size_t *out_len,
                                           size_t *out_off)
{
    size_t total = content_len();
    if (line_num < 0 || line_num > s_nl_total) {
        if (out_len) *out_len = 0;
        if (out_off) *out_off = total;
        return "";
    }

    /* Walk from the nearest line whose start is known: the cached
     * line, the cursor line or the first line. */
    int    cur = 0;
    size_t off = 0;
    int    cl  = s_nl_before;
    if (abs(line_num - cl) < line_num) {
        cur = cl;
        off = find_line_start(s_gap_start);
    }
    if (s_line_cache_num >= 0 && s_line_cache_offset <= total &&
        abs(line_num - s_line_cache_num) < abs(line_num - cur)) {
        cur = s_line_cache_num;
        off = s_line_cache_offset;
    }
    while (cur < line_num) {
        size_t nl = find_byte_fwd(off, '\n');
        if (nl >= total) break;   /* cannot happen: line_num <= s_nl_total */
        off = nl + 1;
        cur++;
    }
    while (cur > line_num) {
        /* off is a line start after a '\n'; find the start of the line
         * before it. */
        size_t q = off - 1;
        while (q > 0 && char_at(q - 1) != '\n') q--;
        off = q;
        cur--;
    }
    s_line_cache_num    = line_num;
    s_line_cache_offset = off;
    size_t end = find_byte_fwd(off, '\n');
    size_t n = end - off;
    if (out_len) *out_len = n;
    if (out_off) *out_off = off;

    if (end <= s_gap_start || off >= s_gap_start) return n ? phys(off) : "";

    /* The gap splits this line (it is the cursor line): copy it. */
    if (n + 1 > s_active->line_buf_cap) {
        size_t cap = n + 1 + 256;
        char *nb = (char *)heap_caps_realloc(s_active->line_buf, cap, MALLOC_CAP_SPIRAM);
        if (!nb) nb = (char *)heap_caps_realloc(s_active->line_buf, cap, MALLOC_CAP_DEFAULT);
        if (!nb) {
            /* Out of memory: show the part before the cursor. */
            if (out_len) *out_len = s_gap_start - off;
            return s_buf + off;
        }
        s_active->line_buf = nb;
        s_active->line_buf_cap = cap;
    }
    copy_range(off, end, s_active->line_buf);
    s_active->line_buf[n] = '\0';
    return s_active->line_buf;
}

extern "C" const char *editor_get_line(int line_num, size_t *out_len)
{
    return editor_get_line_off(line_num, out_len, NULL);
}

extern "C" void editor_change_mark_reset(int slot)
{
    if (slot >= 0 && slot < EDITOR_CHANGE_MARKS) s_active->change_mark[slot] = SIZE_MAX;
}

extern "C" size_t editor_change_mark_get(int slot)
{
    if (slot < 0 || slot >= EDITOR_CHANGE_MARKS) return 0;
    return s_active->change_mark[slot];
}

extern "C" int editor_get_scroll_line(void) { return s_scroll_line; }
extern "C" void editor_set_scroll_line(int line) { s_scroll_line = line < 0 ? 0 : line; }

/* Raw selection-anchor accessors. Used by the split-screen UI to keep a
 * separate selection per pane when the same document is shown in both
 * panes (see editor_doc_is_shared). A value < 0 means "no selection". */
extern "C" int editor_get_sel_anchor(void) { return s_sel_anchor; }
extern "C" void editor_set_sel_anchor(int anchor)
{
    if (anchor < 0) { s_sel_anchor = -1; return; }
    size_t len = content_len();
    if ((size_t)anchor > len) anchor = (int)len;
    s_sel_anchor = anchor;
}

/* ---- Cursor movement ---- */

/* Byte-level gap shifts (internal primitives). */
static void gap_shift_left(void)
{
    s_gap_end--;
    s_buf[s_gap_end] = s_buf[s_gap_start - 1];
    s_gap_start--;
    if (s_buf[s_gap_end] == '\n') s_nl_before--;
}

static void gap_shift_right(void)
{
    s_buf[s_gap_start] = s_buf[s_gap_end];
    if (s_buf[s_gap_start] == '\n') s_nl_before++;
    s_gap_start++;
    s_gap_end++;
}

/* Move the gap to a given logical position using memmove (faster than
 * byte-by-byte shifting for large distances). */
static void move_gap_to(size_t pos)
{
    if (pos == s_gap_start) return;
    if (pos < s_gap_start) {
        size_t n = s_gap_start - pos;
        s_nl_before -= count_nl(s_buf + pos, n);
        memmove(s_buf + s_gap_end - n, s_buf + pos, n);
        s_gap_start = pos;
        s_gap_end -= n;
    } else {
        size_t n = pos - s_gap_start;
        s_nl_before += count_nl(s_buf + s_gap_end, n);
        memmove(s_buf + s_gap_start, s_buf + s_gap_end, n);
        s_gap_start += n;
        s_gap_end += n;
    }
}

/* Count UTF-8 characters in s_buf[start..end) (before the gap). */
static int utf8_char_count(size_t start, size_t end)
{
    int n = 0;
    for (size_t i = start; i < end; i++) {
        if ((s_buf[i] & 0xC0) != 0x80) n++;
    }
    return n;
}

extern "C" void editor_move_left(void)
{
    if (s_gap_start == 0) return;
    /* Move back one byte, then keep going while we are on a
     * UTF-8 continuation byte (10xxxxxx) to land on the start
     * byte of the previous character. */
    do {
        gap_shift_left();
    } while (s_gap_start > 0 && (s_buf[s_gap_start] & 0xC0) == 0x80);
}

extern "C" void editor_move_right(void)
{
    if (s_gap_end >= s_buf_size) return;
    /* Move forward one byte, then skip any continuation bytes
     * so we land at the start of the next character. */
    gap_shift_right();
    while (s_gap_end < s_buf_size && (s_buf[s_gap_end] & 0xC0) == 0x80)
        gap_shift_right();
}

static size_t find_line_start(size_t pos)
{
    if (pos == 0) return 0;
    size_t i = pos - 1;
    while (i > 0 && s_buf[i] != '\n') i--;
    return (s_buf[i] == '\n') ? i + 1 : 0;
}

extern "C" void editor_move_up(void)
{
    size_t line_start = find_line_start(s_gap_start);
    if (line_start == 0) return; /* already on first line */

    /* Remember the cursor column in UTF-8 characters */
    int col_chars = utf8_char_count(line_start, s_gap_start);

    /* Move to beginning of current line (byte-level) */
    while (s_gap_start > line_start) gap_shift_left();

    /* Step past the '\n' onto the end of the previous line */
    gap_shift_left();

    /* Find the start of the previous line and move there */
    size_t prev_start = find_line_start(s_gap_start);
    while (s_gap_start > prev_start) gap_shift_left();

    /* Advance by col_chars characters (or to end of line) */
    for (int i = 0; i < col_chars; i++) {
        if (s_gap_end >= s_buf_size || s_buf[s_gap_end] == '\n') break;
        editor_move_right();
    }
}

extern "C" void editor_move_down(void)
{
    size_t line_start = find_line_start(s_gap_start);

    /* Remember the cursor column in UTF-8 characters */
    int col_chars = utf8_char_count(line_start, s_gap_start);

    /* Move to end of current line (byte-level) */
    while (s_gap_end < s_buf_size && s_buf[s_gap_end] != '\n')
        gap_shift_right();

    /* If at end of buffer, there is no next line */
    if (s_gap_end >= s_buf_size) return;

    /* Skip the '\n' to reach the start of the next line */
    gap_shift_right();

    /* Advance by col_chars characters (or to end of line) */
    for (int i = 0; i < col_chars; i++) {
        if (s_gap_end >= s_buf_size || s_buf[s_gap_end] == '\n') break;
        editor_move_right();
    }
}

extern "C" void editor_move_home(void)
{
    size_t line_start = find_line_start(s_gap_start);
    while (s_gap_start > line_start) editor_move_left();
}

extern "C" void editor_move_end(void)
{
    while (s_gap_end < s_buf_size && s_buf[s_gap_end] != '\n')
        editor_move_right();
}

extern "C" void editor_move_page_up(int visible_lines)
{
    for (int i = 0; i < visible_lines; i++) editor_move_up();
}

extern "C" void editor_move_page_down(int visible_lines)
{
    for (int i = 0; i < visible_lines; i++) editor_move_down();
}

extern "C" void editor_move_doc_start(void)
{
    move_gap_to(0);
}

extern "C" void editor_move_doc_end(void)
{
    move_gap_to(content_len());
}

extern "C" void editor_move_word_left(void)
{
    if (s_gap_start == 0) return;
    editor_move_left();
    /* Skip whitespace */
    while (s_gap_start > 0 && (s_buf[s_gap_start - 1] == ' ' || s_buf[s_gap_start - 1] == '\t'))
        editor_move_left();
    /* Skip word chars */
    while (s_gap_start > 0 && s_buf[s_gap_start - 1] != ' ' &&
           s_buf[s_gap_start - 1] != '\t' && s_buf[s_gap_start - 1] != '\n')
        editor_move_left();
}

extern "C" void editor_move_word_right(void)
{
    if (s_gap_end >= s_buf_size) return;
    /* Skip word chars */
    while (s_gap_end < s_buf_size && s_buf[s_gap_end] != ' ' &&
           s_buf[s_gap_end] != '\t' && s_buf[s_gap_end] != '\n')
        editor_move_right();
    /* Skip whitespace */
    while (s_gap_end < s_buf_size && (s_buf[s_gap_end] == ' ' || s_buf[s_gap_end] == '\t'))
        editor_move_right();
}

/* ---- Editing ---- */

extern "C" bool editor_insert_char(char c)
{
    /* Reserve a small safety margin so we never completely fill the
     * buffer (matches editor_open_file() / editor_replace_range()). */
    if (content_len() + 1 > s_buf_size - 64) {
        ESP_LOGW(TAG, "Editor buffer full (%u KB), rejecting insert",
                 (unsigned)(s_buf_size / 1024));
        return false;
    }
    ensure_gap(1);
    if (s_gap_start >= s_gap_end) return false;
    size_t pos = s_gap_start;
    insert_at_gap(&c, 1);
    content_changed(pos);
    return true;
}

extern "C" bool editor_insert_text(const char *text, size_t len)
{
    if (!text || len == 0) return true;
    if (content_len() + len > s_buf_size - 64) {
        ESP_LOGW(TAG, "Editor buffer full (%u KB), rejecting %zu-byte insert",
                 (unsigned)(s_buf_size / 1024), len);
        return false;
    }
    size_t pos = s_gap_start;
    insert_at_gap(text, len);
    content_changed(pos);
    return true;
}

extern "C" size_t editor_peek_before(char *out, size_t max)
{
    /* The bytes before the cursor are s_buf[0 .. s_gap_start). */
    size_t n = 0;
    while (n < max && n < s_gap_start && s_buf[s_gap_start - n - 1] != '\n') n++;
    memcpy(out, s_buf + s_gap_start - n, n);
    return n;
}

extern "C" bool editor_replace_before(size_t del, const char *text)
{
    if (del > s_gap_start) return false;
    size_t len = text ? strlen(text) : 0;
    if (content_len() - del + len > s_buf_size - 64) {
        ESP_LOGW(TAG, "Editor buffer full (%u KB), rejecting %zu-byte insert",
                 (unsigned)(s_buf_size / 1024), len);
        return false;
    }
    remove_before_gap(s_gap_start - del);
    s_sel_anchor = -1;
    size_t pos = s_gap_start;
    if (len > 0) insert_at_gap(text, len);
    content_changed(pos);
    return true;
}

extern "C" bool editor_insert_newline(void)
{
    return editor_insert_char('\n');
}

extern "C" void editor_delete_back(void)
{
    if (s_gap_start == 0) return;
    /* Remove all bytes of the previous UTF-8 character by expanding
     * the gap leftward past continuation bytes and the start byte. */
    size_t ns = s_gap_start;
    do {
        ns--;
    } while (ns > 0 && (s_buf[ns] & 0xC0) == 0x80);
    remove_before_gap(ns);
    content_changed(ns);
}

extern "C" void editor_delete_forward(void)
{
    if (s_gap_end >= s_buf_size) return;
    /* Remove the start byte and any following continuation bytes. */
    size_t ne = s_gap_end + 1;
    while (ne < s_buf_size && (s_buf[ne] & 0xC0) == 0x80)
        ne++;
    remove_after_gap(ne);
    content_changed(s_gap_start);
}

extern "C" void editor_delete_line(void)
{
    editor_move_home();
    size_t end = s_gap_end;
    while (end < s_buf_size && s_buf[end] != '\n') end++;
    if (end < s_buf_size) end++; /* include the newline */
    remove_after_gap(end);
    content_changed(s_gap_start);
}

extern "C" void editor_set_cursor(size_t pos)
{
    size_t len = content_len();
    if (pos > len) pos = len;
    move_gap_to(pos);
}

/* ---- Find / Replace ----
 *
 * Forward case-sensitive substring search starting at byte offset
 * from_pos in the flat text. Returns the byte offset of the first
 * match at or after from_pos, or -1 when nothing matches. An empty
 * needle is treated as "no match" so callers can safely loop on the
 * return value.
 */
extern "C" int editor_find(const char *needle, size_t from_pos)
{
    if (!needle || needle[0] == '\0') return -1;
    size_t total = content_len();
    size_t nlen = strlen(needle);
    if (nlen > total) return -1;
    if (from_pos > total - nlen) return -1;
    /* Jump between occurrences of the first byte, then compare the
     * rest across the gap. */
    for (size_t i = from_pos; i + nlen <= total; i++) {
        i = find_byte_fwd(i, needle[0]);
        if (i + nlen > total) return -1;
        size_t k = 1;
        while (k < nlen && char_at(i + k) == needle[k]) k++;
        if (k == nlen) return (int)i;
    }
    return -1;
}

/* Replace the byte range [start, end) with the supplied replacement
 * text. The cursor is left positioned at the end of the replacement,
 * any active selection is cleared, and the document is marked
 * modified. Returns ESP_OK on success or ESP_ERR_NO_MEM if the
 * replacement would overflow the gap buffer. */
extern "C" esp_err_t editor_replace_range(size_t start, size_t end,
                                          const char *replacement)
{
    size_t len = content_len();
    if (start > len) start = len;
    if (end > len) end = len;
    if (start > end) { size_t t = start; start = end; end = t; }

    size_t rlen = replacement ? strlen(replacement) : 0;
    size_t old_span = end - start;
    /* New content size must fit in the buffer with the editor's usual
     * 64-byte safety margin (matches editor_open_file()). */
    if (len - old_span + rlen > s_buf_size - 64) return ESP_ERR_NO_MEM;

    /* Delete the old range, then insert the replacement at the same
     * position. Going through the existing helpers preserves all
     * gap/cache invariants. */
    move_gap_to(end);
    remove_before_gap(start);
    s_sel_anchor = -1;
    if (rlen > 0) {
        ensure_gap(rlen);
        insert_at_gap(replacement, rlen);
    }
    content_changed(start);
    return ESP_OK;
}

/* ---- Selection ---- */

extern "C" bool editor_selection_active(void)
{
    return s_sel_anchor >= 0 && (size_t)s_sel_anchor != s_gap_start;
}

extern "C" void editor_set_selection_anchor(void)
{
    if (s_sel_anchor < 0) s_sel_anchor = (int)s_gap_start;
}

extern "C" void editor_clear_selection(void)
{
    s_sel_anchor = -1;
}

extern "C" void editor_get_selection_range(size_t *start, size_t *end)
{
    if (!editor_selection_active()) {
        if (start) *start = s_gap_start;
        if (end)   *end   = s_gap_start;
        return;
    }
    size_t a = (size_t)s_sel_anchor;
    size_t c = s_gap_start;
    if (start) *start = (a < c) ? a : c;
    if (end)   *end   = (a > c) ? a : c;
}

extern "C" bool editor_delete_selection(void)
{
    if (!editor_selection_active()) return false;
    size_t sel_s, sel_e;
    editor_get_selection_range(&sel_s, &sel_e);
    move_gap_to(sel_e);
    remove_before_gap(sel_s);
    s_sel_anchor = -1;
    content_changed(sel_s);
    return true;
}

extern "C" void editor_select_all(void)
{
    s_sel_anchor = 0;
    move_gap_to(content_len());
}

/* ---- Clipboard ---- */

extern "C" bool editor_copy(void)
{
    if (!editor_selection_active()) return false;
    size_t sel_s, sel_e;
    editor_get_selection_range(&sel_s, &sel_e);
    size_t len = sel_e - sel_s;

    if (s_clipboard) { free(s_clipboard); s_clipboard = NULL; }
    s_clip_len = 0;
    s_clip_in_file = false;
    s_clipboard = (char *)heap_caps_malloc(len + 1, MALLOC_CAP_SPIRAM);
    if (s_clipboard) {
        copy_range(sel_s, sel_e, s_clipboard);
        s_clipboard[len] = '\0';
        s_clip_len = len;
        return true;
    }
    /* Not enough PSRAM: the selection goes to the SD card, as the part
     * before the gap and the part after it. */
    size_t a_end = sel_e < s_gap_start ? sel_e : s_gap_start;
    size_t alen = sel_s < a_end ? a_end - sel_s : 0;
    size_t bstart = sel_s + alen;
    if (sd_card_write_file_parts(CLIPBOARD_FILE, alen ? s_buf + sel_s : NULL, alen,
                                 bstart < sel_e ? phys(bstart) : NULL, sel_e - bstart) != ESP_OK) {
        ESP_LOGW(TAG, "Copy of %zu bytes failed: no memory and no SD card", len);
        return false;
    }
    s_clip_len = len;
    s_clip_in_file = true;
    return true;
}

extern "C" bool editor_cut(void)
{
    if (!editor_copy()) return false;
    editor_delete_selection();
    return true;
}

extern "C" bool editor_paste(void)
{
    if (s_clip_len == 0) return true;
    if (!s_clip_in_file) {
        editor_delete_selection();
        return editor_insert_text(s_clipboard, s_clip_len);
    }
    /* Read the clipboard file straight into the gap. */
    size_t sel_s = s_gap_start, sel_e = s_gap_start;
    if (editor_selection_active()) editor_get_selection_range(&sel_s, &sel_e);
    if (content_len() - (sel_e - sel_s) + s_clip_len > s_buf_size - 64) {
        ESP_LOGW(TAG, "Editor buffer full (%u KB), rejecting %zu-byte paste",
                 (unsigned)(s_buf_size / 1024), s_clip_len);
        return false;
    }
    FILE *f = fopen(CLIPBOARD_FILE, "rb");
    if (!f) return false;
    editor_delete_selection();
    size_t pos = s_gap_start;
    size_t got = fread(s_buf + s_gap_start, 1, s_clip_len, f);
    fclose(f);
    if (got == 0) return false;
    int n = count_nl(s_buf + s_gap_start, got);
    s_nl_before += n;
    s_nl_total  += n;
    s_gap_start += got;
    content_changed(pos);
    return got == s_clip_len;
}

extern "C" bool editor_has_clipboard(void)
{
    return s_clip_len > 0;
}

extern "C" editor_mode_t editor_get_mode(void) { return s_mode; }
extern "C" void editor_set_mode(editor_mode_t mode) { s_mode = mode; }
