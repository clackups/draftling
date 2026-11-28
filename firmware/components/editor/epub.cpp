/* epub.cpp -- EPUB books for the reader. See private_include/epub.h.
 *
 * An EPUB is a ZIP archive: META-INF/container.xml names the package
 * (OPF) file, whose manifest lists the files and whose spine gives the
 * reading order; the table of contents is the EPUB 3 navigation
 * document (manifest item with properties="nav") or the EPUB 2 NCX.
 * Each chapter is an XHTML file, converted here to the Markdown the
 * editor renders (headings, emphasis, lists, quotes, rules, code).
 * CSS is ignored and images are shown as "[image]".
 *
 * Only what the book needs is kept in memory: the ZIP central
 * directory, the spine and the chapter titles, all in PSRAM. A chapter
 * is inflated whole (the ROM's miniz, raw deflate, the decompressor
 * state on the heap) into a temporary
 * PSRAM buffer and converted straight into the document buffer. */
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <cstring>
#include <string>
#include <vector>
#include <strings.h>
#include <esp_log.h>
#include <esp_heap_caps.h>
#include "miniz.h"

#include "epub.h"

static const char *TAG = "Epub";

/* ---- PSRAM-backed containers ---- */

static void *ps_malloc(size_t n)
{
    void *p = heap_caps_malloc(n ? n : 1, MALLOC_CAP_SPIRAM);
    if (!p) p = malloc(n ? n : 1);
    return p;
}

template <class T> struct ps_alloc {
    using value_type = T;
    ps_alloc() = default;
    template <class U> ps_alloc(const ps_alloc<U> &) {}
    T *allocate(size_t n)
    {
        T *p = (T *)ps_malloc(n * sizeof(T));
        if (!p) abort();   /* out of memory: no exceptions in this build */
        return p;
    }
    void deallocate(T *p, size_t) { heap_caps_free(p); }
    template <class U> bool operator==(const ps_alloc<U> &) const { return true; }
    template <class U> bool operator!=(const ps_alloc<U> &) const { return false; }
};
typedef std::basic_string<char, std::char_traits<char>, ps_alloc<char>> pstr;
template <class T> using pvec = std::vector<T, ps_alloc<T>>;

/* ---- ZIP ---- */

struct zip_entry {
    pstr     name;
    uint16_t method;     /* 0 stored, 8 deflate */
    uint16_t flags;      /* bit 0: encrypted */
    uint32_t csize, usize, lhoff;
};

struct epub_book {
    pstr            path;
    pvec<zip_entry> zip;
    pstr            title;
    pvec<pstr>      spine;      /* archive paths, reading order */
    pvec<pstr>      titles;     /* per spine item, "" = none */
};

static uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0] | p[1] << 8); }
static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static bool zip_read_dir(epub_book_t *b, const char **why)
{
    FILE *f = fopen(b->path.c_str(), "rb");
    if (!f) { *why = "Cannot open the file"; return false; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    /* End of central directory: 22 bytes plus a comment of up to 64 KB. */
    long tail = size < 22 + 65535 ? size : 22 + 65535;
    uint8_t *buf = (uint8_t *)ps_malloc((size_t)tail);
    bool ok = false;
    *why = "Not an EPUB (no ZIP directory)";
    if (buf && tail >= 22 && fseek(f, size - tail, SEEK_SET) == 0 &&
        fread(buf, 1, (size_t)tail, f) == (size_t)tail) {
        for (long i = tail - 22; i >= 0; i--) {
            if (rd32(buf + i) != 0x06054b50) continue;
            uint32_t cd_size = rd32(buf + i + 12), cd_off = rd32(buf + i + 16);
            uint16_t count = rd16(buf + i + 10);
            if (cd_off == 0xFFFFFFFFu || (long)cd_off + (long)cd_size > size) {
                *why = "Unsupported ZIP format";
                break;
            }
            uint8_t *cd = (uint8_t *)ps_malloc(cd_size);
            if (!cd || fseek(f, (long)cd_off, SEEK_SET) != 0 ||
                fread(cd, 1, cd_size, f) != cd_size) {
                heap_caps_free(cd);
                *why = "Cannot read the ZIP directory";
                break;
            }
            size_t p = 0;
            for (unsigned n = 0; n < count && p + 46 <= cd_size; n++) {
                if (rd32(cd + p) != 0x02014b50) break;
                zip_entry e;
                e.flags  = rd16(cd + p + 8);
                e.method = rd16(cd + p + 10);
                e.csize  = rd32(cd + p + 20);
                e.usize  = rd32(cd + p + 24);
                uint16_t nl = rd16(cd + p + 28), xl = rd16(cd + p + 30), cl = rd16(cd + p + 32);
                e.lhoff  = rd32(cd + p + 42);
                if (p + 46 + nl > cd_size) break;
                e.name.assign((const char *)cd + p + 46, nl);
                b->zip.push_back(e);
                p += 46u + nl + xl + cl;
            }
            heap_caps_free(cd);
            ok = !b->zip.empty();
            if (!ok) *why = "Empty or damaged ZIP";
            break;
        }
    }
    heap_caps_free(buf);
    fclose(f);
    return ok;
}

static const zip_entry *zip_find(const epub_book_t *b, const char *name)
{
    for (const zip_entry &e : b->zip)
        if (e.name == name) return &e;
    for (const zip_entry &e : b->zip)   /* some books get the case wrong */
        if (strcasecmp(e.name.c_str(), name) == 0) return &e;
    return NULL;
}

/* Inflate an entry into a new PSRAM buffer (NUL-terminated). */
static char *zip_read(const epub_book_t *b, const char *name, size_t *len, const char **why)
{
    const zip_entry *e = zip_find(b, name);
    if (!e) { *why = "A file of the book is missing"; return NULL; }
    if (e->flags & 1) { *why = "The book is encrypted (DRM)"; return NULL; }
    if (e->usize > EPUB_MAX_ENTRY) { *why = "A chapter is too large"; return NULL; }
    if (e->method != 0 && e->method != 8) { *why = "Unsupported compression"; return NULL; }
    FILE *f = fopen(b->path.c_str(), "rb");
    if (!f) { *why = "Cannot open the file"; return NULL; }
    uint8_t lh[30];
    char *out = NULL;
    *why = "Cannot read the book";
    if (fseek(f, (long)e->lhoff, SEEK_SET) == 0 && fread(lh, 1, 30, f) == 30 &&
        rd32(lh) == 0x04034b50 &&
        fseek(f, (long)e->lhoff + 30 + rd16(lh + 26) + rd16(lh + 28), SEEK_SET) == 0) {
        out = (char *)ps_malloc((size_t)e->usize + 1);
        if (!out) {
            *why = "Out of memory";
        } else if (e->method == 0) {
            if (fread(out, 1, e->usize, f) != e->usize) { heap_caps_free(out); out = NULL; }
        } else {
            /* The decompressor state is about 11 KB: on the heap, never
             * on the stack (tinfl_decompress_mem_to_mem() keeps it on the
             * stack, which overflowed the LVGL task that opens books). */
            uint8_t *in = (uint8_t *)ps_malloc(e->csize);
            tinfl_decompressor *d = (tinfl_decompressor *)ps_malloc(sizeof(tinfl_decompressor));
            size_t got = (size_t)-1;
            if (in && d && fread(in, 1, e->csize, f) == e->csize) {
                tinfl_init(d);
                size_t in_n = e->csize, out_n = e->usize;
                tinfl_status st = tinfl_decompress(d, in, &in_n, (mz_uint8 *)out, (mz_uint8 *)out,
                                                   &out_n, TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
                if (st == TINFL_STATUS_DONE) got = out_n;
            }
            heap_caps_free(d);
            heap_caps_free(in);
            if (got != e->usize) {
                heap_caps_free(out);
                out = NULL;
                *why = "The book is damaged or encrypted";
            }
        }
    }
    fclose(f);
    if (out) {
        out[e->usize] = '\0';
        *len = e->usize;
    }
    return out;
}

/* ---- XML scanning ---- */

struct xtag {
    const char *name;   size_t name_len;    /* local name, prefix dropped */
    const char *attrs;  size_t attrs_len;
    bool closing, self_closing;
};

/* At p == '<': parse a tag, comment, CDATA, PI or doctype. Returns the
 * position after it; *is_tag tells whether t was filled. For CDATA,
 * *cdata / *cdata_len receive the text. */
static const char *scan_tag(const char *p, const char *end, xtag *t, bool *is_tag,
                            const char **cdata = NULL, size_t *cdata_len = NULL)
{
    *is_tag = false;
    if (cdata) *cdata = NULL;
    if (end - p >= 4 && !strncmp(p, "<!--", 4)) {
        const char *q = (const char *)memmem(p + 4, (size_t)(end - p - 4), "-->", 3);
        return q ? q + 3 : end;
    }
    if (end - p >= 9 && !strncmp(p, "<![CDATA[", 9)) {
        const char *q = (const char *)memmem(p + 9, (size_t)(end - p - 9), "]]>", 3);
        if (cdata) { *cdata = p + 9; *cdata_len = (size_t)((q ? q : end) - (p + 9)); }
        return q ? q + 3 : end;
    }
    if (end - p >= 2 && (p[1] == '?' || p[1] == '!')) {
        const char *q = (const char *)memchr(p, '>', (size_t)(end - p));
        return q ? q + 1 : end;
    }
    const char *q = p + 1;
    t->closing = (q < end && *q == '/');
    if (t->closing) q++;
    const char *n0 = q;
    while (q < end && !strchr(" \t\r\n/>", *q)) q++;
    const char *colon = (const char *)memchr(n0, ':', (size_t)(q - n0));
    t->name = colon ? colon + 1 : n0;
    t->name_len = (size_t)(q - t->name);
    t->attrs = q;
    /* Find the closing '>' outside quoted attribute values. */
    char quote = 0;
    while (q < end) {
        if (quote) { if (*q == quote) quote = 0; }
        else if (*q == '"' || *q == '\'') quote = *q;
        else if (*q == '>') break;
        q++;
    }
    t->attrs_len = (size_t)(q - t->attrs);
    t->self_closing = t->attrs_len && t->attrs[t->attrs_len - 1] == '/';
    *is_tag = t->name_len > 0;
    return q < end ? q + 1 : end;
}

static bool tag_is(const xtag &t, const char *name)
{
    size_t n = strlen(name);
    return t.name_len == n && strncasecmp(t.name, name, n) == 0;
}

static size_t put_utf8(char *o, uint32_t cp)
{
    if (cp < 0x80) { o[0] = (char)cp; return 1; }
    if (cp < 0x800) { o[0] = (char)(0xC0 | cp >> 6); o[1] = (char)(0x80 | (cp & 0x3F)); return 2; }
    if (cp < 0x10000) {
        o[0] = (char)(0xE0 | cp >> 12); o[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        o[2] = (char)(0x80 | (cp & 0x3F)); return 3;
    }
    o[0] = (char)(0xF0 | cp >> 18); o[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
    o[2] = (char)(0x80 | ((cp >> 6) & 0x3F)); o[3] = (char)(0x80 | (cp & 0x3F)); return 4;
}

/* At p == '&': decode an entity into out (up to 4 bytes). Returns the
 * bytes consumed, 0 if it is not a known entity (the '&' is literal). */
static size_t decode_entity(const char *p, const char *end, char *out, size_t *out_len)
{
    static const struct { const char *name; uint32_t cp; } names[] = {
        { "amp", '&' }, { "lt", '<' }, { "gt", '>' }, { "quot", '"' }, { "apos", '\'' },
        { "nbsp", 0xA0 }, { "shy", 0xAD }, { "copy", 0xA9 }, { "reg", 0xAE },
        { "trade", 0x2122 }, { "mdash", 0x2014 }, { "ndash", 0x2013 },
        { "hellip", 0x2026 }, { "lsquo", 0x2018 }, { "rsquo", 0x2019 },
        { "ldquo", 0x201C }, { "rdquo", 0x201D }, { "sbquo", 0x201A },
        { "bdquo", 0x201E }, { "laquo", 0xAB }, { "raquo", 0xBB },
        { "middot", 0xB7 }, { "bull", 0x2022 }, { "deg", 0xB0 }, { "times", 0xD7 },
        { "eacute", 0xE9 }, { "egrave", 0xE8 }, { "agrave", 0xE0 }, { "ccedil", 0xE7 },
        { "euro", 0x20AC }, { "pound", 0xA3 }, { "sect", 0xA7 }, { "para", 0xB6 },
        { "thinsp", 0x2009 }, { "ensp", 0x2002 }, { "emsp", 0x2003 },
        { "zwnj", 0x200C }, { "zwj", 0x200D },
    };
    const char *semi = (const char *)memchr(p, ';', (size_t)(end - p) < 12 ? (size_t)(end - p) : 12);
    if (!semi) return 0;
    uint32_t cp = 0;
    if (p[1] == '#') {
        const char *q = p + 2;
        int base = 10;
        if (q < semi && (*q == 'x' || *q == 'X')) { base = 16; q++; }
        if (q == semi) return 0;
        for (; q < semi; q++) {
            int d = (*q >= '0' && *q <= '9') ? *q - '0'
                  : (base == 16 && *q >= 'a' && *q <= 'f') ? *q - 'a' + 10
                  : (base == 16 && *q >= 'A' && *q <= 'F') ? *q - 'A' + 10 : -1;
            if (d < 0) return 0;
            cp = cp * (uint32_t)base + (uint32_t)d;
            if (cp > 0x10FFFF) return 0;
        }
    } else {
        size_t n = (size_t)(semi - p - 1);
        bool found = false;
        for (const auto &e : names) {
            if (strlen(e.name) == n && !strncmp(p + 1, e.name, n)) { cp = e.cp; found = true; break; }
        }
        if (!found) return 0;
    }
    if (cp == 0) return 0;
    *out_len = put_utf8(out, cp);
    return (size_t)(semi - p + 1);
}

/* Attribute value (entities decoded), "" when missing. */
static pstr attr(const xtag &t, const char *name)
{
    size_t n = strlen(name);
    const char *p = t.attrs, *end = t.attrs + t.attrs_len;
    while (p < end) {
        while (p < end && strchr(" \t\r\n/", *p)) p++;
        const char *a0 = p;
        while (p < end && !strchr(" \t\r\n=/>", *p)) p++;
        size_t al = (size_t)(p - a0);
        while (p < end && strchr(" \t\r\n", *p)) p++;
        if (p >= end || *p != '=') continue;
        p++;
        while (p < end && strchr(" \t\r\n", *p)) p++;
        if (p >= end) break;
        char q = *p;
        const char *v0, *v1;
        if (q == '"' || q == '\'') {
            v0 = ++p;
            while (p < end && *p != q) p++;
            v1 = p;
            if (p < end) p++;
        } else {
            v0 = p;
            while (p < end && !strchr(" \t\r\n>", *p)) p++;
            v1 = p;
        }
        if (al == n && !strncasecmp(a0, name, n)) {
            pstr v;
            for (const char *s = v0; s < v1; s++) {
                char u[4];
                size_t ul = 0, used = (*s == '&') ? decode_entity(s, v1, u, &ul) : 0;
                if (used) { v.append(u, ul); s += used - 1; }
                else v += *s;
            }
            return v;
        }
    }
    return pstr();
}

/* Append the text up to the next tag to out, entities decoded and
 * whitespace collapsed to single spaces (also across calls, so text
 * split by inline tags keeps its spaces). See trimmed(). */
static void append_text(pstr &out, const char *&p, const char *end)
{
    while (p < end && *p != '<') {
        if (strchr(" \t\r\n", *p)) {
            if (!out.empty() && out.back() != ' ') out += ' ';
            p++;
            continue;
        }
        char u[4];
        size_t ul = 0, used = (*p == '&') ? decode_entity(p, end, u, &ul) : 0;
        if (used) { out.append(u, ul); p += used; }
        else out += *p++;
    }
}

static pstr trimmed(const pstr &s)
{
    size_t a = s.find_first_not_of(' '), z = s.find_last_not_of(' ');
    return a == pstr::npos ? pstr() : s.substr(a, z - a + 1);
}

/* ---- Paths inside the archive ---- */

static pstr dir_of(const pstr &path)
{
    size_t s = path.rfind('/');
    return s == pstr::npos ? pstr() : path.substr(0, s + 1);
}

/* href (URL-encoded, maybe with a #fragment) relative to base_dir. */
static pstr resolve(const pstr &base_dir, const pstr &href)
{
    pstr h;
    for (size_t i = 0; i < href.size(); i++) {
        if (href[i] == '#' || href[i] == '?') break;
        if (href[i] == '%' && i + 2 < href.size()) {
            char hex[3] = { href[i + 1], href[i + 2], 0 };
            char *e = NULL;
            long v = strtol(hex, &e, 16);
            if (e == hex + 2) { h += (char)v; i += 2; continue; }
        }
        h += href[i];
    }
    pstr full = (!h.empty() && h[0] == '/') ? h.substr(1) : base_dir + h;
    pvec<pstr> parts;
    size_t s = 0;
    while (s <= full.size()) {
        size_t e = full.find('/', s);
        if (e == pstr::npos) e = full.size();
        pstr seg = full.substr(s, e - s);
        if (seg == "..") { if (!parts.empty()) parts.pop_back(); }
        else if (!seg.empty() && seg != ".") parts.push_back(seg);
        s = e + 1;
    }
    pstr out;
    for (size_t i = 0; i < parts.size(); i++) {
        if (i) out += '/';
        out += parts[i];
    }
    return out;
}

/* ---- Package and table of contents ---- */

static void set_title(epub_book_t *b, const pstr &file, const pstr &label)
{
    if (label.empty()) return;
    for (size_t i = 0; i < b->spine.size(); i++) {
        if (b->spine[i] == file) {
            if (b->titles[i].empty()) b->titles[i] = label;
            return;
        }
    }
}

/* EPUB 3 navigation document: the <a> links inside <nav epub:type="toc">. */
static bool parse_nav(epub_book_t *b, const char *x, size_t len, const pstr &base)
{
    const char *p = x, *end = x + len;
    int nav_depth = 0;          /* inside the toc nav */
    bool found = false;
    pstr href, label;
    bool in_a = false;
    while (p < end) {
        if (*p != '<') {
            if (in_a) append_text(label, p, end);
            else while (p < end && *p != '<') p++;
            continue;
        }
        xtag t; bool is;
        p = scan_tag(p, end, &t, &is);
        if (!is) continue;
        if (tag_is(t, "nav")) {
            if (t.closing) { if (nav_depth) nav_depth--; }
            else if (nav_depth) nav_depth++;
            else {
                pstr type = attr(t, "epub:type");
                if (type.empty()) type = attr(t, "type");
                if (type.find("toc") != pstr::npos) { nav_depth = 1; found = true; }
            }
            continue;
        }
        if (!nav_depth) continue;
        if (tag_is(t, "a")) {
            if (!t.closing) {
                href = attr(t, "href");
                label.clear();
                in_a = !t.self_closing;
            } else if (in_a) {
                set_title(b, resolve(base, href), trimmed(label));
                in_a = false;
            }
        }
    }
    return found;
}

/* EPUB 2 NCX: navPoint -> navLabel/text + content/@src. */
static void parse_ncx(epub_book_t *b, const char *x, size_t len, const pstr &base)
{
    const char *p = x, *end = x + len;
    pstr label;
    bool in_text = false;
    while (p < end) {
        if (*p != '<') {
            if (in_text) append_text(label, p, end);
            else while (p < end && *p != '<') p++;
            continue;
        }
        xtag t; bool is;
        p = scan_tag(p, end, &t, &is);
        if (!is) continue;
        if (tag_is(t, "text")) {
            in_text = !t.closing && !t.self_closing;
            if (in_text) label.clear();
        } else if (tag_is(t, "content") && !t.closing) {
            set_title(b, resolve(base, attr(t, "src")), trimmed(label));
            label.clear();
        }
    }
}

static bool parse_package(epub_book_t *b, const char **why)
{
    size_t len = 0;
    char *x = zip_read(b, "META-INF/container.xml", &len, why);
    if (!x) { *why = "Not an EPUB (no container.xml)"; return false; }
    pstr opf;
    {
        const char *p = x, *end = x + len;
        while (p < end && opf.empty()) {
            if (*p != '<') { p++; continue; }
            xtag t; bool is;
            p = scan_tag(p, end, &t, &is);
            if (is && !t.closing && tag_is(t, "rootfile")) opf = attr(t, "full-path");
        }
    }
    heap_caps_free(x);
    if (opf.empty()) { *why = "Not an EPUB (no package file)"; return false; }
    x = zip_read(b, opf.c_str(), &len, why);
    if (!x) return false;

    struct item { pstr id, href, type, props; };
    pvec<item> manifest;
    pvec<pstr> spine_ids;
    pstr toc_id;
    const pstr base = dir_of(opf);
    const char *p = x, *end = x + len;
    bool in_title = false;
    while (p < end) {
        if (*p != '<') {
            if (in_title) append_text(b->title, p, end);
            else while (p < end && *p != '<') p++;
            continue;
        }
        xtag t; bool is;
        p = scan_tag(p, end, &t, &is);
        if (!is) continue;
        if (tag_is(t, "title")) {
            /* The first dc:title only. */
            if (!t.closing && !t.self_closing && b->title.empty()) in_title = true;
            else if (in_title) { in_title = false; b->title = trimmed(b->title); }
        } else if (t.closing) continue;
        else if (tag_is(t, "item")) {
            item it;
            it.id = attr(t, "id");
            it.href = resolve(base, attr(t, "href"));
            it.type = attr(t, "media-type");
            it.props = attr(t, "properties");
            manifest.push_back(it);
        } else if (tag_is(t, "spine")) {
            toc_id = attr(t, "toc");
        } else if (tag_is(t, "itemref")) {
            spine_ids.push_back(attr(t, "idref"));
        }
    }
    heap_caps_free(x);

    for (const pstr &id : spine_ids) {
        for (const item &it : manifest) {
            if (it.id == id) {
                b->spine.push_back(it.href);
                b->titles.push_back(pstr());
                break;
            }
        }
    }
    if (b->spine.empty()) { *why = "The book has no chapters"; return false; }

    /* Table of contents: the nav document, else the NCX. A missing or
     * broken one only costs the chapter titles. */
    bool have_nav = false;
    for (const item &it : manifest) {
        if (it.props.find("nav") == pstr::npos) continue;
        const char *w = NULL;
        char *nav = zip_read(b, it.href.c_str(), &len, &w);
        if (nav) {
            have_nav = parse_nav(b, nav, len, dir_of(it.href));
            heap_caps_free(nav);
        }
        break;
    }
    if (!have_nav) {
        for (const item &it : manifest) {
            if (it.id != toc_id && it.type != "application/x-dtbncx+xml") continue;
            const char *w = NULL;
            char *ncx = zip_read(b, it.href.c_str(), &len, &w);
            if (ncx) {
                parse_ncx(b, ncx, len, dir_of(it.href));
                heap_caps_free(ncx);
            }
            break;
        }
    }
    return true;
}

extern "C" epub_book_t *epub_open(const char *path, const char **why)
{
    const char *dummy;
    if (!why) why = &dummy;
    void *mem = ps_malloc(sizeof(epub_book_t));
    if (!mem) { *why = "Out of memory"; return NULL; }
    epub_book_t *b = new (mem) epub_book_t();
    b->path = path;
    if (!zip_read_dir(b, why) || !parse_package(b, why)) {
        ESP_LOGW(TAG, "%s: %s", path, *why);
        epub_close(b);
        return NULL;
    }
    ESP_LOGI(TAG, "%s: %u chapters", path, (unsigned)b->spine.size());
    return b;
}

extern "C" void epub_close(epub_book_t *b)
{
    if (!b) return;
    b->~epub_book();
    heap_caps_free(b);
}

extern "C" void epub_set_path(epub_book_t *b, const char *path)
{
    if (b && path) b->path = path;
}

extern "C" const char *epub_title(const epub_book_t *b)
{
    return (b && !b->title.empty()) ? b->title.c_str() : NULL;
}

extern "C" int epub_chapter_count(const epub_book_t *b)
{
    return b ? (int)b->spine.size() : 0;
}

extern "C" const char *epub_chapter_title(const epub_book_t *b, int i)
{
    if (!b || i < 0 || i >= (int)b->titles.size() || b->titles[i].empty()) return NULL;
    return b->titles[i].c_str();
}

/* ---- XHTML to Markdown ---- */

namespace {

/* Room kept at the end of the output for the "does not fit" note. */
const size_t RESERVE = 96;
const char TRUNC_NOTE[] = "\n\n[The rest of this chapter does not fit in memory.]\n";

struct style_ent { const char *mark; bool emitted; };

struct md_writer {
    char  *dst;
    size_t cap, len = 0;
    bool   full = false;

    int  brk = 0;              /* line breaks owed before the next text: 1 or 2 */
    bool space = false;        /* a collapsed space owed before the next text */
    const char *line_mark = NULL;  /* "## ", "- "...: put before the next text */
    char list_mark[24] = "";
    int  quote = 0;            /* blockquote depth */
    int  list_depth = 0;
    int  ol_num[8] = {};       /* item counters of ordered lists, 0 = bulleted */
    bool in_pre = false;
    int  code = 0;             /* inside <code> etc. (no escaping, no markers) */
    pvec<style_ent> styles;    /* open emphasis, outermost first */
    int  cell = 0;             /* table cell index in the row */

    void raw(const char *s, size_t n)
    {
        if (full) return;
        if (len + n > cap - RESERVE) { full = true; return; }
        memcpy(dst + len, s, n);
        len += n;
    }
    void raw(const char *s) { raw(s, strlen(s)); }
    char last() const { return len ? dst[len - 1] : '\n'; }

    void want_break(int n) { if (len && n > brk) brk = n; space = false; }

    /* Before text: line breaks (closing emphasis first, so no span
     * crosses a line), the line's prefix, emphasis openers, space. */
    void begin_text()
    {
        if (brk && len) {
            for (size_t i = styles.size(); i-- > 0;) {
                if (styles[i].mark && styles[i].emitted) { raw(styles[i].mark); styles[i].emitted = false; }
            }
            while (len && dst[len - 1] == ' ') len--;
            raw(brk >= 2 ? "\n\n" : "\n");
            space = false;
        }
        bool line_start = brk || !len;
        brk = 0;
        if (line_start) {
            for (int i = 0; i < quote; i++) raw("> ");
            if (line_mark) { raw(line_mark); line_mark = NULL; }
            else if (list_mark[0]) { raw(list_mark); list_mark[0] = '\0'; }
            else if (list_depth) for (int i = 0; i < list_depth; i++) raw("  ");
            space = false;
        }
        if (space) { raw(" "); space = false; }
        if (!code) {
            for (style_ent &s : styles)
                if (s.mark && !s.emitted) { raw(s.mark); s.emitted = true; }
        }
    }

    /* One character of text (UTF-8 sequence in s[0, n)); next is the
     * following input byte, for the escaping decisions. */
    void text_char(const char *s, size_t n, char next)
    {
        unsigned char c = (unsigned char)s[0];
        if (!in_pre && (c == ' ' || c == '\t' || c == '\n' || c == '\r')) {
            if (len && !brk && last() != '\n') space = true;
            return;
        }
        /* Invisible characters that only get in the way. */
        if (n == 2 && c == 0xC2 && (unsigned char)s[1] == 0xAD) return;    /* soft hyphen */
        if (n == 3 && c == 0xE2 && (unsigned char)s[1] == 0x80 &&
            (unsigned char)s[2] == 0x8B) return;                             /* zero-width space */
        if (n == 3 && c == 0xEF && (unsigned char)s[1] == 0xBB &&
            (unsigned char)s[2] == 0xBF) return;                             /* BOM */
        if (in_pre) {
            if (c == '\r') return;
            raw(s, n);
            return;
        }
        bool first = brk || !len || last() == '\n';
        begin_text();
        if (!code && n == 1) {
            char prev = last();
            bool esc = false;
            switch (c) {
            case '\\': esc = strchr("\\`*_~#>-+.!()[]", next) != NULL; break;
            case '`':  esc = true; break;
            case '*':  esc = !(prev == ' ' && (next == ' ' || next == '\n')); break;
            case '_':  esc = !(isalnum((unsigned char)prev) && isalnum((unsigned char)next)); break;
            case '~':  esc = next == '~' || prev == '~'; break;
            case '#': case '>': esc = first; break;
            case '-': case '+': esc = first && (next == ' ' || next == '-'); break;
            case '.': case ')': esc = num_start && next == ' '; break;
            default: break;
            }
            if (esc) raw("\\");
            raw(s, n);
            /* Digits opening a line, then ". " or ") ", would make it a
             * list item: num_start follows such a run of digits. */
            num_start = (c >= '0' && c <= '9') && (first || num_start);
            return;
        }
        num_start = false;
        raw(s, n);
    }
    bool num_start = false;

    void push_style(const char *mark)
    {
        for (const style_ent &s : styles)
            if (s.mark && !strcmp(s.mark, mark)) { styles.push_back({ NULL, false }); return; }
        styles.push_back({ mark, false });
    }
    void pop_style()
    {
        if (styles.empty()) return;
        style_ent s = styles.back();
        styles.pop_back();
        if (s.mark && s.emitted && !code) raw(s.mark);
    }
};

bool is_block(const xtag &t)
{
    static const char *const blocks[] = {
        "p", "div", "section", "article", "header", "footer", "aside", "nav",
        "figure", "figcaption", "main", "address", "center", "dl", "table",
        "caption", "body", "hgroup",
    };
    for (const char *b : blocks) if (tag_is(t, b)) return true;
    return false;
}

const char *style_of(const xtag &t)
{
    if (tag_is(t, "em") || tag_is(t, "i") || tag_is(t, "cite") || tag_is(t, "dfn") ||
        tag_is(t, "var")) return "*";
    if (tag_is(t, "strong") || tag_is(t, "b")) return "**";
    if (tag_is(t, "s") || tag_is(t, "strike") || tag_is(t, "del")) return "~~";
    return NULL;
}

bool is_code(const xtag &t)
{
    return tag_is(t, "code") || tag_is(t, "kbd") || tag_is(t, "samp") || tag_is(t, "tt");
}

/* Elements whose content is not shown. */
bool is_skipped(const xtag &t)
{
    return tag_is(t, "head") || tag_is(t, "script") || tag_is(t, "style") ||
           tag_is(t, "svg") || tag_is(t, "math") || tag_is(t, "rt") || tag_is(t, "rp") ||
           tag_is(t, "noscript") || tag_is(t, "template");
}

}  /* namespace */

extern "C" size_t epub_xhtml_to_markdown(const char *x, size_t xlen, char *dst, size_t cap)
{
    if (cap <= RESERVE) return 0;
    md_writer w;
    w.dst = dst;
    w.cap = cap;
    const char *p = x, *end = x + xlen;
    int skip = 0;                       /* depth inside skipped elements */
    /* Element stack for the inline styles (one entry per open style or
     * code element), so a close pops what its open pushed. */
    while (p < end && !w.full) {
        if (*p == '<') {
            xtag t; bool is;
            const char *cd = NULL; size_t cdl = 0;
            p = scan_tag(p, end, &t, &is, &cd, &cdl);
            if (cd && !skip) {
                for (size_t i = 0; i < cdl; i++) w.text_char(cd + i, 1, i + 1 < cdl ? cd[i + 1] : ' ');
                continue;
            }
            if (!is) continue;
            if (is_skipped(t)) {
                if (t.self_closing) continue;
                if (t.closing) { if (skip) skip--; }
                else {
                    if (!skip && tag_is(t, "svg")) {
                        w.want_break(2);
                        w.begin_text();
                        w.raw("[image]");
                        w.want_break(2);
                    }
                    skip++;
                }
                continue;
            }
            if (skip) continue;

            if (tag_is(t, "br")) {
                if (w.in_pre) w.raw("\n");
                else if (w.len) { w.want_break(1); }
                continue;
            }
            if (tag_is(t, "hr")) {
                w.want_break(2);
                w.begin_text();
                w.raw("---");
                w.want_break(2);
                continue;
            }
            if (tag_is(t, "img") || tag_is(t, "image")) {
                pstr alt = attr(t, "alt");
                w.begin_text();
                w.raw(alt.empty() ? "[image]" : "[image: ");
                if (!alt.empty()) {
                    for (size_t i = 0; i < alt.size(); i++)
                        w.text_char(&alt[i], 1, i + 1 < alt.size() ? alt[i + 1] : ']');
                    w.raw("]");
                }
                continue;
            }
            if (t.name_len == 2 && (t.name[0] == 'h' || t.name[0] == 'H') &&
                t.name[1] >= '1' && t.name[1] <= '6') {
                w.want_break(2);
                if (!t.closing) {
                    static const char *const marks[] = { "# ", "## ", "### ", "#### " };
                    int lvl = t.name[1] - '1';
                    w.line_mark = marks[lvl > 3 ? 3 : lvl];
                }
                continue;
            }
            if (tag_is(t, "pre")) {
                if (!t.closing) {
                    w.want_break(2);
                    w.begin_text();
                    w.raw("```\n");
                    w.in_pre = true;
                } else if (w.in_pre) {
                    if (w.last() != '\n') w.raw("\n");
                    w.raw("```");
                    w.in_pre = false;
                    w.want_break(2);
                }
                continue;
            }
            if (tag_is(t, "blockquote")) {
                w.want_break(2);
                if (t.closing) { if (w.quote) w.quote--; }
                else if (!t.self_closing) w.quote++;
                continue;
            }
            if (tag_is(t, "ul") || tag_is(t, "ol")) {
                w.want_break(w.list_depth ? 1 : 2);
                if (t.closing) { if (w.list_depth) w.list_depth--; }
                else if (!t.self_closing && w.list_depth < 8) {
                    w.ol_num[w.list_depth] = tag_is(t, "ol") ? 1 : 0;
                    w.list_depth++;
                }
                if (t.closing && !w.list_depth) w.want_break(2);
                continue;
            }
            if (tag_is(t, "li")) {
                w.want_break(1);
                if (!t.closing && w.list_depth) {
                    int d = w.list_depth - 1;
                    char ind[17];
                    int n = d * 2 < 16 ? d * 2 : 16;
                    memset(ind, ' ', (size_t)n);
                    ind[n] = '\0';
                    if (w.ol_num[d]) snprintf(w.list_mark, sizeof(w.list_mark), "%s%d. ", ind, w.ol_num[d]++);
                    else snprintf(w.list_mark, sizeof(w.list_mark), "%s- ", ind);
                }
                continue;
            }
            if (tag_is(t, "tr")) { w.want_break(1); w.cell = 0; continue; }
            if (tag_is(t, "td") || tag_is(t, "th")) {
                if (!t.closing) {
                    if (w.cell++) { w.begin_text(); w.raw(" | "); }
                }
                continue;
            }
            if (tag_is(t, "dt") || tag_is(t, "dd")) { w.want_break(1); continue; }
            if (const char *mark = style_of(t)) {
                if (t.self_closing) continue;
                if (t.closing) w.pop_style();
                else w.push_style(mark);
                continue;
            }
            if (is_code(t) && !w.in_pre) {
                if (t.self_closing) continue;
                if (!t.closing) {
                    w.begin_text();
                    if (!w.code++) w.raw("`");
                } else if (w.code && !--w.code) {
                    w.raw("`");
                }
                continue;
            }
            if (is_block(t)) {
                /* A paragraph right at the start of a list item stays on
                 * the item's line. */
                if (!(w.list_mark[0] && !t.closing)) w.want_break(w.list_depth ? 1 : 2);
                continue;
            }
            continue;   /* span, a, sup, sub, ...: just their text */
        }
        if (skip) {
            while (p < end && *p != '<') p++;
            continue;
        }
        if (*p == '&') {
            char u[4];
            size_t ul = 0, used = decode_entity(p, end, u, &ul);
            if (used) {
                const char *q = p + used;
                w.text_char(u, ul, q < end ? *q : ' ');
                p = q;
                continue;
            }
        }
        unsigned char c = (unsigned char)*p;
        size_t n = c < 0x80 ? 1 : (c >> 5) == 6 ? 2 : (c >> 4) == 14 ? 3 : (c >> 3) == 30 ? 4 : 1;
        if (p + n > end) n = (size_t)(end - p);
        w.text_char(p, n, p + n < end ? p[n] : ' ');
        p += n;
    }
    /* Close what is still open and end with one newline. */
    while (!w.styles.empty()) w.pop_style();
    if (w.code) w.raw("`");
    if (w.in_pre) { if (w.last() != '\n') w.raw("\n"); w.raw("```"); }
    while (w.len && (w.dst[w.len - 1] == ' ' || w.dst[w.len - 1] == '\n')) w.len--;
    if (w.full) {
        size_t n = strlen(TRUNC_NOTE);
        memcpy(dst + w.len, TRUNC_NOTE, n);   /* RESERVE keeps the room */
        w.len += n;
    } else if (w.len) {
        dst[w.len++] = '\n';
    }
    return w.len;
}

extern "C" long epub_chapter_markdown(epub_book_t *b, int chapter, char *dst, size_t cap,
                                      const char **why)
{
    const char *dummy;
    if (!why) why = &dummy;
    if (!b || chapter < 0 || chapter >= (int)b->spine.size()) { *why = "No such chapter"; return -1; }
    size_t len = 0;
    char *x = zip_read(b, b->spine[chapter].c_str(), &len, why);
    if (!x) return -1;
    size_t n = epub_xhtml_to_markdown(x, len, dst, cap);
    heap_caps_free(x);
    return (long)n;
}
