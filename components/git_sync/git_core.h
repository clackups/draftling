/* git_core.h -- internal API for Draftling's native Git client.
 *
 * This is NOT a general-purpose libgit2 replacement.  It implements just
 * enough of Git to keep a local commit history on the SD card and to
 * exchange objects with a remote over the "smart" Git HTTP protocol
 * (gitprotocol-http):
 *
 *   - a loose-object database under <workdir>/.git/objects
 *   - refs (HEAD, refs/heads/<branch>, refs/remotes/origin/<branch>)
 *   - commit / tree / blob object parsing and creation
 *   - pkt-line framing, git-upload-pack (fetch) and git-receive-pack (push)
 *   - packfile v2 reading (incl. OFS_DELTA / REF_DELTA) and writing
 *   - merge-base, three-way tree merge and diff3 line merge with
 *     conflict markers
 *
 * Scope limits (documented in AGENTS.md / README.md):
 *   - a single branch, no tags, no submodules, no signed objects
 *   - the working tree is the flat set of "*.md" files in <workdir>;
 *     an optional remote sub-directory prefix ("path=") maps that flat
 *     set onto a sub-tree of the repository
 *   - full history is fetched on the first sync (no shallow clone)
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- object ids -------------------------------------------------------- */

#define GIT_OID_RAWSZ 20
#define GIT_OID_HEXSZ 40

typedef struct {
    char hex[GIT_OID_HEXSZ + 1];
} git_oid;

bool git_oid_is_zero(const git_oid *o);
void git_oid_clear(git_oid *o);
bool git_oid_eq(const git_oid *a, const git_oid *b);
void git_oid_from_raw(git_oid *o, const uint8_t raw[GIT_OID_RAWSZ]);
bool git_oid_to_raw(uint8_t raw[GIT_OID_RAWSZ], const git_oid *o);

/* ---- growable byte buffer (SPIRAM-backed) ---------------------------- */

typedef struct {
    uint8_t *data;
    size_t   len;
    size_t   cap;
} git_buf;

#define GIT_BUF_INIT {NULL, 0, 0}

void  git_buf_free(git_buf *b);
bool  git_buf_reserve(git_buf *b, size_t need);
bool  git_buf_put(git_buf *b, const void *p, size_t n);
bool  git_buf_putc(git_buf *b, uint8_t c);
bool  git_buf_puts(git_buf *b, const char *s);
bool  git_buf_printf(git_buf *b, const char *fmt, ...);

/* ---- SHA-1 ----------------------------------------------------------- */

typedef struct git_sha1_ctx git_sha1_ctx;
git_sha1_ctx *git_sha1_new(void);
void git_sha1_update(git_sha1_ctx *c, const void *data, size_t n);
void git_sha1_final(git_sha1_ctx *c, uint8_t out[GIT_OID_RAWSZ]); /* frees c */
void git_sha1_hex(const void *data, size_t n, git_oid *out);

/* ---- zlib (miniz in ROM) ------------------------------------------- */

/* Inflate a complete zlib stream whose decompressed length is known.
 * Returns bytes consumed from `in` via *in_consumed (so the caller can
 * advance to the next packed object). `out` must be `out_expected`
 * bytes. */
esp_err_t git_zlib_inflate(const uint8_t *in, size_t in_avail,
                           uint8_t *out, size_t out_expected,
                           size_t *in_consumed);

/* Inflate an object of unknown length, growing `out`. Used for loose
 * objects. */
esp_err_t git_zlib_inflate_buf(const uint8_t *in, size_t in_avail, git_buf *out);

/* Deflate `in` into `out` (zlib wrapped). Appends to `out`. */
esp_err_t git_zlib_deflate(const uint8_t *in, size_t in_len, git_buf *out);

/* ---- repository / odb --------------------------------------------- */

typedef enum {
    GIT_OBJ_BAD    = -1,
    GIT_OBJ_COMMIT = 1,
    GIT_OBJ_TREE   = 2,
    GIT_OBJ_BLOB   = 3,
    GIT_OBJ_TAG    = 4,
    GIT_OBJ_OFS_DELTA = 6,
    GIT_OBJ_REF_DELTA = 7,
} git_obj_type;

const char *git_obj_type_name(git_obj_type t);
git_obj_type git_obj_type_from_name(const char *s);

/* Open (creating .git if needed). `workdir` is the SD directory that
 * holds the "*.md" working tree (e.g. "/sdcard"). */
esp_err_t git_repo_open(const char *workdir);
void      git_repo_close(void);
const char *git_repo_gitdir(void);   /* "<workdir>/.git" */
const char *git_repo_workdir(void);

/* Loose object store. Content is the raw object payload (no header). */
bool      git_odb_exists(const git_oid *oid);
esp_err_t git_odb_read(const git_oid *oid, git_obj_type *type, git_buf *out);
esp_err_t git_odb_read_type(const git_oid *oid, git_obj_type *type, size_t *size);
esp_err_t git_odb_write(git_obj_type type, const void *data, size_t len,
                        git_oid *out_oid);
/* Hash without writing (used to test the working tree against HEAD). */
void      git_odb_hash(git_obj_type type, const void *data, size_t len,
                       git_oid *out_oid);

/* refs. `name` is e.g. "HEAD", "refs/heads/main". Returns ESP_ERR_NOT_FOUND
 * if the ref does not exist. HEAD is resolved through one symref level. */
esp_err_t git_ref_read(const char *name, git_oid *out);
esp_err_t git_ref_write(const char *name, const git_oid *oid);
esp_err_t git_ref_delete(const char *name);
/* Write "ref: <target>\n" into HEAD. */
esp_err_t git_head_set_symbolic(const char *target);

/* ---- tree ---------------------------------------------------------- */

typedef struct {
    uint32_t mode;              /* 0100644, 0100755, 040000, ... */
    char     name[256];
    git_oid  oid;
} git_tree_entry;

typedef struct {
    git_tree_entry *entries;
    int             count;
    int             cap;
} git_tree;

void      git_tree_free(git_tree *t);
esp_err_t git_tree_parse(const uint8_t *data, size_t len, git_tree *out);
esp_err_t git_tree_load(const git_oid *oid, git_tree *out);
/* Set (or replace, or with oid==NULL remove) an entry. Keeps sort order. */
bool      git_tree_set(git_tree *t, uint32_t mode, const char *name,
                       const git_oid *oid);
const git_tree_entry *git_tree_get(const git_tree *t, const char *name);
/* Serialise + store, returning the new tree oid. */
esp_err_t git_tree_write(const git_tree *t, git_oid *out_oid);

/* Resolve a "/"-separated path to a sub-tree oid (dir must exist or
 * ESP_ERR_NOT_FOUND). prefix=="" yields `root`. */
esp_err_t git_tree_subtree_oid(const git_oid *root, const char *prefix,
                               git_oid *out_oid);
/* Return a new root tree with `subtree_oid` spliced in at `prefix`
 * (creating intermediate trees). If subtree is empty, the prefix dir is
 * removed. `root` may be zero (empty base). */
esp_err_t git_tree_splice_subtree(const git_oid *root, const char *prefix,
                                  const git_oid *subtree_oid, git_oid *out_root);

/* ---- commit ------------------------------------------------------- */

#define GIT_MAX_PARENTS 4

typedef struct {
    git_oid tree;
    git_oid parents[GIT_MAX_PARENTS];
    int     nparents;
    char    author[192];        /* full "Name <email> ts tz" ident line */
    char    committer[192];
    char   *message;            /* malloc'd, may be NULL */
} git_commit;

void      git_commit_free(git_commit *c);
esp_err_t git_commit_parse(const uint8_t *data, size_t len, git_commit *out);
esp_err_t git_commit_load(const git_oid *oid, git_commit *out);
esp_err_t git_commit_create(const git_oid *tree,
                            const git_oid *parents, int nparents,
                            const char *author_ident,
                            const char *message, git_oid *out_oid);

/* ---- history walk ------------------------------------------------- */

/* merge-base of two commits via first-parent-agnostic BFS.  Sets
 * out to zero and returns ESP_OK when the histories are unrelated. */
esp_err_t git_merge_base(const git_oid *a, const git_oid *b, git_oid *out);

/* true if `anc` is an ancestor of (or equal to) `desc`. */
bool git_is_ancestor(const git_oid *anc, const git_oid *desc);

/* Collect the linear list of commits in (base, head], oldest first,
 * following first parents.  Fails if the range is not linear. */
esp_err_t git_commit_list_linear(const git_oid *base, const git_oid *head,
                                 git_oid **out, int *out_count);

/* Collect every object oid reachable from `heads` but not from `haves`
 * (commits, trees, blobs).  Used to build a push packfile. */
typedef struct { git_oid *v; int n, cap; } git_oidset;
void git_oidset_free(git_oidset *s);
bool git_oidset_add(git_oidset *s, const git_oid *o);
bool git_oidset_has(const git_oidset *s, const git_oid *o);
esp_err_t git_reachable_objects(const git_oid *heads, int nheads,
                                const git_oid *haves, int nhaves,
                                git_oidset *out);

/* ---- packfiles --------------------------------------------------- */

/* Read a packfile from an open FILE* (positioned at the 'P' of "PACK"),
 * writing every contained object to the odb as a loose object. */
esp_err_t git_pack_unpack_file(FILE *fp);

/* Write a packfile containing exactly `set` to `path`. */
esp_err_t git_pack_write_file(const char *path, const git_oidset *set);

/* ---- transport (smart HTTP) ------------------------------------- */

typedef struct {
    char base_url[320];         /* ".../repo.git" (no trailing slash) */
    char auth_basic[420];       /* pre-computed "Basic xxx" or "" */
} git_remote;

typedef struct {
    char    name[128];
    git_oid oid;
} git_ref_adv;

/* GET /info/refs?service=<service>. Fills up to `max` advertised refs. */
esp_err_t git_http_list_refs(const git_remote *r, const char *service,
                             git_ref_adv *refs, int max, int *out_count);

/* POST /git-upload-pack: negotiate `want`/`have` and stream the returned
 * packfile to `out_pack_path`.  `haves` may be NULL/0. Returns
 * ESP_ERR_NOT_FOUND-ish semantics via ESP_OK + no file only when the
 * server sends an empty pack. */
esp_err_t git_http_fetch_pack(const git_remote *r,
                              const git_oid *wants, int nwants,
                              const git_oid *haves, int nhaves,
                              const char *out_pack_path);

/* POST /git-receive-pack: update `refname` from `old` to `new`, sending
 * the packfile at `pack_path`. `report` receives the server's
 * report-status text. */
esp_err_t git_http_push(const git_remote *r, const char *refname,
                        const git_oid *old_oid, const git_oid *new_oid,
                        const char *pack_path, git_buf *report);

/* ---- three-way merge ------------------------------------------- */

typedef struct {
    /* per-path result */
    char    name[256];
    git_oid oid;                /* resulting blob (0 => deleted) */
    bool    conflict;
} git_merge_path_result;

/* Merge two flat "name -> blob oid" trees against a common `base` tree.
 * Text files get a diff3 line merge; conflicts are written into the
 * resulting blob with <<<<<<< / ======= / >>>>>>> markers and
 * `conflict` is set.  Returns a malloc'd array via *out / *out_count. */
esp_err_t git_merge_trees_flat(const git_oid *base,
                               const git_oid *ours,
                               const git_oid *theirs,
                               const char *ours_label,
                               const char *theirs_label,
                               git_merge_path_result **out, int *out_count,
                               int *out_nconflicts);

#ifdef __cplusplus
}
#endif
