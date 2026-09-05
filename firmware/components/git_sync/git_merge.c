/* git_merge.c -- flat three-way tree merge with a diff3 line merge and
 * textual conflict markers. */
#include "git_core.h"

#include <stdlib.h>
#include <string.h>

#include <esp_log.h>
#include <esp_heap_caps.h>

static const char *TAG = "git.merge";

/* Above this many lines on any side we skip the line-level merge and
 * fall back to a whole-file conflict. Keeps the O(N*M) LCS matrix within
 * ~2 MB of PSRAM. */
#define MERGE_MAX_LINES 1400

/* ------------------------------------------------------------------ */
/* line splitting                                                     */
/* ------------------------------------------------------------------ */

typedef struct { const char *p; size_t n; } line_t;

typedef struct { line_t *v; int n; } lines_t;

static void lines_free(lines_t *L) { if (L->v) free(L->v); L->v = NULL; L->n = 0; }

static bool lines_split(const char *data, size_t len, lines_t *out)
{
    out->v = NULL;
    out->n = 0;
    int cap = 0;
    size_t i = 0;
    while (i < len) {
        size_t j = i;
        while (j < len && data[j] != '\n') j++;
        size_t end = (j < len) ? j + 1 : j;   /* include the '\n' */
        if (out->n >= cap) {
            cap = cap ? cap * 2 : 64;
            line_t *nv = realloc(out->v, (size_t)cap * sizeof(*nv));
            if (!nv) { lines_free(out); return false; }
            out->v = nv;
        }
        out->v[out->n].p = data + i;
        out->v[out->n].n = end - i;
        out->n++;
        i = end;
    }
    return true;
}

static bool line_eq(const line_t *a, const line_t *b)
{
    return a->n == b->n && memcmp(a->p, b->p, a->n) == 0;
}

static bool region_eq(const line_t *a, int a0, int a1,
                      const line_t *b, int b0, int b1)
{
    if (a1 - a0 != b1 - b0) return false;
    for (int i = 0; i < a1 - a0; i++)
        if (!line_eq(&a[a0 + i], &b[b0 + i])) return false;
    return true;
}

/* ------------------------------------------------------------------ */
/* LCS alignment: match_a[i] = index in b matched to a[i], or -1        */
/* ------------------------------------------------------------------ */

static bool lcs_align(const line_t *a, int na, const line_t *b, int nb,
                      int *match_a)
{
    for (int i = 0; i < na; i++) match_a[i] = -1;
    if (na == 0 || nb == 0) return true;

    /* dp is (na+1) x (nb+1) of uint16_t */
    size_t rows = (size_t)na + 1, cols = (size_t)nb + 1;
    uint16_t *dp = heap_caps_malloc(rows * cols * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    if (!dp) dp = malloc(rows * cols * sizeof(uint16_t));
    if (!dp) return false;
#define DP(i, j) dp[(size_t)(i) * cols + (j)]

    for (int j = 0; j <= nb; j++) DP(na, j) = 0;
    for (int i = 0; i <= na; i++) DP(i, nb) = 0;

    for (int i = na - 1; i >= 0; i--) {
        for (int j = nb - 1; j >= 0; j--) {
            if (line_eq(&a[i], &b[j]))
                DP(i, j) = (uint16_t)(DP(i + 1, j + 1) + 1);
            else {
                uint16_t d = DP(i + 1, j);
                uint16_t r = DP(i, j + 1);
                DP(i, j) = d > r ? d : r;
            }
        }
    }

    int i = 0, j = 0;
    while (i < na && j < nb) {
        if (line_eq(&a[i], &b[j])) {
            match_a[i] = j;
            i++; j++;
        } else if (DP(i + 1, j) >= DP(i, j + 1)) {
            i++;
        } else {
            j++;
        }
    }
#undef DP
    free(dp);
    return true;
}

/* ------------------------------------------------------------------ */
/* diff3 merge                                                        */
/* ------------------------------------------------------------------ */

static void emit_lines(git_buf *out, const line_t *v, int a, int b)
{
    for (int i = a; i < b; i++) git_buf_put(out, v[i].p, v[i].n);
}

/* Ensure the buffer ends with a newline before we add a marker line. */
static void ensure_nl(git_buf *out)
{
    if (out->len && out->data[out->len - 1] != '\n') git_buf_putc(out, '\n');
}

static esp_err_t diff3_merge(const char *base, size_t base_len,
                             const char *ours, size_t ours_len,
                             const char *theirs, size_t theirs_len,
                             const char *ours_label, const char *theirs_label,
                             git_buf *out, bool *conflict)
{
    *conflict = false;
    lines_t O = {0}, A = {0}, B = {0};
    esp_err_t ret = ESP_ERR_NO_MEM;
    if (!lines_split(base, base_len, &O)) goto done;
    if (!lines_split(ours, ours_len, &A)) goto done;
    if (!lines_split(theirs, theirs_len, &B)) goto done;

    if (O.n > MERGE_MAX_LINES || A.n > MERGE_MAX_LINES || B.n > MERGE_MAX_LINES) {
        /* coarse whole-file conflict */
        ensure_nl(out);
        git_buf_printf(out, "<<<<<<< %s\n", ours_label);
        git_buf_put(out, ours, ours_len);
        ensure_nl(out);
        git_buf_puts(out, "=======\n");
        git_buf_put(out, theirs, theirs_len);
        ensure_nl(out);
        git_buf_printf(out, ">>>>>>> %s\n", theirs_label);
        *conflict = true;
        ret = ESP_OK;
        goto done;
    }

    int *ma = calloc((size_t)(O.n ? O.n : 1), sizeof(int));
    int *mb = calloc((size_t)(O.n ? O.n : 1), sizeof(int));
    if (!ma || !mb) { free(ma); free(mb); goto done; }
    if (!lcs_align(O.v, O.n, A.v, A.n, ma) ||
        !lcs_align(O.v, O.n, B.v, B.n, mb)) {
        free(ma); free(mb);
        goto done;
    }

    /* sync points: o indices matched in BOTH A and B (+ sentinels) */
    int op = -1, ap = -1, bp = -1;
    for (int k = 0; k <= O.n; k++) {
        int oc, ac, bc;
        if (k == O.n) { oc = O.n; ac = A.n; bc = B.n; }
        else if (ma[k] >= 0 && mb[k] >= 0 && ma[k] > ap && mb[k] > bp) {
            oc = k; ac = ma[k]; bc = mb[k];
        } else {
            continue;
        }

        int o0 = op + 1, a0 = ap + 1, b0 = bp + 1;
        bool a_changed = !region_eq(A.v, a0, ac, O.v, o0, oc);
        bool b_changed = !region_eq(B.v, b0, bc, O.v, o0, oc);

        if (!a_changed && !b_changed) {
            emit_lines(out, O.v, o0, oc);
        } else if (a_changed && !b_changed) {
            emit_lines(out, A.v, a0, ac);
        } else if (!a_changed && b_changed) {
            emit_lines(out, B.v, b0, bc);
        } else if (region_eq(A.v, a0, ac, B.v, b0, bc)) {
            emit_lines(out, A.v, a0, ac);     /* same change on both sides */
        } else {
            ensure_nl(out);
            git_buf_printf(out, "<<<<<<< %s\n", ours_label);
            emit_lines(out, A.v, a0, ac);
            ensure_nl(out);
            git_buf_puts(out, "=======\n");
            emit_lines(out, B.v, b0, bc);
            ensure_nl(out);
            git_buf_printf(out, ">>>>>>> %s\n", theirs_label);
            *conflict = true;
        }

        if (oc < O.n) git_buf_put(out, O.v[oc].p, O.v[oc].n);
        op = oc; ap = ac; bp = bc;
    }

    free(ma);
    free(mb);
    ret = ESP_OK;

done:
    lines_free(&O);
    lines_free(&A);
    lines_free(&B);
    return ret;
}

/* ------------------------------------------------------------------ */
/* flat tree merge                                                    */
/* ------------------------------------------------------------------ */

static esp_err_t load_blob(const git_oid *oid, git_buf *out)
{
    if (!oid || git_oid_is_zero(oid)) return ESP_OK;   /* absent => empty */
    git_obj_type t;
    esp_err_t ret = git_odb_read(oid, &t, out);
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

            if (git_oid_eq(&oo, &to)) {
                r->oid = oo;                       /* identical (incl. both deleted) */
            } else if (git_oid_eq(&oo, &bo)) {
                r->oid = to;                       /* only theirs changed */
            } else if (git_oid_eq(&to, &bo)) {
                r->oid = oo;                       /* only ours changed */
            } else if (git_oid_is_zero(&oo) || git_oid_is_zero(&to)) {
                /* delete/modify conflict: keep the surviving content */
                r->oid = git_oid_is_zero(&oo) ? to : oo;
                r->conflict = true;
                (*out_nconflicts)++;
            } else {
                /* content conflict: diff3 line merge */
                git_buf bb = GIT_BUF_INIT, ob = GIT_BUF_INIT, tb = GIT_BUF_INIT;
                git_buf merged = GIT_BUF_INIT;
                bool conflict = false;
                ret = load_blob(&bo, &bb);
                if (ret == ESP_OK) ret = load_blob(&oo, &ob);
                if (ret == ESP_OK) ret = load_blob(&to, &tb);
                if (ret == ESP_OK)
                    ret = diff3_merge((char *)bb.data, bb.len,
                                      (char *)ob.data, ob.len,
                                      (char *)tb.data, tb.len,
                                      ours_label, theirs_label,
                                      &merged, &conflict);
                git_buf_free(&bb); git_buf_free(&ob); git_buf_free(&tb);
                if (ret != ESP_OK) { git_buf_free(&merged); goto done_res; }

                ret = git_odb_write(GIT_OBJ_BLOB, merged.data, merged.len, &r->oid);
                git_buf_free(&merged);
                if (ret != ESP_OK) goto done_res;
                r->conflict = conflict;
                if (conflict) (*out_nconflicts)++;
                ESP_LOGI(TAG, "merged %s%s", nm, conflict ? " (CONFLICT)" : "");
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
