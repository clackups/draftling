/* git_odb.c -- repository layout, loose object store, refs, trees,
 * commits and history walking. */
#include "git_core.h"

#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <esp_log.h>
#include <esp_heap_caps.h>

static const char *TAG = "git.odb";

static char s_workdir[256];
static char s_gitdir[288];
static bool s_open;

const char *git_repo_gitdir(void)  { return s_gitdir; }
const char *git_repo_workdir(void) { return s_workdir; }

/* ------------------------------------------------------------------ */
/* filesystem helpers                                                 */
/* ------------------------------------------------------------------ */

static int mkdir_p(const char *path)
{
    char tmp[320];
    size_t n = strlcpy(tmp, path, sizeof(tmp));
    if (n >= sizeof(tmp)) return -1;
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, 0777) != 0 && errno != EEXIST) { *p = '/'; }
            *p = '/';
        }
    }
    if (mkdir(tmp, 0777) != 0 && errno != EEXIST) return -1;
    return 0;
}

static bool file_exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0;
}

static esp_err_t read_whole_file(const char *path, git_buf *out)
{
    FILE *f = fopen(path, "rb");
    if (!f) return ESP_ERR_NOT_FOUND;
    uint8_t chunk[1024];
    size_t r;
    esp_err_t ret = ESP_OK;
    while ((r = fread(chunk, 1, sizeof(chunk), f)) > 0) {
        if (!git_buf_put(out, chunk, r)) { ret = ESP_ERR_NO_MEM; break; }
    }
    fclose(f);
    return ret;
}

static esp_err_t write_whole_file(const char *path, const void *data, size_t len)
{
    char tmp[352];
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    FILE *f = fopen(tmp, "wb");
    if (!f) return ESP_FAIL;
    bool ok = (len == 0) || (fwrite(data, 1, len, f) == len);
    if (fclose(f) != 0) ok = false;
    if (!ok) { remove(tmp); return ESP_FAIL; }
    remove(path);
    if (rename(tmp, path) != 0) { remove(tmp); return ESP_FAIL; }
    return ESP_OK;
}

/* ------------------------------------------------------------------ */
/* repo open                                                          */
/* ------------------------------------------------------------------ */

esp_err_t git_repo_open(const char *workdir)
{
    strlcpy(s_workdir, workdir, sizeof(s_workdir));
    size_t n = strlen(s_workdir);
    while (n > 1 && s_workdir[n - 1] == '/') s_workdir[--n] = '\0';
    snprintf(s_gitdir, sizeof(s_gitdir), "%s/.git", s_workdir);

    char path[320];
    if (mkdir_p(s_gitdir) != 0) {
        ESP_LOGE(TAG, "cannot create %s", s_gitdir);
        return ESP_FAIL;
    }
    snprintf(path, sizeof(path), "%s/objects", s_gitdir);  mkdir_p(path);
    snprintf(path, sizeof(path), "%s/refs/heads", s_gitdir); mkdir_p(path);
    snprintf(path, sizeof(path), "%s/refs/remotes/origin", s_gitdir); mkdir_p(path);
    snprintf(path, sizeof(path), "%s/tmp", s_gitdir); mkdir_p(path);

    snprintf(path, sizeof(path), "%s/HEAD", s_gitdir);
    if (!file_exists(path))
        write_whole_file(path, "ref: refs/heads/main\n", 21);

    s_open = true;
    return ESP_OK;
}

void git_repo_close(void)
{
    s_open = false;
}

/* ------------------------------------------------------------------ */
/* loose object store                                                 */
/* ------------------------------------------------------------------ */

static void object_path(const git_oid *oid, char *out, size_t outsz)
{
    snprintf(out, outsz, "%s/objects/%c%c/%s", s_gitdir,
             oid->hex[0], oid->hex[1], oid->hex + 2);
}

bool git_odb_exists(const git_oid *oid)
{
    char p[352];
    object_path(oid, p, sizeof(p));
    return file_exists(p);
}

void git_odb_hash(git_obj_type type, const void *data, size_t len, git_oid *out)
{
    char hdr[32];
    int hlen = snprintf(hdr, sizeof(hdr), "%s %zu", git_obj_type_name(type), len);
    git_sha1_ctx *c = git_sha1_new();
    uint8_t raw[GIT_OID_RAWSZ];
    git_sha1_update(c, hdr, (size_t)hlen + 1);   /* include the NUL */
    git_sha1_update(c, data, len);
    git_sha1_final(c, raw);
    git_oid_from_raw(out, raw);
}

esp_err_t git_odb_write(git_obj_type type, const void *data, size_t len,
                        git_oid *out_oid)
{
    git_oid oid;
    git_odb_hash(type, data, len, &oid);
    if (out_oid) *out_oid = oid;

    if (git_odb_exists(&oid)) return ESP_OK;

    char hdr[32];
    int hlen = snprintf(hdr, sizeof(hdr), "%s %zu", git_obj_type_name(type), len);

    git_buf raw = GIT_BUF_INIT, z = GIT_BUF_INIT;
    esp_err_t ret = ESP_ERR_NO_MEM;
    if (!git_buf_put(&raw, hdr, (size_t)hlen + 1)) goto out;
    if (!git_buf_put(&raw, data, len)) goto out;
    ret = git_zlib_deflate(raw.data, raw.len, &z);
    if (ret != ESP_OK) goto out;

    char dir[352], p[352];
    snprintf(dir, sizeof(dir), "%s/objects/%c%c", s_gitdir, oid.hex[0], oid.hex[1]);
    mkdir(dir, 0777);
    object_path(&oid, p, sizeof(p));
    ret = write_whole_file(p, z.data, z.len);

out:
    git_buf_free(&raw);
    git_buf_free(&z);
    return ret;
}

esp_err_t git_odb_read(const git_oid *oid, git_obj_type *type, git_buf *out)
{
    char p[352];
    object_path(oid, p, sizeof(p));

    git_buf comp = GIT_BUF_INIT, raw = GIT_BUF_INIT;
    esp_err_t ret = read_whole_file(p, &comp);
    if (ret != ESP_OK) { git_buf_free(&comp); return ret; }

    ret = git_zlib_inflate_buf(comp.data, comp.len, &raw);
    git_buf_free(&comp);
    if (ret != ESP_OK) { git_buf_free(&raw); return ret; }

    /* header: "<type> <size>\0" */
    size_t sp = 0;
    while (sp < raw.len && raw.data[sp] != ' ') sp++;
    size_t nul = sp;
    while (nul < raw.len && raw.data[nul] != '\0') nul++;
    if (sp >= raw.len || nul >= raw.len) { git_buf_free(&raw); return ESP_FAIL; }

    char tname[16] = {0};
    if (sp >= sizeof(tname)) { git_buf_free(&raw); return ESP_FAIL; }
    memcpy(tname, raw.data, sp);
    git_obj_type t = git_obj_type_from_name(tname);
    size_t bodylen = raw.len - (nul + 1);
    size_t declared = strtoul((char *)raw.data + sp + 1, NULL, 10);
    if (t == GIT_OBJ_BAD || declared != bodylen) { git_buf_free(&raw); return ESP_FAIL; }

    if (type) *type = t;
    bool ok = git_buf_put(out, raw.data + nul + 1, bodylen);
    git_buf_free(&raw);
    return ok ? ESP_OK : ESP_ERR_NO_MEM;
}

esp_err_t git_odb_read_type(const git_oid *oid, git_obj_type *type, size_t *size)
{
    git_buf b = GIT_BUF_INIT;
    esp_err_t ret = git_odb_read(oid, type, &b);
    if (ret == ESP_OK && size) *size = b.len;
    git_buf_free(&b);
    return ret;
}

/* ------------------------------------------------------------------ */
/* refs                                                               */
/* ------------------------------------------------------------------ */

static void ref_path(const char *name, char *out, size_t outsz)
{
    snprintf(out, outsz, "%s/%s", s_gitdir, name);
}

esp_err_t git_ref_read(const char *name, git_oid *out)
{
    char p[352];
    ref_path(name, p, sizeof(p));

    git_buf b = GIT_BUF_INIT;
    esp_err_t ret = read_whole_file(p, &b);
    if (ret != ESP_OK) { git_buf_free(&b); return ESP_ERR_NOT_FOUND; }

    /* trim */
    while (b.len && (b.data[b.len - 1] == '\n' || b.data[b.len - 1] == '\r' ||
                     b.data[b.len - 1] == ' '))
        b.data[--b.len] = '\0';

    if (b.len > 5 && memcmp(b.data, "ref: ", 5) == 0) {
        char target[128];
        strlcpy(target, (char *)b.data + 5, sizeof(target));
        git_buf_free(&b);
        return git_ref_read(target, out);
    }

    if (b.len < GIT_OID_HEXSZ) { git_buf_free(&b); return ESP_FAIL; }
    memcpy(out->hex, b.data, GIT_OID_HEXSZ);
    out->hex[GIT_OID_HEXSZ] = '\0';
    git_buf_free(&b);
    return ESP_OK;
}

esp_err_t git_ref_write(const char *name, const git_oid *oid)
{
    char p[352];
    ref_path(name, p, sizeof(p));
    char line[GIT_OID_HEXSZ + 2];
    snprintf(line, sizeof(line), "%s\n", oid->hex);
    return write_whole_file(p, line, strlen(line));
}

esp_err_t git_ref_delete(const char *name)
{
    char p[352];
    ref_path(name, p, sizeof(p));
    remove(p);
    return ESP_OK;
}

esp_err_t git_head_set_symbolic(const char *target)
{
    char p[352];
    ref_path("HEAD", p, sizeof(p));
    char line[160];
    snprintf(line, sizeof(line), "ref: %s\n", target);
    return write_whole_file(p, line, strlen(line));
}

/* ------------------------------------------------------------------ */
/* trees                                                              */
/* ------------------------------------------------------------------ */

void git_tree_free(git_tree *t)
{
    if (t->entries) heap_caps_free(t->entries);
    t->entries = NULL;
    t->count = t->cap = 0;
}

static bool tree_grow(git_tree *t)
{
    if (t->count < t->cap) return true;
    int nc = t->cap ? t->cap * 2 : 16;
    git_tree_entry *ne = heap_caps_realloc(t->entries, (size_t)nc * sizeof(*ne),
                                           MALLOC_CAP_SPIRAM);
    if (!ne) ne = realloc(t->entries, (size_t)nc * sizeof(*ne));
    if (!ne) return false;
    t->entries = ne;
    t->cap = nc;
    return true;
}

/* Git's tree sort: compare names, treating a dir as name + "/". */
static int tree_name_cmp(const char *an, bool adir, const char *bn, bool bdir)
{
    size_t al = strlen(an), bl = strlen(bn);
    size_t ml = al < bl ? al : bl;
    int c = memcmp(an, bn, ml);
    if (c) return c;
    unsigned char ac = al > ml ? (unsigned char)an[ml] : (adir ? '/' : 0);
    unsigned char bc = bl > ml ? (unsigned char)bn[ml] : (bdir ? '/' : 0);
    return (int)ac - (int)bc;
}

static int tree_entry_cmp(const void *pa, const void *pb)
{
    const git_tree_entry *a = pa, *b = pb;
    return tree_name_cmp(a->name, (a->mode & 040000) == 040000 && !(a->mode & 0100000),
                         b->name, (b->mode & 040000) == 040000 && !(b->mode & 0100000));
}

esp_err_t git_tree_parse(const uint8_t *data, size_t len, git_tree *out)
{
    memset(out, 0, sizeof(*out));
    size_t i = 0;
    while (i < len) {
        size_t sp = i;
        while (sp < len && data[sp] != ' ') sp++;
        if (sp >= len) return ESP_FAIL;
        size_t nul = sp + 1;
        while (nul < len && data[nul] != '\0') nul++;
        if (nul + 1 + GIT_OID_RAWSZ > len) return ESP_FAIL;

        if (!tree_grow(out)) return ESP_ERR_NO_MEM;
        git_tree_entry *e = &out->entries[out->count++];
        memset(e, 0, sizeof(*e));
        e->mode = (uint32_t)strtoul((const char *)data + i, NULL, 8);
        size_t nlen = nul - (sp + 1);
        if (nlen >= sizeof(e->name)) return ESP_FAIL;
        memcpy(e->name, data + sp + 1, nlen);
        git_oid_from_raw(&e->oid, data + nul + 1);
        i = nul + 1 + GIT_OID_RAWSZ;
    }
    return ESP_OK;
}

esp_err_t git_tree_load(const git_oid *oid, git_tree *out)
{
    memset(out, 0, sizeof(*out));
    if (git_oid_is_zero(oid)) return ESP_OK;   /* empty tree */
    git_obj_type t;
    git_buf b = GIT_BUF_INIT;
    esp_err_t ret = git_odb_read(oid, &t, &b);
    if (ret != ESP_OK) { git_buf_free(&b); return ret; }
    if (t != GIT_OBJ_TREE) { git_buf_free(&b); return ESP_FAIL; }
    ret = git_tree_parse(b.data, b.len, out);
    git_buf_free(&b);
    return ret;
}

const git_tree_entry *git_tree_get(const git_tree *t, const char *name)
{
    for (int i = 0; i < t->count; i++)
        if (strcmp(t->entries[i].name, name) == 0) return &t->entries[i];
    return NULL;
}

bool git_tree_set(git_tree *t, uint32_t mode, const char *name, const git_oid *oid)
{
    for (int i = 0; i < t->count; i++) {
        if (strcmp(t->entries[i].name, name) == 0) {
            if (!oid || git_oid_is_zero(oid)) {
                memmove(&t->entries[i], &t->entries[i + 1],
                        (size_t)(t->count - i - 1) * sizeof(*t->entries));
                t->count--;
            } else {
                t->entries[i].mode = mode;
                t->entries[i].oid = *oid;
            }
            return true;
        }
    }
    if (!oid || git_oid_is_zero(oid)) return true;
    if (!tree_grow(t)) return false;
    git_tree_entry *e = &t->entries[t->count++];
    memset(e, 0, sizeof(*e));
    e->mode = mode;
    strlcpy(e->name, name, sizeof(e->name));
    e->oid = *oid;
    return true;
}

esp_err_t git_tree_write(const git_tree *t, git_oid *out_oid)
{
    /* copy + sort */
    git_tree tmp = {0};
    for (int i = 0; i < t->count; i++) {
        if (!tree_grow(&tmp)) { git_tree_free(&tmp); return ESP_ERR_NO_MEM; }
        tmp.entries[tmp.count++] = t->entries[i];
    }
    qsort(tmp.entries, tmp.count, sizeof(*tmp.entries), tree_entry_cmp);

    git_buf b = GIT_BUF_INIT;
    esp_err_t ret = ESP_ERR_NO_MEM;
    for (int i = 0; i < tmp.count; i++) {
        uint8_t raw[GIT_OID_RAWSZ];
        if (!git_oid_to_raw(raw, &tmp.entries[i].oid)) { ret = ESP_FAIL; goto out; }
        if (!git_buf_printf(&b, "%o %s", tmp.entries[i].mode, tmp.entries[i].name))
            goto out;
        if (!git_buf_putc(&b, 0)) goto out;
        if (!git_buf_put(&b, raw, GIT_OID_RAWSZ)) goto out;
    }
    ret = git_odb_write(GIT_OBJ_TREE, b.data, b.len, out_oid);

out:
    git_buf_free(&b);
    git_tree_free(&tmp);
    return ret;
}

esp_err_t git_tree_subtree_oid(const git_oid *root, const char *prefix, git_oid *out)
{
    if (!prefix || !*prefix) { *out = *root; return ESP_OK; }

    char buf[256];
    strlcpy(buf, prefix, sizeof(buf));
    git_oid cur = *root;

    char *save = NULL;
    for (char *tok = strtok_r(buf, "/", &save); tok; tok = strtok_r(NULL, "/", &save)) {
        if (!*tok) continue;
        git_tree t;
        esp_err_t ret = git_tree_load(&cur, &t);
        if (ret != ESP_OK) return ret;
        const git_tree_entry *e = git_tree_get(&t, tok);
        if (!e || (e->mode & 040000) != 040000) { git_tree_free(&t); return ESP_ERR_NOT_FOUND; }
        cur = e->oid;
        git_tree_free(&t);
    }
    *out = cur;
    return ESP_OK;
}

/* recursive splice helper */
static esp_err_t splice_rec(const git_oid *node, char **toks, int ntok, int depth,
                            const git_oid *sub, git_oid *out)
{
    if (depth == ntok) { *out = *sub; return ESP_OK; }

    git_tree t;
    esp_err_t ret = git_tree_load(node, &t);
    if (ret != ESP_OK) return ret;

    const git_tree_entry *e = git_tree_get(&t, toks[depth]);
    git_oid child = {0};
    if (e && (e->mode & 040000) == 040000) child = e->oid;

    git_oid newchild;
    ret = splice_rec(&child, toks, ntok, depth + 1, sub, &newchild);
    if (ret != ESP_OK) { git_tree_free(&t); return ret; }

    if (!git_tree_set(&t, 040000, toks[depth],
                      git_oid_is_zero(&newchild) ? NULL : &newchild)) {
        git_tree_free(&t);
        return ESP_ERR_NO_MEM;
    }

    if (t.count == 0) { git_oid_clear(out); git_tree_free(&t); return ESP_OK; }
    ret = git_tree_write(&t, out);
    git_tree_free(&t);
    return ret;
}

esp_err_t git_tree_splice_subtree(const git_oid *root, const char *prefix,
                                  const git_oid *sub, git_oid *out_root)
{
    if (!prefix || !*prefix) { *out_root = *sub; return ESP_OK; }

    char buf[256];
    strlcpy(buf, prefix, sizeof(buf));
    char *toks[16];
    int ntok = 0;
    char *save = NULL;
    for (char *tok = strtok_r(buf, "/", &save); tok && ntok < 16;
         tok = strtok_r(NULL, "/", &save)) {
        if (*tok) toks[ntok++] = tok;
    }
    if (ntok == 0) { *out_root = *sub; return ESP_OK; }

    git_oid base = {0};
    if (root && !git_oid_is_zero(root)) base = *root;
    return splice_rec(&base, toks, ntok, 0, sub, out_root);
}

/* ------------------------------------------------------------------ */
/* commits                                                            */
/* ------------------------------------------------------------------ */

void git_commit_free(git_commit *c)
{
    if (c->message) free(c->message);
    c->message = NULL;
}

esp_err_t git_commit_parse(const uint8_t *data, size_t len, git_commit *out)
{
    memset(out, 0, sizeof(*out));
    const char *p = (const char *)data;
    const char *end = p + len;

    while (p < end) {
        const char *nl = memchr(p, '\n', (size_t)(end - p));
        if (!nl) break;
        size_t linelen = (size_t)(nl - p);
        if (linelen == 0) { p = nl + 1; break; }   /* header/body separator */

        if (linelen > 5 && memcmp(p, "tree ", 5) == 0 && linelen >= 5 + GIT_OID_HEXSZ) {
            memcpy(out->tree.hex, p + 5, GIT_OID_HEXSZ);
            out->tree.hex[GIT_OID_HEXSZ] = '\0';
        } else if (linelen > 7 && memcmp(p, "parent ", 7) == 0 &&
                   out->nparents < GIT_MAX_PARENTS && linelen >= 7 + GIT_OID_HEXSZ) {
            memcpy(out->parents[out->nparents].hex, p + 7, GIT_OID_HEXSZ);
            out->parents[out->nparents].hex[GIT_OID_HEXSZ] = '\0';
            out->nparents++;
        } else if (linelen > 7 && memcmp(p, "author ", 7) == 0) {
            size_t n = linelen - 7;
            if (n >= sizeof(out->author)) n = sizeof(out->author) - 1;
            memcpy(out->author, p + 7, n);
            out->author[n] = '\0';
        } else if (linelen > 10 && memcmp(p, "committer ", 10) == 0) {
            size_t n = linelen - 10;
            if (n >= sizeof(out->committer)) n = sizeof(out->committer) - 1;
            memcpy(out->committer, p + 10, n);
            out->committer[n] = '\0';
        }
        p = nl + 1;
    }

    size_t mlen = (size_t)(end - p);
    out->message = malloc(mlen + 1);
    if (!out->message) return ESP_ERR_NO_MEM;
    memcpy(out->message, p, mlen);
    out->message[mlen] = '\0';
    return ESP_OK;
}

esp_err_t git_commit_load(const git_oid *oid, git_commit *out)
{
    git_obj_type t;
    git_buf b = GIT_BUF_INIT;
    esp_err_t ret = git_odb_read(oid, &t, &b);
    if (ret != ESP_OK) { git_buf_free(&b); return ret; }
    if (t != GIT_OBJ_COMMIT) { git_buf_free(&b); return ESP_FAIL; }
    ret = git_commit_parse(b.data, b.len, out);
    git_buf_free(&b);
    return ret;
}

esp_err_t git_commit_create(const git_oid *tree,
                            const git_oid *parents, int nparents,
                            const char *ident, const char *message,
                            git_oid *out_oid)
{
    git_buf b = GIT_BUF_INIT;
    esp_err_t ret = ESP_ERR_NO_MEM;
    if (!git_buf_printf(&b, "tree %s\n", tree->hex)) goto out;
    for (int i = 0; i < nparents; i++)
        if (!git_buf_printf(&b, "parent %s\n", parents[i].hex)) goto out;
    if (!git_buf_printf(&b, "author %s\n", ident)) goto out;
    if (!git_buf_printf(&b, "committer %s\n\n", ident)) goto out;
    if (!git_buf_puts(&b, message)) goto out;
    if (b.len && b.data[b.len - 1] != '\n') git_buf_putc(&b, '\n');
    ret = git_odb_write(GIT_OBJ_COMMIT, b.data, b.len, out_oid);
out:
    git_buf_free(&b);
    return ret;
}

/* ------------------------------------------------------------------ */
/* history walk                                                       */
/* ------------------------------------------------------------------ */

void git_oidset_free(git_oidset *s)
{
    if (s->v) heap_caps_free(s->v);
    s->v = NULL;
    s->n = s->cap = 0;
}

bool git_oidset_has(const git_oidset *s, const git_oid *o)
{
    for (int i = 0; i < s->n; i++)
        if (git_oid_eq(&s->v[i], o)) return true;
    return false;
}

bool git_oidset_add(git_oidset *s, const git_oid *o)
{
    if (git_oidset_has(s, o)) return true;
    if (s->n >= s->cap) {
        int nc = s->cap ? s->cap * 2 : 64;
        git_oid *nv = heap_caps_realloc(s->v, (size_t)nc * sizeof(*nv), MALLOC_CAP_SPIRAM);
        if (!nv) nv = realloc(s->v, (size_t)nc * sizeof(*nv));
        if (!nv) return false;
        s->v = nv;
        s->cap = nc;
    }
    s->v[s->n++] = *o;
    return true;
}

bool git_is_ancestor(const git_oid *anc, const git_oid *desc)
{
    if (git_oid_is_zero(anc)) return true;
    if (git_oid_eq(anc, desc)) return true;

    git_oidset seen = {0}, queue = {0};
    git_oidset_add(&queue, desc);
    bool found = false;
    for (int i = 0; i < queue.n && !found; i++) {
        git_oid cur = queue.v[i];
        if (git_oidset_has(&seen, &cur)) continue;
        git_oidset_add(&seen, &cur);
        git_commit c;
        if (git_commit_load(&cur, &c) != ESP_OK) continue;
        for (int p = 0; p < c.nparents; p++) {
            if (git_oid_eq(&c.parents[p], anc)) { found = true; break; }
            git_oidset_add(&queue, &c.parents[p]);
        }
        git_commit_free(&c);
    }
    git_oidset_free(&seen);
    git_oidset_free(&queue);
    return found;
}

/* BFS from `a`, record depth; then BFS from `b` and return the first
 * commit already seen from `a` (approximate LCA, good enough for the
 * linear + one-diverging-branch topology Draftling produces). */
esp_err_t git_merge_base(const git_oid *a, const git_oid *b, git_oid *out)
{
    git_oid_clear(out);
    if (git_oid_is_zero(a) || git_oid_is_zero(b)) return ESP_OK;
    if (git_oid_eq(a, b)) { *out = *a; return ESP_OK; }

    git_oidset seen_a = {0}, q = {0};
    git_oidset_add(&q, a);
    for (int i = 0; i < q.n; i++) {
        git_oid cur = q.v[i];
        if (git_oidset_has(&seen_a, &cur)) continue;
        git_oidset_add(&seen_a, &cur);
        git_commit c;
        if (git_commit_load(&cur, &c) != ESP_OK) continue;
        for (int p = 0; p < c.nparents; p++) git_oidset_add(&q, &c.parents[p]);
        git_commit_free(&c);
    }
    git_oidset_free(&q);

    git_oidset seen_b = {0};
    git_oidset_add(&q, b);
    esp_err_t ret = ESP_OK;
    for (int i = 0; i < q.n; i++) {
        git_oid cur = q.v[i];
        if (git_oidset_has(&seen_b, &cur)) continue;
        git_oidset_add(&seen_b, &cur);
        if (git_oidset_has(&seen_a, &cur)) { *out = cur; break; }
        git_commit c;
        if (git_commit_load(&cur, &c) != ESP_OK) continue;
        for (int p = 0; p < c.nparents; p++) git_oidset_add(&q, &c.parents[p]);
        git_commit_free(&c);
    }
    git_oidset_free(&q);
    git_oidset_free(&seen_a);
    git_oidset_free(&seen_b);
    return ret;
}

esp_err_t git_commit_list_linear(const git_oid *base, const git_oid *head,
                                 git_oid **out, int *out_count)
{
    *out = NULL;
    *out_count = 0;

    git_oid stack[64];
    int n = 0;
    git_oid cur = *head;
    while (!git_oid_is_zero(&cur) && !(base && git_oid_eq(&cur, base))) {
        if (n >= (int)(sizeof(stack) / sizeof(stack[0]))) return ESP_FAIL;
        stack[n++] = cur;
        git_commit c;
        if (git_commit_load(&cur, &c) != ESP_OK) return ESP_FAIL;
        if (c.nparents == 0) { git_commit_free(&c); cur = (git_oid){0}; break; }
        cur = c.parents[0];   /* first-parent walk */
        git_commit_free(&c);
    }

    git_oid *arr = malloc((size_t)n * sizeof(*arr) + 1);
    if (!arr && n) return ESP_ERR_NO_MEM;
    for (int i = 0; i < n; i++) arr[i] = stack[n - 1 - i];   /* oldest first */
    *out = arr;
    *out_count = n;
    return ESP_OK;
}

static esp_err_t walk_tree_objects(const git_oid *tree_oid, const git_oidset *exclude,
                                   git_oidset *out)
{
    if (git_oid_is_zero(tree_oid)) return ESP_OK;
    if (exclude && git_oidset_has(exclude, tree_oid)) return ESP_OK;
    if (!git_oidset_add(out, tree_oid)) return ESP_ERR_NO_MEM;

    git_tree t;
    esp_err_t ret = git_tree_load(tree_oid, &t);
    if (ret != ESP_OK) return ret;
    for (int i = 0; i < t.count; i++) {
        const git_tree_entry *e = &t.entries[i];
        if ((e->mode & 040000) == 040000 && !(e->mode & 0100000)) {
            ret = walk_tree_objects(&e->oid, exclude, out);
            if (ret != ESP_OK) break;
        } else {
            if (exclude && git_oidset_has(exclude, &e->oid)) continue;
            if (!git_oidset_add(out, &e->oid)) { ret = ESP_ERR_NO_MEM; break; }
        }
    }
    git_tree_free(&t);
    return ret;
}

esp_err_t git_reachable_objects(const git_oid *heads, int nheads,
                                const git_oid *haves, int nhaves,
                                git_oidset *out)
{
    /* First gather every object reachable from the haves so we can skip
     * them.  For the shapes Draftling produces (one remote branch) this
     * stays small. */
    git_oidset have_objs = {0}, have_commits = {0}, q = {0};
    for (int i = 0; i < nhaves; i++) {
        git_oidset_add(&q, &haves[i]);
        git_oidset_add(&have_commits, &haves[i]);
        git_commit c;
        if (git_commit_load(&haves[i], &c) != ESP_OK) continue;
        /* Only the boundary commits' current trees are needed as the
         * "already on the server" exclusion set -- walking their whole
         * ancestry would touch every blob the repo ever had. Sending a
         * few objects the server already has is harmless. */
        walk_tree_objects(&c.tree, NULL, &have_objs);
        git_commit_free(&c);
    }
    git_oidset_free(&q);

    esp_err_t ret = ESP_OK;
    for (int i = 0; i < nheads; i++) git_oidset_add(&q, &heads[i]);
    git_oidset walked = {0};
    for (int i = 0; i < q.n && ret == ESP_OK; i++) {
        git_oid cur = q.v[i];
        if (git_oidset_has(&walked, &cur)) continue;
        git_oidset_add(&walked, &cur);
        if (git_oidset_has(&have_commits, &cur)) continue;

        git_commit c;
        if (git_commit_load(&cur, &c) != ESP_OK) { ret = ESP_FAIL; break; }
        if (!git_oidset_add(out, &cur)) ret = ESP_ERR_NO_MEM;
        if (ret == ESP_OK) ret = walk_tree_objects(&c.tree, &have_objs, out);
        for (int p = 0; p < c.nparents; p++) git_oidset_add(&q, &c.parents[p]);
        git_commit_free(&c);
    }

    git_oidset_free(&q);
    git_oidset_free(&walked);
    git_oidset_free(&have_objs);
    git_oidset_free(&have_commits);
    return ret;
}
