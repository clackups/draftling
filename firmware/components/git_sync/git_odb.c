/* git_odb.c -- repository layout, loose object store, refs, trees,
 * commits and history walking. */
#include "git_core.h"

#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

#include <esp_log.h>
#include <esp_heap_caps.h>

#include "sd_card.h"

static const char *TAG = "git.odb";

static char s_workdir[256];
static char s_gitdir[288];
static bool s_open;

const char *git_repo_gitdir(void)  { return s_gitdir; }

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
            mkdir(tmp, 0777);    /* the final mkdir() below reports failure */
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
    if (ret == ESP_OK && ferror(f)) ret = ESP_FAIL;
    fclose(f);
    return ret;
}

/* "<dir>/.<base>.dtmp", the same hidden temporary sd_card_write_file()
 * uses. */
static bool hidden_tmp_for(const char *path, char *out, size_t osz)
{
    const char *slash = strrchr(path, '/');
    int n = slash ? snprintf(out, osz, "%.*s/.%s.dtmp", (int)(slash - path), path, slash + 1)
                  : snprintf(out, osz, ".%s.dtmp", path);
    return n > 0 && (size_t)n < osz;
}

/* Refs and HEAD: written in full, synced to the card, then swapped in
 * by sd_card_replace_with(), so a power loss leaves the old or the new
 * ref (recovered at the next mount), never none -- a missing branch
 * ref would make the next sync start over as a first sync. */
static esp_err_t write_whole_file(const char *path, const void *data, size_t len)
{
    char tmp[400];
    if (!hidden_tmp_for(path, tmp, sizeof(tmp))) return ESP_FAIL;
    FILE *f = fopen(tmp, "wb");
    if (!f) return ESP_FAIL;
    bool ok = (len == 0) || (fwrite(data, 1, len, f) == len);
    if (fflush(f) != 0 || fsync(fileno(f)) != 0) ok = false;
    if (fclose(f) != 0) ok = false;
    if (!ok) { remove(tmp); return ESP_FAIL; }
    return sd_card_replace_with(tmp, path);
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
    static const char *const dirs[] = { "", "/objects", "/refs/heads",
                                        "/refs/remotes/origin", "/tmp" };
    for (size_t i = 0; i < sizeof(dirs) / sizeof(dirs[0]); i++) {
        snprintf(path, sizeof(path), "%s%s", s_gitdir, dirs[i]);
        if (mkdir_p(path) != 0) {
            ESP_LOGE(TAG, "cannot create %s", path);
            return ESP_FAIL;
        }
    }

    snprintf(path, sizeof(path), "%s/HEAD", s_gitdir);
    if (!file_exists(path) &&
        write_whole_file(path, "ref: refs/heads/main\n", 21) != ESP_OK) {
        ESP_LOGE(TAG, "cannot write %s", path);
        return ESP_FAIL;
    }

    s_open = true;
    return ESP_OK;
}

void git_repo_clean_tmp(void)
{
    char dir[320];
    snprintf(dir, sizeof(dir), "%s/tmp", s_gitdir);
    DIR *d = opendir(dir);
    if (!d) return;
    struct dirent *de;
    char p[600];
    while ((de = readdir(d)) != NULL) {
        if (de->d_name[0] == '.') continue;
        snprintf(p, sizeof(p), "%s/%s", dir, de->d_name);
        remove(p);
    }
    closedir(d);
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

esp_err_t git_odb_hash(git_obj_type type, const void *data, size_t len, git_oid *out)
{
    char hdr[32];
    int hlen = snprintf(hdr, sizeof(hdr), "%s %zu", git_obj_type_name(type), len);
    git_sha1_ctx *c = git_sha1_new();
    uint8_t raw[GIT_OID_RAWSZ];
    git_sha1_update(c, hdr, (size_t)hlen + 1);   /* include the NUL */
    git_sha1_update(c, data, len);
    if (!git_sha1_final(c, raw)) return ESP_FAIL;
    git_oid_from_raw(out, raw);
    return ESP_OK;
}

#define IO_CHUNK 4096

static long file_size_of(FILE *f)
{
    if (fseek(f, 0, SEEK_END) != 0) return -1;
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    return n;
}

esp_err_t git_odb_hash_file(git_obj_type type, const char *path, git_oid *out)
{
    FILE *f = fopen(path, "rb");
    if (!f) return ESP_ERR_NOT_FOUND;
    long size = file_size_of(f);
    uint8_t *buf = malloc(IO_CHUNK);
    git_sha1_ctx *c = git_sha1_new();
    esp_err_t ret = (size >= 0 && buf && c) ? ESP_OK : ESP_ERR_NO_MEM;
    if (ret == ESP_OK) {
        char hdr[32];
        int hlen = snprintf(hdr, sizeof(hdr), "%s %ld", git_obj_type_name(type), size);
        git_sha1_update(c, hdr, (size_t)hlen + 1);
        size_t r, total = 0;
        while ((r = fread(buf, 1, IO_CHUNK, f)) > 0) {
            git_sha1_update(c, buf, r);
            total += r;
        }
        if (total != (size_t)size) ret = ESP_FAIL;
    }
    uint8_t raw[GIT_OID_RAWSZ];
    if (!git_sha1_final(c, raw) && ret == ESP_OK) ret = ESP_FAIL;   /* frees c */
    if (ret == ESP_OK) git_oid_from_raw(out, raw);
    free(buf);
    fclose(f);
    return ret;
}

void git_odb_tmp_path(char *out, size_t osz, const char *name)
{
    snprintf(out, osz, "%s/tmp/%s", s_gitdir, name);
}

/* ---- object writer ---- */

static unsigned s_tmp_seq;

esp_err_t git_odb_writer_open(git_odb_writer *w, git_obj_type type, size_t size)
{
    memset(w, 0, sizeof(*w));
    char name[24];
    snprintf(name, sizeof(name), "obj%u", s_tmp_seq++);
    git_odb_tmp_path(w->tmp, sizeof(w->tmp), name);
    w->fp = fopen(w->tmp, "wb");
    if (!w->fp) return ESP_FAIL;
    w->zw = git_zwriter_new();
    w->sha = git_sha1_new();
    if (!w->zw || !w->sha || git_zwriter_start(w->zw, w->fp, NULL) != ESP_OK) {
        git_odb_writer_abort(w);
        return ESP_ERR_NO_MEM;
    }
    char hdr[32];
    int hlen = snprintf(hdr, sizeof(hdr), "%s %zu", git_obj_type_name(type), size);
    git_sha1_update(w->sha, hdr, (size_t)hlen + 1);
    if (git_zwriter_write(w->zw, hdr, (size_t)hlen + 1) != ESP_OK) {
        git_odb_writer_abort(w);
        return ESP_FAIL;
    }
    w->left = size;
    return ESP_OK;
}

esp_err_t git_odb_writer_write(git_odb_writer *w, const void *p, size_t n)
{
    if (n > w->left) return ESP_FAIL;
    w->left -= n;
    git_sha1_update(w->sha, p, n);
    return git_zwriter_write(w->zw, p, n);
}

void git_odb_writer_abort(git_odb_writer *w)
{
    if (w->fp) { fclose(w->fp); w->fp = NULL; remove(w->tmp); }
    if (w->zw) { git_zwriter_free(w->zw); w->zw = NULL; }
    if (w->sha) {
        uint8_t raw[GIT_OID_RAWSZ];
        git_sha1_final(w->sha, raw);
        w->sha = NULL;
    }
}

esp_err_t git_odb_writer_finish(git_odb_writer *w, git_oid *out_oid)
{
    esp_err_t ret = (w->left == 0) ? git_zwriter_finish(w->zw) : ESP_FAIL;
    git_zwriter_free(w->zw);
    w->zw = NULL;
    uint8_t raw[GIT_OID_RAWSZ];
    if (!git_sha1_final(w->sha, raw)) ret = ESP_FAIL;
    w->sha = NULL;
    if (fclose(w->fp) != 0) ret = ESP_FAIL;
    w->fp = NULL;
    if (ret != ESP_OK) { remove(w->tmp); return ret; }

    git_oid oid;
    git_oid_from_raw(&oid, raw);
    if (out_oid) *out_oid = oid;
    if (git_odb_exists(&oid)) { remove(w->tmp); return ESP_OK; }

    char dir[352], p[352];
    snprintf(dir, sizeof(dir), "%s/objects/%c%c", s_gitdir, oid.hex[0], oid.hex[1]);
    mkdir(dir, 0777);
    object_path(&oid, p, sizeof(p));
    if (rename(w->tmp, p) != 0) { remove(w->tmp); return ESP_FAIL; }
    return ESP_OK;
}

esp_err_t git_odb_write(git_obj_type type, const void *data, size_t len,
                        git_oid *out_oid)
{
    git_oid oid;
    esp_err_t hr = git_odb_hash(type, data, len, &oid);
    if (hr != ESP_OK) return hr;
    if (out_oid) *out_oid = oid;
    if (git_odb_exists(&oid)) return ESP_OK;

    git_odb_writer w;
    esp_err_t ret = git_odb_writer_open(&w, type, len);
    if (ret != ESP_OK) return ret;
    ret = git_odb_writer_write(&w, data, len);
    if (ret != ESP_OK) { git_odb_writer_abort(&w); return ret; }
    return git_odb_writer_finish(&w, NULL);
}

esp_err_t git_odb_write_file(git_obj_type type, const char *path, git_oid *out_oid)
{
    git_oid oid;
    esp_err_t ret = git_odb_hash_file(type, path, &oid);
    if (ret != ESP_OK) return ret;
    if (out_oid) *out_oid = oid;
    if (git_odb_exists(&oid)) return ESP_OK;

    FILE *f = fopen(path, "rb");
    if (!f) return ESP_ERR_NOT_FOUND;
    long size = file_size_of(f);
    uint8_t *buf = malloc(IO_CHUNK);
    git_odb_writer w;
    ret = (size >= 0 && buf) ? git_odb_writer_open(&w, type, (size_t)size) : ESP_ERR_NO_MEM;
    if (ret == ESP_OK) {
        size_t r;
        while (ret == ESP_OK && (r = fread(buf, 1, IO_CHUNK, f)) > 0)
            ret = git_odb_writer_write(&w, buf, r);
        if (ret == ESP_OK) ret = git_odb_writer_finish(&w, &oid);
        else git_odb_writer_abort(&w);
        /* The file changed between the passes: report what was stored. */
        if (ret == ESP_OK && out_oid) *out_oid = oid;
    }
    free(buf);
    fclose(f);
    return ret;
}

/* ---- object reader ---- */

esp_err_t git_odb_stream_open(git_odb_stream *s, const git_oid *oid)
{
    memset(s, 0, sizeof(*s));
    char p[352];
    object_path(oid, p, sizeof(p));
    s->fp = fopen(p, "rb");
    if (!s->fp) return ESP_ERR_NOT_FOUND;
    s->z = git_zreader_new();
    if (!s->z) { git_odb_stream_close(s); return ESP_ERR_NO_MEM; }
    git_zreader_start(s->z, s->fp);

    /* header: "<type> <size>\0" */
    char hdr[32];
    size_t n = 0;
    for (;;) {
        if (n >= sizeof(hdr) || git_zreader_read(s->z, hdr + n, 1) != 1) {
            git_odb_stream_close(s);
            return ESP_FAIL;
        }
        if (hdr[n] == '\0') break;
        n++;
    }
    char *sp = strchr(hdr, ' ');
    if (!sp) { git_odb_stream_close(s); return ESP_FAIL; }
    *sp = '\0';
    s->type = git_obj_type_from_name(hdr);
    char *end = NULL;
    unsigned long sz = strtoul(sp + 1, &end, 10);
    if (s->type == GIT_OBJ_BAD || !end || *end) { git_odb_stream_close(s); return ESP_FAIL; }
    s->size = s->left = (size_t)sz;
    return ESP_OK;
}

long git_odb_stream_read(git_odb_stream *s, void *buf, size_t n)
{
    if (n > s->left) n = s->left;
    if (n == 0) return 0;
    long r = git_zreader_read(s->z, buf, n);
    if (r != (long)n) return -1;           /* corrupt or truncated */
    s->left -= n;
    return r;
}

void git_odb_stream_close(git_odb_stream *s)
{
    if (s->z) { git_zreader_free(s->z); s->z = NULL; }
    if (s->fp) { fclose(s->fp); s->fp = NULL; }
}

static long odb_src_read(void *ctx, void *buf, size_t n)
{
    return git_odb_stream_read((git_odb_stream *)ctx, buf, n);
}

void git_reader_odb(git_reader *r, git_odb_stream *s)
{
    r->read = odb_src_read;
    r->ctx = s;
}

esp_err_t git_odb_read(const git_oid *oid, git_obj_type *type, git_buf *out)
{
    git_odb_stream s;
    esp_err_t ret = git_odb_stream_open(&s, oid);
    if (ret != ESP_OK) return ret;
    if (!git_buf_reserve(out, out->len + s.size + 1)) {
        git_odb_stream_close(&s);
        return ESP_ERR_NO_MEM;
    }
    long r = s.size ? git_odb_stream_read(&s, out->data + out->len, s.size) : 0;
    if (r == (long)s.size) {
        out->len += s.size;
        out->data[out->len] = 0;
        if (type) *type = s.type;
    } else {
        ret = ESP_FAIL;
    }
    git_odb_stream_close(&s);
    return ret;
}

esp_err_t git_odb_read_type(const git_oid *oid, git_obj_type *type, size_t *size)
{
    git_odb_stream s;
    esp_err_t ret = git_odb_stream_open(&s, oid);
    if (ret != ESP_OK) return ret;
    if (type) *type = s.type;
    if (size) *size = s.size;
    git_odb_stream_close(&s);
    return ESP_OK;
}

esp_err_t git_odb_read_to_file(const git_oid *oid, git_obj_type *type, const char *path)
{
    char tmp[400];
    if (!hidden_tmp_for(path, tmp, sizeof(tmp))) return ESP_FAIL;
    git_odb_stream s;
    esp_err_t ret = git_odb_stream_open(&s, oid);
    if (ret != ESP_OK) return ret;
    if (type) *type = s.type;
    FILE *f = fopen(tmp, "wb");
    uint8_t *buf = malloc(IO_CHUNK);
    if (!f || !buf) ret = ESP_FAIL;
    while (ret == ESP_OK && s.left) {
        long r = git_odb_stream_read(&s, buf, IO_CHUNK);
        if (r <= 0 || fwrite(buf, 1, (size_t)r, f) != (size_t)r) ret = ESP_FAIL;
    }
    git_odb_stream_close(&s);
    free(buf);
    if (f) {
        if (fflush(f) != 0 || fsync(fileno(f)) != 0) ret = ESP_FAIL;
        if (fclose(f) != 0) ret = ESP_FAIL;
    }
    if (ret != ESP_OK) { remove(tmp); return ret; }
    return sd_card_replace_with(tmp, path);
}

/* ------------------------------------------------------------------ */
/* refs                                                               */
/* ------------------------------------------------------------------ */

static void ref_path(const char *name, char *out, size_t outsz)
{
    snprintf(out, outsz, "%s/%s", s_gitdir, name);
}

static esp_err_t ref_read_depth(const char *name, git_oid *out, int depth)
{
    if (depth > 5) {                       /* "ref: X" loops */
        ESP_LOGE(TAG, "symbolic ref loop at %s", name);
        return ESP_FAIL;
    }
    char p[352];
    ref_path(name, p, sizeof(p));

    git_buf b = GIT_BUF_INIT;
    esp_err_t ret = read_whole_file(p, &b);
    /* ESP_ERR_NOT_FOUND = no such ref; anything else is a read error,
     * which must not look like "no commit yet". */
    if (ret != ESP_OK) { git_buf_free(&b); return ret; }

    /* trim */
    while (b.len && (b.data[b.len - 1] == '\n' || b.data[b.len - 1] == '\r' ||
                     b.data[b.len - 1] == ' '))
        b.data[--b.len] = '\0';

    if (b.len > 5 && memcmp(b.data, "ref: ", 5) == 0) {
        char target[128];
        strlcpy(target, (char *)b.data + 5, sizeof(target));
        git_buf_free(&b);
        return ref_read_depth(target, out, depth + 1);
    }

    if (b.len < GIT_OID_HEXSZ) { git_buf_free(&b); return ESP_FAIL; }
    git_oid oid;
    memcpy(oid.hex, b.data, GIT_OID_HEXSZ);
    oid.hex[GIT_OID_HEXSZ] = '\0';
    git_buf_free(&b);
    uint8_t raw[GIT_OID_RAWSZ];
    if (!git_oid_to_raw(raw, &oid)) {
        ESP_LOGE(TAG, "ref %s is not an object id", name);
        return ESP_FAIL;
    }
    *out = oid;
    return ESP_OK;
}

esp_err_t git_ref_read(const char *name, git_oid *out)
{
    return ref_read_depth(name, out, 0);
}

esp_err_t git_ref_write(const char *name, const git_oid *oid)
{
    char p[352];
    ref_path(name, p, sizeof(p));
    char line[GIT_OID_HEXSZ + 2];
    snprintf(line, sizeof(line), "%s\n", oid->hex);
    return write_whole_file(p, line, strlen(line));
}

esp_err_t git_head_set_symbolic(const char *target)
{
    char p[352];
    ref_path("HEAD", p, sizeof(p));
    char line[160];
    snprintf(line, sizeof(line), "ref: %s\n", target);
    return write_whole_file(p, line, strlen(line));
}

bool git_branch_name_ok(const char *s)
{
    size_t n = strlen(s);
    if (n == 0 || s[0] == '/' || s[0] == '.' || s[0] == '-' ||
        s[n - 1] == '/' || s[n - 1] == '.')
        return false;
    if (n >= 5 && strcmp(s + n - 5, ".lock") == 0) return false;
    if (strstr(s, "..") || strstr(s, "//") || strstr(s, "@{") || strstr(s, "/."))
        return false;
    for (const char *p = s; *p; p++) {
        unsigned char c = (unsigned char)*p;
        if (c <= 0x20 || c == 0x7f || strchr("~^:?*[\\", c)) return false;
    }
    return true;
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

/* Entry names git itself refuses (fsck): empty, ".", "..", ".git", or
 * containing '/'. Trees come from the server, and an entry name ends up
 * in a file path on the SD card. */
static bool tree_name_ok(const uint8_t *p, size_t n)
{
    if (n == 0) return false;
    if (memchr(p, '/', n)) return false;
    if (n == 1 && p[0] == '.') return false;
    if (n == 2 && p[0] == '.' && p[1] == '.') return false;
    if (n == 4 && strncasecmp((const char *)p, ".git", 4) == 0) return false;
    return true;
}

esp_err_t git_tree_parse(const uint8_t *data, size_t len, git_tree *out)
{
    memset(out, 0, sizeof(*out));
    esp_err_t ret = ESP_OK;
    size_t i = 0;
    while (i < len) {
        size_t sp = i;
        while (sp < len && data[sp] != ' ') sp++;
        if (sp >= len) { ret = ESP_FAIL; break; }
        size_t nul = sp + 1;
        while (nul < len && data[nul] != '\0') nul++;
        if (nul + 1 + GIT_OID_RAWSZ > len) { ret = ESP_FAIL; break; }

        size_t nlen = nul - (sp + 1);
        if (nlen >= sizeof(out->entries[0].name) ||
            !tree_name_ok(data + sp + 1, nlen)) {
            ESP_LOGE(TAG, "tree entry with an invalid name");
            ret = ESP_FAIL;
            break;
        }
        if (!tree_grow(out)) { ret = ESP_ERR_NO_MEM; break; }
        git_tree_entry *e = &out->entries[out->count++];
        memset(e, 0, sizeof(*e));
        e->mode = (uint32_t)strtoul((const char *)data + i, NULL, 8);
        memcpy(e->name, data + sp + 1, nlen);
        git_oid_from_raw(&e->oid, data + nul + 1);
        i = nul + 1 + GIT_OID_RAWSZ;
    }
    /* Callers only free the tree on success. */
    if (ret != ESP_OK) git_tree_free(out);
    return ret;
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
/* Breadth-first walk of all ancestors of start (start included) into
 * seen; stops early at the first commit already in stop (when given)
 * and returns it in *hit. Every commit must be present: a missing one
 * would make the walk, and the merge base, silently wrong. */
static esp_err_t walk_ancestors(const git_oid *start, git_oidset *seen,
                                const git_oidset *stop, git_oid *hit)
{
    git_oidset q = {0};
    esp_err_t ret = git_oidset_add(&q, start) ? ESP_OK : ESP_ERR_NO_MEM;
    for (int i = 0; ret == ESP_OK && i < q.n; i++) {
        git_oid cur = q.v[i];
        if (git_oidset_has(seen, &cur)) continue;
        if (!git_oidset_add(seen, &cur)) { ret = ESP_ERR_NO_MEM; break; }
        if (stop && git_oidset_has(stop, &cur)) { *hit = cur; break; }
        git_commit c;
        ret = git_commit_load(&cur, &c);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "merge base: commit %s missing", cur.hex);
            break;
        }
        for (int p = 0; p < c.nparents && ret == ESP_OK; p++)
            if (!git_oidset_add(&q, &c.parents[p])) ret = ESP_ERR_NO_MEM;
        git_commit_free(&c);
    }
    git_oidset_free(&q);
    return ret;
}

esp_err_t git_merge_base(const git_oid *a, const git_oid *b, git_oid *out)
{
    git_oid_clear(out);
    if (git_oid_is_zero(a) || git_oid_is_zero(b)) return ESP_OK;
    if (git_oid_eq(a, b)) { *out = *a; return ESP_OK; }

    git_oidset seen_a = {0}, seen_b = {0};
    esp_err_t ret = walk_ancestors(a, &seen_a, NULL, NULL);
    if (ret == ESP_OK) ret = walk_ancestors(b, &seen_b, &seen_a, out);
    if (ret != ESP_OK) git_oid_clear(out);
    git_oidset_free(&seen_a);
    git_oidset_free(&seen_b);
    return ret;
}

esp_err_t git_commit_list_linear(const git_oid *base, const git_oid *head, int max,
                                 git_oid **out, int *out_count)
{
    *out = NULL;
    *out_count = 0;

    /* First-parent walk from head back to base (or the root), newest
     * first; reversed below. max > 0 keeps only the newest max. */
    git_oid *v = NULL;
    int n = 0, cap = 0;
    esp_err_t ret = ESP_OK;
    git_oid cur = *head;
    while (!git_oid_is_zero(&cur) && !(base && git_oid_eq(&cur, base)) &&
           (max <= 0 || n < max)) {
        if (n == cap) {
            int nc = cap ? cap * 2 : 32;
            git_oid *nv = realloc(v, (size_t)nc * sizeof(*nv));
            if (!nv) { ret = ESP_ERR_NO_MEM; break; }
            v = nv;
            cap = nc;
        }
        v[n++] = cur;
        git_commit c;
        ret = git_commit_load(&cur, &c);
        if (ret != ESP_OK) break;
        if (c.nparents == 0) git_oid_clear(&cur);
        else cur = c.parents[0];
        git_commit_free(&c);
    }
    if (ret != ESP_OK) { free(v); return ret; }
    /* A base that the first-parent walk never met (it was reached
     * through a merge's second parent): the range is not a straight
     * line, and replaying everything back to the root instead would
     * re-apply commits the other side already has. */
    if (base && !git_oid_is_zero(base) && git_oid_is_zero(&cur) &&
        (max <= 0 || n < max)) {
        free(v);
        return ESP_ERR_NOT_FOUND;
    }

    for (int i = 0; i < n / 2; i++) {               /* oldest first */
        git_oid t = v[i];
        v[i] = v[n - 1 - i];
        v[n - 1 - i] = t;
    }
    *out = v;
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
