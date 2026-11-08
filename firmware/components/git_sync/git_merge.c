/* git_merge.c -- flat three-way tree merge with a diff3 line merge and
 * textual conflict markers. */
#include "git_core.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <esp_log.h>
#include <esp_heap_caps.h>

static const char *TAG = "git.merge";

#define IO_CHUNK 4096

/* Myers' search gives up on the optimal alignment of a region after
 * this many edit steps and splits it heuristically (as GNU diff does),
 * which bounds the time on two very different texts. */
#define DIFF_TOO_EXPENSIVE 2048

/* Lines of the shared tail still merged line by line (see
 * common_ends()). */
#define SUFFIX_MARGIN 64

/* ------------------------------------------------------------------ */
/* line tables                                                        */
/* ------------------------------------------------------------------ */

/* Lines are compared by a 64-bit hash of their bytes (newline
 * included), so only the hashes -- and for the merge each line's
 * offset in its file -- are kept in RAM; the text stays on the SD card
 * or wherever the source reads it from. */
typedef struct {
    uint64_t *h;            /* per-line hash */
    uint32_t *off;          /* per-line start offset (NULL = not kept) */
    int       n, cap;
} lines_t;

static void lines_free(lines_t *L)
{
    free(L->h);
    free(L->off);
    memset(L, 0, sizeof(*L));
}

static bool lines_push(lines_t *L, uint64_t h, uint32_t off, bool keep_off)
{
    if (L->n >= L->cap) {
        int cap = L->cap ? L->cap * 2 : 64;
        uint64_t *nh = heap_caps_realloc(L->h, (size_t)cap * sizeof(*nh), MALLOC_CAP_SPIRAM);
        if (!nh) nh = realloc(L->h, (size_t)cap * sizeof(*nh));
        if (!nh) return false;
        L->h = nh;
        if (keep_off) {
            uint32_t *no = heap_caps_realloc(L->off, (size_t)cap * sizeof(*no), MALLOC_CAP_SPIRAM);
            if (!no) no = realloc(L->off, (size_t)cap * sizeof(*no));
            if (!no) return false;
            L->off = no;
        }
        L->cap = cap;
    }
    L->h[L->n] = h;
    if (keep_off) L->off[L->n] = off;
    L->n++;
    return true;
}

#define FNV_OFF 0xcbf29ce484222325ULL
#define FNV_PRIME 0x100000001b3ULL

/* Read the whole source, hashing each line; line offsets start at
 * base. A final line without a newline is a line of its own (it
 * differs from the same text with one). */
static esp_err_t lines_scan(git_reader *src, lines_t *L, bool keep_off, uint32_t base)
{
    memset(L, 0, sizeof(*L));
    uint8_t *buf = malloc(IO_CHUNK);
    if (!buf) return ESP_ERR_NO_MEM;
    esp_err_t ret = ESP_OK;
    uint64_t h = FNV_OFF;
    uint32_t start = base, pos = base;
    bool open = false;
    long r;
    while ((r = src->read(src->ctx, buf, IO_CHUNK)) > 0) {
        for (long i = 0; i < r; i++) {
            h = (h ^ buf[i]) * FNV_PRIME;
            open = true;
            pos++;
            if (buf[i] == '\n') {
                if (!lines_push(L, h ^ (pos - start), start, keep_off)) { ret = ESP_ERR_NO_MEM; goto out; }
                h = FNV_OFF;
                start = pos;
                open = false;
            }
        }
    }
    if (r < 0) { ret = ESP_FAIL; goto out; }
    if (open && !lines_push(L, h ^ (pos - start) ^ 0x8000000000000000ULL, start, keep_off))
        ret = ESP_ERR_NO_MEM;
out:
    free(buf);
    return ret;
}

static bool region_eq(const uint64_t *a, int a0, int a1,
                      const uint64_t *b, int b0, int b1)
{
    if (a1 - a0 != b1 - b0) return false;
    for (int i = 0; i < a1 - a0; i++)
        if (a[a0 + i] != b[b0 + i]) return false;
    return true;
}

/* ------------------------------------------------------------------ */
/* line alignment: Myers' O(ND) diff in linear space                  */
/* ------------------------------------------------------------------ */

/* Find the midpoint of the shortest edit script of x[xoff, xlim) and
 * y[yoff, ylim) (whose first and last lines differ), after gnulib's
 * diffseq.h. fd / bd are indexed by diagonal (x - y). */
static void diag(const uint64_t *xv, int xoff, int xlim,
                 const uint64_t *yv, int yoff, int ylim,
                 int *fd, int *bd, int *pxmid, int *pymid)
{
    const int dmin = xoff - ylim;
    const int dmax = xlim - yoff;
    const int fmid = xoff - yoff;
    const int bmid = xlim - ylim;
    int fmin = fmid, fmax = fmid;
    int bmin = bmid, bmax = bmid;
    const bool odd = (fmid - bmid) & 1;
    fd[fmid] = xoff;
    bd[bmid] = xlim;

    for (int c = 1;; ++c) {
        int d;
        /* extend the forward search by one edit */
        if (fmin > dmin) fd[--fmin - 1] = -1; else ++fmin;
        if (fmax < dmax) fd[++fmax + 1] = -1; else --fmax;
        for (d = fmax; d >= fmin; d -= 2) {
            int tlo = fd[d - 1], thi = fd[d + 1];
            int x = tlo >= thi ? tlo + 1 : thi;
            int y = x - d;
            while (x < xlim && y < ylim && xv[x] == yv[y]) { x++; y++; }
            fd[d] = x;
            if (odd && bmin <= d && d <= bmax && bd[d] <= x) {
                *pxmid = x; *pymid = y;
                return;
            }
        }
        /* and the backward search */
        if (bmin > dmin) bd[--bmin - 1] = INT_MAX; else ++bmin;
        if (bmax < dmax) bd[++bmax + 1] = INT_MAX; else --bmax;
        for (d = bmax; d >= bmin; d -= 2) {
            int tlo = bd[d - 1], thi = bd[d + 1];
            int x = tlo < thi ? tlo : thi - 1;
            int y = x - d;
            while (x > xoff && y > yoff && xv[x - 1] == yv[y - 1]) { x--; y--; }
            bd[d] = x;
            if (!odd && fmin <= d && d <= fmax && x <= fd[d]) {
                *pxmid = x; *pymid = y;
                return;
            }
        }
        if (c >= DIFF_TOO_EXPENSIVE) {
            /* Too costly: split at the furthest point either search
             * has reached. */
            int fxybest = -1, fxbest = xoff;
            for (d = fmax; d >= fmin; d -= 2) {
                int x = fd[d] < xlim ? fd[d] : xlim;
                int y = x - d;
                if (ylim < y) { x = ylim + d; y = ylim; }
                if (fxybest < x + y) { fxybest = x + y; fxbest = x; }
            }
            int bxybest = INT_MAX, bxbest = xlim;
            for (d = bmax; d >= bmin; d -= 2) {
                int x = bd[d] > xoff ? bd[d] : xoff;
                int y = x - d;
                if (y < yoff) { x = yoff + d; y = yoff; }
                if (x + y < bxybest) { bxybest = x + y; bxbest = x; }
            }
            if ((xlim + ylim) - bxybest < fxybest - (xoff + yoff)) {
                *pxmid = fxbest; *pymid = fxybest - fxbest;
            } else {
                *pxmid = bxbest; *pymid = bxybest - bxbest;
            }
            return;
        }
    }
}

typedef struct { int xoff, xlim, yoff, ylim; } diff_range;

/* match_a[i] = index in b matched to a[i], or -1. Memory is linear in
 * na + nb. Returns false when out of memory. */
static bool lcs_align(const uint64_t *a, int na, const uint64_t *b, int nb,
                      int *match_a)
{
    for (int i = 0; i < na; i++) match_a[i] = -1;
    if (na == 0 || nb == 0) return true;

    size_t ndiag = (size_t)na + nb + 3;
    int *fbuf = heap_caps_malloc(ndiag * sizeof(int), MALLOC_CAP_SPIRAM);
    if (!fbuf) fbuf = malloc(ndiag * sizeof(int));
    int *bbuf = heap_caps_malloc(ndiag * sizeof(int), MALLOC_CAP_SPIRAM);
    if (!bbuf) bbuf = malloc(ndiag * sizeof(int));
    int scap = 64, sn = 0;
    diff_range *stack = malloc((size_t)scap * sizeof(*stack));
    bool ok = fbuf && bbuf && stack;
    int *fd = fbuf + nb + 1, *bd = bbuf + nb + 1;

    if (ok) stack[sn++] = (diff_range){ 0, na, 0, nb };
    while (ok && sn) {
        diff_range r = stack[--sn];
        /* common lines at both ends need no search */
        while (r.xoff < r.xlim && r.yoff < r.ylim && a[r.xoff] == b[r.yoff])
            match_a[r.xoff++] = r.yoff++;
        while (r.xlim > r.xoff && r.ylim > r.yoff && a[r.xlim - 1] == b[r.ylim - 1])
            match_a[--r.xlim] = --r.ylim;
        if (r.xoff == r.xlim || r.yoff == r.ylim) continue;

        int xmid, ymid;
        diag(a, r.xoff, r.xlim, b, r.yoff, r.ylim, fd, bd, &xmid, &ymid);
        /* A split that does not divide the region (the heuristic can do
         * that) leaves it unmatched: still a valid alignment. */
        if ((xmid == r.xoff && ymid == r.yoff) || (xmid == r.xlim && ymid == r.ylim))
            continue;
        if (sn + 2 > scap) {
            scap *= 2;
            diff_range *ns = realloc(stack, (size_t)scap * sizeof(*ns));
            if (!ns) { ok = false; break; }
            stack = ns;
        }
        stack[sn++] = (diff_range){ r.xoff, xmid, r.yoff, ymid };
        stack[sn++] = (diff_range){ xmid, r.xlim, ymid, r.ylim };
    }
    free(fbuf);
    free(bbuf);
    free(stack);
    return ok;
}

/* Canonical form of an alignment, after git's xdl_change_compact().
 * Several alignments are often equally short -- which of two blank
 * lines was deleted? -- and the two sides of a merge must describe the
 * same edit the same way, or they conflict for nothing. A block of
 * changed lines can slide along lines equal to it; each block is moved
 * to the lowest position where it faces a change in the other text
 * (so a replaced line stays one deletion plus one insertion), else as
 * far down as it goes. Blocks never slide into one another.
 *
 * The alignment is the list of matched pairs P[0..K); gap k lies
 * before pair k, and each text has a (possibly empty) block of
 * changed lines in every gap. */
typedef struct { int i[2]; } line_pair;

static void compact_side(line_pair *P, int K, const uint64_t *h, int n, int me,
                         int n_other)
{
    int other = !me;
#define IX(k)  ((k) < 0 ? -1 : (k) >= K ? n : P[k].i[me])
#define OX(k)  ((k) < 0 ? -1 : (k) >= K ? n_other : P[k].i[other])
#define OTHER_CHANGED(gg) (OX(gg) > OX((gg) - 1) + 1)
    for (int k = 0; k <= K; k++) {
        int start = IX(k - 1) + 1, end = IX(k);
        if (start >= end) continue;            /* no block in this gap */
        int g = k;
        /* up as far as possible */
        while (g > 0 && IX(g - 1) == IX(g - 2) + 1 && h[end - 1] == h[IX(g - 1)]) {
            P[g - 1].i[me] = end - 1;
            g--;
            start = IX(g - 1) + 1;
            end = IX(g);
        }
        /* then down, noting where the other text has a change too */
        int best = OTHER_CHANGED(g) ? g : -1;
        while (g < K && IX(g + 1) == IX(g) + 1 && h[start] == h[IX(g)]) {
            P[g].i[me] = start;
            g++;
            start = IX(g - 1) + 1;
            end = IX(g);
            if (OTHER_CHANGED(g)) best = g;
        }
        /* back up to the best spot facing a change */
        while (best >= 0 && g > best) {
            P[g - 1].i[me] = end - 1;
            g--;
            start = IX(g - 1) + 1;
            end = IX(g);
        }
        k = g;                                  /* gaps above are done */
    }
#undef OTHER_CHANGED
#undef IX
#undef OX
}

static bool compact(const uint64_t *a, int na, const uint64_t *b, int nb, int *match_a)
{
    int K = 0;
    for (int i = 0; i < na; i++) if (match_a[i] >= 0) K++;
    line_pair *P = malloc((size_t)(K ? K : 1) * sizeof(*P));
    if (!P) return false;
    for (int i = 0, k = 0; i < na; i++)
        if (match_a[i] >= 0) { P[k].i[0] = i; P[k].i[1] = match_a[i]; k++; }
    compact_side(P, K, a, na, 0, nb);
    compact_side(P, K, b, nb, 1, na);
    for (int i = 0; i < na; i++) match_a[i] = -1;
    for (int k = 0; k < K; k++) match_a[P[k].i[0]] = P[k].i[1];
    free(P);
    return true;
}

/* ------------------------------------------------------------------ */
/* line diff statistics                                               */
/* ------------------------------------------------------------------ */

esp_err_t git_diff_line_counts(git_reader *a, git_reader *b,
                               int *added, int *deleted)
{
    *added = 0;
    *deleted = 0;
    lines_t A = {0}, B = {0};
    int *match = NULL;
    esp_err_t ret = lines_scan(a, &A, false, 0);
    if (ret == ESP_OK) ret = lines_scan(b, &B, false, 0);
    if (ret != ESP_OK) goto done;

    /* Common leading and trailing lines need no search. */
    int lo = 0;
    while (lo < A.n && lo < B.n && A.h[lo] == B.h[lo]) lo++;
    int ea = A.n, eb = B.n;
    while (ea > lo && eb > lo && A.h[ea - 1] == B.h[eb - 1]) { ea--; eb--; }
    int na = ea - lo, nb = eb - lo;

    ret = ESP_ERR_NO_MEM;
    match = malloc(((size_t)na + 1) * sizeof(int));
    if (!match) goto done;
    if (!lcs_align(A.h + lo, na, B.h + lo, nb, match)) goto done;
    int common = 0;
    for (int i = 0; i < na; i++)
        if (match[i] >= 0) common++;
    *deleted = na - common;
    *added = nb - common;
    ret = ESP_OK;

done:
    free(match);
    lines_free(&A);
    lines_free(&B);
    return ret;
}

/* ------------------------------------------------------------------ */
/* diff3 merge, file to file                                          */
/* ------------------------------------------------------------------ */

typedef struct {
    FILE    *f;
    uint32_t len;           /* file size */
    uint32_t end;           /* end of the region being merged */
    lines_t  L;
} side_t;

typedef struct {
    FILE    *f;
    uint8_t *buf;
    int      last;          /* last byte written, -1 = none */
    bool     err;
} out_t;

static void out_put(out_t *o, const void *p, size_t n)
{
    if (!n || o->err) return;
    if (fwrite(p, 1, n, o->f) != n) o->err = true;
    o->last = ((const uint8_t *)p)[n - 1];
}

static void out_puts(out_t *o, const char *s) { out_put(o, s, strlen(s)); }

/* Ensure the output ends with a newline before a marker line. */
static void ensure_nl(out_t *o)
{
    if (o->last >= 0 && o->last != '\n') out_put(o, "\n", 1);
}

static void out_marker(out_t *o, const char *m, const char *label)
{
    ensure_nl(o);
    out_puts(o, m);
    if (label) { out_puts(o, " "); out_puts(o, label); }
    out_puts(o, "\n");
}

/* Copy bytes [from, to) of a side's file. */
static void out_copy(out_t *o, FILE *f, uint32_t from, uint32_t to)
{
    if (o->err || from >= to) return;
    if (fseek(f, (long)from, SEEK_SET) != 0) { o->err = true; return; }
    while (from < to) {
        size_t c = to - from < IO_CHUNK ? to - from : IO_CHUNK;
        if (fread(o->buf, 1, c, f) != c) { o->err = true; return; }
        out_put(o, o->buf, c);
        from += (uint32_t)c;
    }
}

/* Lines [a, b) of a side. */
static void emit_lines(out_t *o, const side_t *s, int a, int b)
{
    if (a >= b) return;
    uint32_t to = (b < s->L.n) ? s->L.off[b] : s->end;
    out_copy(o, s->f, s->L.off[a], to);
}

/* Reads [pos, end) of a file. */
typedef struct { FILE *f; uint32_t left; } range_src;

static long range_read(void *ctx, void *buf, size_t n)
{
    range_src *r = (range_src *)ctx;
    if (n > r->left) n = r->left;
    if (n == 0) return 0;
    size_t got = fread(buf, 1, n, r->f);
    if (got != n) return -1;
    r->left -= (uint32_t)n;
    return (long)n;
}

/* Hash the lines of [from, s->end). */
static esp_err_t side_scan(side_t *s, uint32_t from)
{
    if (fseek(s->f, (long)from, SEEK_SET) != 0) return ESP_FAIL;
    range_src rs = { s->f, s->end - from };
    git_reader r = { range_read, &rs };
    return lines_scan(&r, &s->L, true, from);
}

/* Bytes that all three files share at the start (*pre) and at the end
 * (*suf), both cut back to whole lines and not overlapping. Only the
 * part in between needs a line table. */
static esp_err_t common_ends(side_t *sd[3], uint8_t *buf, uint32_t *pre, uint32_t *suf)
{
    uint32_t minlen = sd[0]->len;
    for (int i = 1; i < 3; i++) if (sd[i]->len < minlen) minlen = sd[i]->len;
    uint8_t *b[3] = { buf, buf + IO_CHUNK, buf + 2 * IO_CHUNK };

    /* prefix: up to the last newline before the first difference */
    uint32_t p = 0, last_nl = 0;
    bool diff = false;
    for (int i = 0; i < 3; i++) fseek(sd[i]->f, 0, SEEK_SET);
    while (!diff && p < minlen) {
        size_t c = minlen - p < IO_CHUNK ? minlen - p : IO_CHUNK;
        for (int i = 0; i < 3; i++)
            if (fread(b[i], 1, c, sd[i]->f) != c) return ESP_FAIL;
        for (size_t k = 0; k < c; k++) {
            if (b[0][k] != b[1][k] || b[0][k] != b[2][k]) { diff = true; break; }
            p++;
            if (b[0][k] == '\n') last_nl = p;
        }
    }
    *pre = last_nl;

    /* suffix: never into the prefix; it must start right after a
     * newline, so it is cut back to just past its first one */
    uint32_t maxs = minlen - *pre;
    uint32_t sfx = 0;
    diff = false;
    while (!diff && sfx < maxs) {
        size_t c = maxs - sfx < IO_CHUNK ? maxs - sfx : IO_CHUNK;
        for (int i = 0; i < 3; i++) {
            if (fseek(sd[i]->f, (long)(sd[i]->len - sfx - c), SEEK_SET) != 0 ||
                fread(b[i], 1, c, sd[i]->f) != c) return ESP_FAIL;
        }
        for (size_t k = c; k-- > 0;) {
            if (b[0][k] != b[1][k] || b[0][k] != b[2][k]) { diff = true; break; }
            sfx++;
        }
    }
    /* The suffix starts after its first newline, plus SUFFIX_MARGIN
     * more lines, so that changes can still slide down into the
     * region (see slide_down()). */
    uint32_t cut = sfx;            /* sfx = not enough newlines */
    uint32_t start = sd[0]->len - sfx, done = 0;
    int nls = 0;
    while (done < sfx && cut == sfx) {
        size_t c = sfx - done < IO_CHUNK ? sfx - done : IO_CHUNK;
        if (fseek(sd[0]->f, (long)(start + done), SEEK_SET) != 0 ||
            fread(b[0], 1, c, sd[0]->f) != c) return ESP_FAIL;
        for (size_t k = 0; k < c; k++) {
            if (b[0][k] == '\n' && ++nls > SUFFIX_MARGIN) { cut = done + (uint32_t)k + 1; break; }
        }
        done += (uint32_t)c;
    }
    *suf = sfx - cut;
    return ESP_OK;
}

/* diff3 of the files at base / ours / theirs into out. */
static esp_err_t diff3_merge(const char *base, const char *ours, const char *theirs,
                             const char *ours_label, const char *theirs_label,
                             out_t *out, bool *conflict)
{
    *conflict = false;
    side_t O, A, B;
    memset(&O, 0, sizeof(O)); memset(&A, 0, sizeof(A)); memset(&B, 0, sizeof(B));
    side_t *sd[3] = { &O, &A, &B };
    const char *paths[3] = { base, ours, theirs };
    int *ma = NULL, *mb = NULL;
    uint8_t *cbuf = NULL;
    uint32_t pre = 0, suf = 0;
    esp_err_t ret = ESP_OK;

    for (int i = 0; i < 3 && ret == ESP_OK; i++) {
        sd[i]->f = fopen(paths[i], "rb");
        if (!sd[i]->f || fseek(sd[i]->f, 0, SEEK_END) != 0) { ret = ESP_FAIL; break; }
        sd[i]->len = (uint32_t)ftell(sd[i]->f);
    }
    if (ret != ESP_OK) goto done;
    cbuf = malloc(3 * IO_CHUNK);
    if (!cbuf) { ret = ESP_ERR_NO_MEM; goto whole; }
    ret = common_ends(sd, cbuf, &pre, &suf);
    free(cbuf);
    if (ret != ESP_OK) goto done;

    for (int i = 0; i < 3 && ret == ESP_OK; i++) {
        sd[i]->end = sd[i]->len - suf;
        ret = side_scan(sd[i], pre);
    }
    if (ret == ESP_OK) {
        ma = calloc((size_t)(O.L.n ? O.L.n : 1), sizeof(int));
        mb = calloc((size_t)(O.L.n ? O.L.n : 1), sizeof(int));
        if (!ma || !mb ||
            !lcs_align(O.L.h, O.L.n, A.L.h, A.L.n, ma) ||
            !lcs_align(O.L.h, O.L.n, B.L.h, B.L.n, mb) ||
            !compact(O.L.h, O.L.n, A.L.h, A.L.n, ma) ||
            !compact(O.L.h, O.L.n, B.L.h, B.L.n, mb))
            ret = ESP_ERR_NO_MEM;
    }
    if (ret == ESP_ERR_NO_MEM) goto whole;
    if (ret != ESP_OK) goto done;

    out_copy(out, O.f, 0, pre);

    /* sync points: o indices matched in BOTH A and B (+ sentinels) */
    int op = -1, ap = -1, bp = -1;
    for (int k = 0; k <= O.L.n; k++) {
        int oc, ac, bc;
        if (k == O.L.n) { oc = O.L.n; ac = A.L.n; bc = B.L.n; }
        else if (ma[k] >= 0 && mb[k] >= 0 && ma[k] > ap && mb[k] > bp) {
            oc = k; ac = ma[k]; bc = mb[k];
        } else {
            continue;
        }

        int o0 = op + 1, a0 = ap + 1, b0 = bp + 1;
        bool a_changed = !region_eq(A.L.h, a0, ac, O.L.h, o0, oc);
        bool b_changed = !region_eq(B.L.h, b0, bc, O.L.h, o0, oc);

        if (!a_changed && !b_changed) {
            emit_lines(out, &O, o0, oc);
        } else if (a_changed && !b_changed) {
            emit_lines(out, &A, a0, ac);
        } else if (!a_changed && b_changed) {
            emit_lines(out, &B, b0, bc);
        } else if (region_eq(A.L.h, a0, ac, B.L.h, b0, bc)) {
            emit_lines(out, &A, a0, ac);     /* same change on both sides */
        } else {
            out_marker(out, "<<<<<<<", ours_label);
            emit_lines(out, &A, a0, ac);
            out_marker(out, "=======", NULL);
            emit_lines(out, &B, b0, bc);
            out_marker(out, ">>>>>>>", theirs_label);
            *conflict = true;
        }

        if (oc < O.L.n) emit_lines(out, &O, oc, oc + 1);
        op = oc; ap = ac; bp = bc;
    }
    out_copy(out, O.f, O.end, O.len);
    goto done;

whole:
    /* Out of memory for the line tables: a whole-file conflict. */
    ESP_LOGW(TAG, "merge: out of memory, whole-file conflict");
    ret = ESP_OK;
    out_marker(out, "<<<<<<<", ours_label);
    out_copy(out, A.f, 0, A.len);
    out_marker(out, "=======", NULL);
    out_copy(out, B.f, 0, B.len);
    out_marker(out, ">>>>>>>", theirs_label);
    *conflict = true;

done:
    free(ma);
    free(mb);
    for (int i = 0; i < 3; i++) {
        if (sd[i]->f) fclose(sd[i]->f);
        lines_free(&sd[i]->L);
    }
    if (ret == ESP_OK && out->err) ret = ESP_FAIL;
    return ret;
}

/* Write a blob to a scratch file (an absent blob: an empty file). */
static esp_err_t blob_to_file(const git_oid *oid, const char *path)
{
    if (!oid || git_oid_is_zero(oid)) {
        FILE *f = fopen(path, "wb");
        if (!f) return ESP_FAIL;
        return fclose(f) == 0 ? ESP_OK : ESP_FAIL;
    }
    git_obj_type t;
    esp_err_t ret = git_odb_read_to_file(oid, &t, path);
    if (ret == ESP_OK && t != GIT_OBJ_BLOB) return ESP_FAIL;
    return ret;
}

static bool name_seen(char (*names)[256], int n, const char *name)
{
    for (int i = 0; i < n; i++)
        if (strcmp(names[i], name) == 0) return true;
    return false;
}

esp_err_t git_merge_trees_flat(const git_oid *base_oid,
                               const git_oid *ours_oid,
                               const git_oid *theirs_oid,
                               const char *ours_label,
                               const char *theirs_label,
                               git_merge_path_result **out, int *out_count,
                               int *out_nconflicts)
{
    *out = NULL;
    *out_count = 0;
    *out_nconflicts = 0;

    git_tree base = {0}, ours = {0}, theirs = {0};
    esp_err_t ret;
    if ((ret = git_tree_load(base_oid, &base)) != ESP_OK) goto done;
    if ((ret = git_tree_load(ours_oid, &ours)) != ESP_OK) goto done;
    if ((ret = git_tree_load(theirs_oid, &theirs)) != ESP_OK) goto done;

    int cap = base.count + ours.count + theirs.count + 1;
    char (*names)[256] = calloc((size_t)cap, sizeof(*names));
    git_merge_path_result *res = calloc((size_t)cap, sizeof(*res));
    if (!names || !res) { free(names); free(res); ret = ESP_ERR_NO_MEM; goto done; }
    int nn = 0;

    git_tree *all[3] = { &ours, &theirs, &base };
    for (int s = 0; s < 3; s++) {
        for (int i = 0; i < all[s]->count; i++) {
            const char *nm = all[s]->entries[i].name;
            if (name_seen(names, nn, nm)) continue;
            strlcpy(names[nn], nm, sizeof(names[nn]));

            const git_tree_entry *be = git_tree_get(&base, nm);
            const git_tree_entry *oe = git_tree_get(&ours, nm);
            const git_tree_entry *te = git_tree_get(&theirs, nm);
            git_oid bo = be ? be->oid : (git_oid){0};
            git_oid oo = oe ? oe->oid : (git_oid){0};
            git_oid to = te ? te->oid : (git_oid){0};

            git_merge_path_result *r = &res[nn];
            strlcpy(r->name, nm, sizeof(r->name));
            r->conflict = false;
            /* Entries other than plain files (folders, symlinks,
             * submodules) cannot be line-merged. */
            bool plain = (!oe || (oe->mode & 0170000) == 0100000) &&
                         (!te || (te->mode & 0170000) == 0100000) &&
                         (!be || (be->mode & 0170000) == 0100000);

            if (git_oid_eq(&oo, &to)) {
                r->oid = oo;                       /* identical (incl. both deleted) */
                r->mode = oe ? oe->mode : 0;
            } else if (git_oid_eq(&oo, &bo)) {
                r->oid = to;                       /* only theirs changed */
                r->mode = te ? te->mode : 0;
            } else if (git_oid_eq(&to, &bo)) {
                r->oid = oo;                       /* only ours changed */
                r->mode = oe ? oe->mode : 0;
            } else if (git_oid_is_zero(&oo) || git_oid_is_zero(&to) || !plain) {
                /* delete/modify conflict: keep the surviving content;
                 * both sides changed a folder or other non-file entry:
                 * keep ours */
                bool take_ours = !git_oid_is_zero(&oo);
                r->oid = take_ours ? oo : to;
                r->mode = take_ours ? oe->mode : te->mode;
                r->conflict = true;
                (*out_nconflicts)++;
            } else {
                /* content conflict: diff3 line merge, through scratch
                 * files on the SD card */
                char pb[320], po[320], pt[320], pm[320];
                git_odb_tmp_path(pb, sizeof(pb), "merge-base");
                git_odb_tmp_path(po, sizeof(po), "merge-ours");
                git_odb_tmp_path(pt, sizeof(pt), "merge-theirs");
                git_odb_tmp_path(pm, sizeof(pm), "merge-out");
                bool conflict = false;
                ret = blob_to_file(&bo, pb);
                if (ret == ESP_OK) ret = blob_to_file(&oo, po);
                if (ret == ESP_OK) ret = blob_to_file(&to, pt);
                if (ret == ESP_OK) {
                    out_t o = { .f = fopen(pm, "wb"), .buf = malloc(IO_CHUNK), .last = -1 };
                    if (!o.f || !o.buf) ret = ESP_ERR_NO_MEM;
                    else ret = diff3_merge(pb, po, pt, ours_label, theirs_label, &o, &conflict);
                    if (o.f && fclose(o.f) != 0 && ret == ESP_OK) ret = ESP_FAIL;
                    free(o.buf);
                }
                if (ret == ESP_OK) ret = git_odb_write_file(GIT_OBJ_BLOB, pm, &r->oid);
                remove(pb); remove(po); remove(pt); remove(pm);
                if (ret != ESP_OK) goto done_res;
                r->mode = oe->mode;
                r->conflict = conflict;
                if (conflict) (*out_nconflicts)++;
                ESP_LOGI(TAG, "merged %s%s", nm, conflict ? " (CONFLICT)" : "");
            }
            /* The mode is merged on its own, like git does: a change of
             * mode on one side (100644 -> 100755) survives even where
             * the content came from the other side or did not change.
             * Both sides changing it differently keeps ours. A mode of
             * another entry type than the content taken is not used. */
            if (!git_oid_is_zero(&r->oid)) {
                uint32_t bm = be ? be->mode : 0, om = oe ? oe->mode : 0,
                         tm = te ? te->mode : 0;
                uint32_t m = (om == tm) ? om : (om == bm) ? tm : om;
                if (m && (m & 0170000) == (r->mode & 0170000)) r->mode = m;
            }
            nn++;
        }
    }

    ret = ESP_OK;
done_res:
    free(names);
    if (ret == ESP_OK) {
        *out = res;
        *out_count = nn;
    } else {
        free(res);
    }
done:
    git_tree_free(&base);
    git_tree_free(&ours);
    git_tree_free(&theirs);
    return ret;
}
