#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <esp_log.h>
#include <esp_heap_caps.h>

#include "editor.h"
#include "sd_card.h"
#include "git_sync.h"

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
    char  *flat;          /* flattened text cache */
    size_t gap_start;     /* gap start == cursor position */
    size_t gap_end;
    size_t buf_size;      /* allocated size of buf (and flat - 1) */

    char   path[256];     /* backing file path, "" when untitled */
    bool   modified;
    bool   flat_dirty;
    editor_mode_t mode;
    int    scroll_line;
    int    sel_anchor;    /* logical byte offset, -1 = no selection */

    /* Cached line count of the flattened text. -1 means "not computed".
     * Invalidated alongside the flat buffer in invalidate_flat(). */
    int    line_count_cache;
    /* Forward-only line-lookup cache used by editor_get_line(). Stores
     * the byte offset into flat where line line_cache_num starts.
     * editor_ui_refresh() iterates lines in increasing order, so this
     * lets each refresh walk the flat buffer at most once instead of
     * O(N^2). Invalidated alongside the flat buffer in invalidate_flat(). */
    int    line_cache_num;
    size_t line_cache_offset;
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
#define s_flat              (s_active->flat)
#define s_gap_start         (s_active->gap_start)
#define s_gap_end           (s_active->gap_end)
#define s_buf_size          (s_active->buf_size)
#define s_path              (s_active->path)
#define s_modified          (s_active->modified)
#define s_flat_dirty        (s_active->flat_dirty)
#define s_mode              (s_active->mode)
#define s_scroll_line       (s_active->scroll_line)
#define s_sel_anchor        (s_active->sel_anchor)
#define s_line_count_cache  (s_active->line_count_cache)
#define s_line_cache_num    (s_active->line_cache_num)
#define s_line_cache_offset (s_active->line_cache_offset)

/* Shared system clipboard (not per-document). */
static char  *s_clipboard  = NULL;
static size_t s_clip_len   = 0;

/* Content length (excluding gap) */
static inline size_t content_len(void) { return s_buf_size - (s_gap_end - s_gap_start); }

/* Character at logical position p (0..content_len-1) */
static inline char char_at(size_t p)
{
    return (p < s_gap_start) ? s_buf[p] : s_buf[p + (s_gap_end - s_gap_start)];
}

static void doc_invalidate_flat(editor_doc_t *d)
{
    d->flat_dirty        = true;
    d->line_count_cache  = -1;
    d->line_cache_num    = -1;
    d->line_cache_offset = 0;
}

static void invalidate_flat(void)
{
    doc_invalidate_flat(s_active);
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
    d->mode        = EDITOR_MODE_NORMAL;
    d->scroll_line = 0;
    d->sel_anchor  = -1;
    doc_invalidate_flat(d);
}

/* Compute the per-document gap-buffer size from the PSRAM that is free
 * right now. Mirrors the original single-buffer sizing so a single open
 * document keeps exactly the same maximum size as before. */
static size_t compute_doc_buf_size(void)
{
    /* ---- Dynamic sizing from free PSRAM ----
     *
     * Use roughly half of the PSRAM that is still free at this point in
     * boot, split evenly between the gap buffer and the flat text cache.
     * The other half is reserved for the rest of the system that
     * initializes after the editor: BLE/Bluedroid
     * (CONFIG_BT_ALLOCATION_FROM_SPIRAM_FIRST), the WiFi/LWIP dynamic
     * pools (CONFIG_SPIRAM_TRY_ALLOCATE_WIFI_LWIP), the Git client's
     * task stack / object buffers / diff3 matrix, the LVGL widget heap growth, and
     * ad-hoc heap_caps_malloc(SPIRAM) allocations during editing
     * (clipboard, file I/O staging, Markdown re-rendering, etc.).
     *
     * Clamped to a sensible min/max so very small or very large PSRAM
     * configurations still get a usable editor:
     *   - Min 64 KB per buffer keeps editor_open_file() useful.
     *   - Max git_sync_max_file_size() per buffer: a sync cannot process
     *     a file larger than what fits (with its object buffers and the
     *     diff3 matrix) into the currently-free PSRAM. Git is the tighter
     *     of the two constraints, so it determines the effective maximum
     *     file size for the whole application. */
    const size_t MIN_BUF = 64  * 1024;
    const size_t MAX_BUF = git_sync_max_file_size();
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
    size_t per_buf = usable / 2;
    per_buf &= ~(ALIGN - 1);
    if (per_buf < MIN_BUF) per_buf = MIN_BUF;
    if (per_buf > MAX_BUF) per_buf = MAX_BUF;
    return per_buf;
}

/* Lazily allocate a slot's gap buffer and flat cache. The first slot is
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
    d->buf  = (char *)heap_caps_malloc(sz, MALLOC_CAP_SPIRAM);
    d->flat = (char *)heap_caps_malloc(sz + 1, MALLOC_CAP_SPIRAM);
    if (!d->buf || !d->flat) {
        free(d->buf);  d->buf  = NULL;
        free(d->flat); d->flat = NULL;
        return false;
    }
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
 * Each opened .md file gets a tiny key=value text sidecar saved next
 * to it that records the cursor position and visible scroll line so
 * the editor can resume exactly where the user left off. The sidecar
 * file name is derived by prefixing the basename with a dot and
 * appending ".meta" -- e.g. /sdcard/notes.md -> /sdcard/.notes.md.meta.
 *
 * The leading dot keeps the sidecar hidden from sd_card_list_dir()
 * (which already filters dotfiles), and the .meta extension keeps it
 * out of the git_sync commits (which only match *.md). The format
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

/* Transcode a UTF-16 buffer (LE if little_endian, BE otherwise) into
 * UTF-8 directly into s_buf. Surrogate pairs are decoded; unpaired
 * surrogates are emitted as U+FFFD. Returns the number of UTF-8 bytes
 * written, or (size_t)-1 if the output would not fit. */
static size_t transcode_utf16_to_utf8(const uint8_t *src, size_t src_len,
                                      bool little_endian,
                                      char *dst, size_t dst_cap)
{
    size_t i = 0;
    size_t out = 0;
    /* Round odd trailing byte off — we cannot decode half a code unit. */
    if (src_len & 1) src_len--;

    while (i + 1 < src_len) {
        uint16_t u = little_endian
            ? (uint16_t)(src[i] | (src[i + 1] << 8))
            : (uint16_t)((src[i] << 8) | src[i + 1]);
        i += 2;

        uint32_t cp;
        if (u >= 0xD800 && u <= 0xDBFF && i + 1 < src_len) {
            /* High surrogate — combine with the following low surrogate. */
            uint16_t lo = little_endian
                ? (uint16_t)(src[i] | (src[i + 1] << 8))
                : (uint16_t)((src[i] << 8) | src[i + 1]);
            if (lo >= 0xDC00 && lo <= 0xDFFF) {
                cp = 0x10000u + ((uint32_t)(u - 0xD800) << 10)
                              + (uint32_t)(lo - 0xDC00);
                i += 2;
            } else {
                cp = 0xFFFD;        /* lone high surrogate */
            }
        } else if (u >= 0xDC00 && u <= 0xDFFF) {
            cp = 0xFFFD;            /* lone low surrogate */
        } else {
            cp = u;
        }

        /* Drop UTF-16 line-separator and the rare CR (U+000D) that
         * Windows files use ahead of LF — the editor stores LF-only
         * lines. We leave the LF itself in place. */
        if (cp == 0x000D) continue;

        if (!append_utf8_cp(dst, dst_cap, &out, cp)) {
            return (size_t)-1;
        }
    }
    return out;
}

extern "C" esp_err_t editor_open_file(const char *path)
{
    /* Cheap size pre-check: avoid loading hundreds of KB into a
     * temporary heap allocation just to discover that the file does
     * not fit in the editor buffer. UTF-16 sources expand by at most
     * 1.5x when transcoded to UTF-8 (BMP chars: 2 -> up to 3 bytes;
     * surrogate pairs: 4 -> 4 bytes), so we still want a generous
     * upper bound here. */
    long fsize = sd_card_file_size(path);
    if (fsize >= 0 && (size_t)fsize > (s_buf_size - 64) * 2 / 3) {
        ESP_LOGW(TAG, "Refusing to open %s: %ld bytes > editor buffer (%u KB)",
                 path, fsize, (unsigned)(s_buf_size / 1024));
        return ESP_ERR_NO_MEM;
    }

    char *data = NULL;
    size_t len = 0;
    esp_err_t ret = sd_card_read_file(path, &data, &len);
    if (ret != ESP_OK) return ret;

    /* Detect a Unicode BOM and transcode if needed. The editor stores
     * text as UTF-8 internally; files saved as UTF-16 (common on
     * Windows -- Notepad still writes UTF-16 LE for "Unicode") would
     * otherwise be interpreted as random Latin-1 / continuation bytes
     * and render as garbage. */
    const uint8_t *u = (const uint8_t *)data;
    size_t written;
    if (len >= 2 && u[0] == 0xFF && u[1] == 0xFE) {
        /* UTF-16 LE BOM */
        written = transcode_utf16_to_utf8(u + 2, len - 2, true,
                                          s_buf, s_buf_size - 64);
        if (written == (size_t)-1) { free(data); return ESP_ERR_NO_MEM; }
    } else if (len >= 2 && u[0] == 0xFE && u[1] == 0xFF) {
        /* UTF-16 BE BOM */
        written = transcode_utf16_to_utf8(u + 2, len - 2, false,
                                          s_buf, s_buf_size - 64);
        if (written == (size_t)-1) { free(data); return ESP_ERR_NO_MEM; }
    } else {
        /* Plain UTF-8 (with or without an optional UTF-8 BOM). Strip a
         * leading EF BB BF so it does not show up as a stray glyph at
         * the top of the buffer. */
        size_t off = 0;
        if (len >= 3 && u[0] == 0xEF && u[1] == 0xBB && u[2] == 0xBF) off = 3;
        if (len - off > s_buf_size - 64) { free(data); return ESP_ERR_NO_MEM; }
        memcpy(s_buf, data + off, len - off);
        written = len - off;
    }
    s_gap_start = written;
    s_gap_end   = s_buf_size;
    free(data);

    strncpy(s_path, path, sizeof(s_path) - 1);
    s_path[sizeof(s_path) - 1] = '\0';
    s_modified = false;
    s_scroll_line = 0;
    s_sel_anchor = -1;
    invalidate_flat();
    s_mode = EDITOR_MODE_EDITING;

    /* Restore cursor position and scroll line from the metadata
     * sidecar (no-op if the file was never opened before). */
    editor_load_meta();

    ESP_LOGI(TAG, "Opened: %s (%zu bytes on disk, %zu stored)",
             path, len, s_gap_start);
    return ESP_OK;
}

extern "C" esp_err_t editor_save_file(void)
{
    if (s_path[0] == '\0') return ESP_ERR_INVALID_STATE;
    size_t len;
    const char *text = editor_get_text(&len);
    esp_err_t ret = sd_card_write_file(s_path, text, len);
    if (ret == ESP_OK) { s_modified = false; ESP_LOGI(TAG, "Saved: %s", s_path); }
    return ret;
}

extern "C" esp_err_t editor_save_file_as(const char *path)
{
    strncpy(s_path, path, sizeof(s_path) - 1);
    s_path[sizeof(s_path) - 1] = '\0';
    return editor_save_file();
}

extern "C" void editor_new_file(void)
{
    s_gap_start = 0;
    s_gap_end   = s_buf_size;
    s_path[0]   = '\0';
    s_modified  = false;
    s_scroll_line = 0;
    s_sel_anchor = -1;
    invalidate_flat();
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
    invalidate_flat();
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
     * gap buffer and flat cache are kept allocated for reuse by a future
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

extern "C" const char *editor_get_text(size_t *out_len)
{
    if (s_flat_dirty) {
        memcpy(s_flat, s_buf, s_gap_start);
        memcpy(s_flat + s_gap_start, s_buf + s_gap_end, s_buf_size - s_gap_end);
        size_t len = content_len();
        s_flat[len] = '\0';
        s_flat_dirty = false;
    }
    if (out_len) *out_len = content_len();
    return s_flat;
}

extern "C" size_t editor_get_cursor(void) { return s_gap_start; }

extern "C" void editor_get_cursor_pos(int *line, int *col)
{
    int l = 0, c = 0;
    for (size_t i = 0; i < s_gap_start; i++) {
        if (s_buf[i] == '\n') { l++; c = 0; }
        else {
            /* Count only the first byte of each UTF-8 character.
             * Continuation bytes have the form 10xxxxxx (0x80..0xBF). */
            if ((s_buf[i] & 0xC0) != 0x80) c++;
        }
    }
    if (line) *line = l;
    if (col)  *col  = c;
}

extern "C" int editor_get_line_count(void)
{
    if (s_line_count_cache >= 0) return s_line_count_cache;
    /* Use the flat buffer (single contiguous scan, no gap branch per
     * byte) instead of char_at() so this is fast even for large
     * documents. editor_get_text() also rebuilds the flat buffer if
     * needed, keeping this and editor_get_line() in sync. */
    size_t total;
    const char *text = editor_get_text(&total);
    int count = 1;
    for (size_t i = 0; i < total; i++) {
        if (text[i] == '\n') count++;
    }
    s_line_count_cache = count;
    return count;
}

extern "C" const char *editor_get_line(int line_num, size_t *out_len)
{
    /* Flatten first, then find the line in the flat buffer */
    size_t total;
    const char *text = editor_get_text(&total);

    /* Forward-only cache: if the requested line is at or after the
     * cached line, walk forward from the cached offset instead of
     * restarting from byte 0. The renderer iterates lines in order,
     * so this collapses the per-refresh cost from O(N^2) to O(N). */
    const char *p;
    int cur;
    if (line_num >= s_line_cache_num && s_line_cache_num >= 0 &&
        s_line_cache_offset <= total) {
        p   = text + s_line_cache_offset;
        cur = s_line_cache_num;
    } else {
        p   = text;
        cur = 0;
    }
    while (cur < line_num && p < text + total) {
        if (*p == '\n') cur++;
        p++;
    }
    if (cur < line_num) { if (out_len) *out_len = 0; return ""; }
    /* Update the cache to the line we just landed on. */
    s_line_cache_num    = line_num;
    s_line_cache_offset = (size_t)(p - text);
    const char *end = p;
    while (end < text + total && *end != '\n') end++;
    if (out_len) *out_len = end - p;
    return p;
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
    invalidate_flat();
}

static void gap_shift_right(void)
{
    s_buf[s_gap_start] = s_buf[s_gap_end];
    s_gap_start++;
    s_gap_end++;
    invalidate_flat();
}

/* Move the gap to a given logical position using memmove (faster than
 * byte-by-byte shifting for large distances). */
static void move_gap_to(size_t pos)
{
    if (pos == s_gap_start) return;
    if (pos < s_gap_start) {
        size_t n = s_gap_start - pos;
        memmove(s_buf + s_gap_end - n, s_buf + pos, n);
        s_gap_start = pos;
        s_gap_end -= n;
    } else {
        size_t n = pos - s_gap_start;
        memmove(s_buf + s_gap_start, s_buf + s_gap_end, n);
        s_gap_start += n;
        s_gap_end += n;
    }
    invalidate_flat();
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
    while (s_gap_start > 0) editor_move_left();
}

extern "C" void editor_move_doc_end(void)
{
    while (s_gap_end < s_buf_size) editor_move_right();
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
    s_buf[s_gap_start++] = c;
    s_modified = true;
    invalidate_flat();
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
    for (size_t i = 0; i < len; i++) {
        if (!editor_insert_char(text[i])) return false;
    }
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
    do {
        s_gap_start--;
    } while (s_gap_start > 0 && (s_buf[s_gap_start] & 0xC0) == 0x80);
    s_modified = true;
    invalidate_flat();
}

extern "C" void editor_delete_forward(void)
{
    if (s_gap_end >= s_buf_size) return;
    /* Remove the start byte and any following continuation bytes. */
    s_gap_end++;
    while (s_gap_end < s_buf_size && (s_buf[s_gap_end] & 0xC0) == 0x80)
        s_gap_end++;
    s_modified = true;
    invalidate_flat();
}

extern "C" void editor_delete_line(void)
{
    editor_move_home();
    size_t end = s_gap_end;
    while (end < s_buf_size && s_buf[end] != '\n') end++;
    if (end < s_buf_size) end++; /* include the newline */
    s_gap_end = end;
    s_modified = true;
    invalidate_flat();
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
    size_t total;
    const char *text = editor_get_text(&total);
    size_t nlen = strlen(needle);
    if (nlen > total) return -1;
    if (from_pos > total - nlen) return -1;
    /* Plain memmem-style scan over the flat buffer. */
    for (size_t i = from_pos; i + nlen <= total; i++) {
        if (memcmp(text + i, needle, nlen) == 0) return (int)i;
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
    s_gap_start = start;
    s_sel_anchor = -1;
    if (rlen > 0) {
        ensure_gap(rlen);
        memcpy(s_buf + s_gap_start, replacement, rlen);
        s_gap_start += rlen;
    }
    s_modified = true;
    invalidate_flat();
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
    s_gap_start = sel_s;
    s_sel_anchor = -1;
    s_modified = true;
    invalidate_flat();
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

    const char *text = editor_get_text(NULL);

    if (s_clipboard) { free(s_clipboard); s_clipboard = NULL; s_clip_len = 0; }
    s_clipboard = (char *)heap_caps_malloc(len + 1, MALLOC_CAP_SPIRAM);
    if (!s_clipboard) return false;
    memcpy(s_clipboard, text + sel_s, len);
    s_clipboard[len] = '\0';
    s_clip_len = len;
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
    if (!s_clipboard || s_clip_len == 0) return true;
    editor_delete_selection();
    return editor_insert_text(s_clipboard, s_clip_len);
}

extern "C" bool editor_has_clipboard(void)
{
    return s_clipboard != NULL && s_clip_len > 0;
}

extern "C" editor_mode_t editor_get_mode(void) { return s_mode; }
extern "C" void editor_set_mode(editor_mode_t mode) { s_mode = mode; }
