/*
 * Web client: the page's Files panel (web/client.js) -- download and
 * upload of the documents kept in the browser's storage.
 *
 * The documents live in the origin-private file system, which only
 * the program's own threads reach (through WasmFS), so the page sends
 * its requests here: web_files_request() queues one (WEB_EV_FILES),
 * and the LVGL loop runs it with the LVGL lock held -- never at the
 * same time as the editor -- and answers through Module.webFilesReply
 * (id, ok, text, bytes).
 *
 * - LIST: first saves every open document with changes (as before
 *   sleep), so what is downloaded is what is on the screen; then the
 *   projects and their documents as JSON.
 * - READ: one document's bytes.
 * - ZIP: the documents of one project, or of all of them, as a ZIP
 *   archive (miniz). Only documents: not the Git history and not
 *   git.cfg, which holds the access token.
 * - WRITE: store an uploaded document in a project (replacing a file
 *   of that name), then reload it where the editor has it open; refused
 *   while it is open with unsaved changes.
 * - MKPROJECT: create a project for uploads.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <emscripten.h>

#include "editor.h"
#include "editor_ui.h"
#include "esp_log.h"
#include "linux_host.h"
#include "miniz.h"
#include "projects.h"
#include "sd_card.h"
#include "web_host.h"

static const char *TAG = "Files";

enum { OP_LIST = 1, OP_READ = 2, OP_ZIP = 3, OP_WRITE = 4, OP_MKPROJECT = 5 };

typedef struct {
    int op, id;
    char *a, *b;        /* project / file name, malloc()ed by the page */
    uint8_t *data;      /* WRITE: the file's bytes */
    int len;
} files_req_t;

/* The page's thread: queue a request (the strings and data become the
 * request's, freed after it ran). */
EMSCRIPTEN_KEEPALIVE void web_files_request(int op, int id, char *a, char *b,
                                            uint8_t *data, int len)
{
    files_req_t *r = calloc(1, sizeof(*r));
    if (!r) {
        free(a);
        free(b);
        free(data);
        return;
    }
    *r = (files_req_t){ op, id, a, b, data, len };
    web_input(WEB_EV_FILES, (int)(intptr_t)r, 0, 0);
}

/* ---- replies ------------------------------------------------------------- */

static void reply(int id, bool ok, const char *text, const void *bytes, size_t n)
{
    MAIN_THREAD_EM_ASM({
        if (!Module.webFilesReply) return;
        Module.webFilesReply($0, !!$1, $2 ? UTF8ToString($2) : null,
                             $3 ? HEAPU8.slice($3, $3 + $4) : null);
    }, id, ok ? 1 : 0, text, bytes, n);
}

static void reply_error(int id, const char *msg)
{
    ESP_LOGW(TAG, "%s", msg);
    reply(id, false, msg, NULL, 0);
}

/* A growable string for JSON. */
typedef struct {
    char *s;
    size_t len, cap;
    bool oom;
} sbuf_t;

static void sb_add(sbuf_t *b, const char *p, size_t n)
{
    if (b->oom) return;
    if (b->len + n + 1 > b->cap) {
        size_t cap = b->cap ? b->cap * 2 : 1024;
        while (cap < b->len + n + 1) cap *= 2;
        char *ns = realloc(b->s, cap);
        if (!ns) {
            b->oom = true;
            return;
        }
        b->s = ns;
        b->cap = cap;
    }
    memcpy(b->s + b->len, p, n);
    b->len += n;
    b->s[b->len] = '\0';
}

static void sb_str(sbuf_t *b, const char *p)
{
    sb_add(b, p, strlen(p));
}

static void sb_json(sbuf_t *b, const char *p)
{
    sb_add(b, "\"", 1);
    for (; *p; p++) {
        unsigned char c = (unsigned char)*p;
        if (c == '"' || c == '\\') {
            char e[2] = { '\\', (char)c };
            sb_add(b, e, 2);
        } else if (c < 0x20) {
            char e[8];
            snprintf(e, sizeof(e), "\\u%04x", c);
            sb_str(b, e);
        } else {
            sb_add(b, (const char *)&c, 1);
        }
    }
    sb_add(b, "\"", 1);
}

/* ---- names --------------------------------------------------------------- */

static bool is_doc(const char *name)
{
    editor_doc_format_t f;
    return name[0] != '.' && editor_path_format(name, &f);
}

/* An uploaded file's name: a document name the editor lists, without
 * folders or characters a FAT card would refuse. */
static const char *file_name_problem(const char *name)
{
    size_t n = strlen(name);
    if (n == 0) return "The file has no name.";
    if (n > 100) return "The file name is too long (at most 100 bytes).";
    if (name[0] == '.' || name[0] == ' ' || name[n - 1] == ' ' || name[n - 1] == '.')
        return "A file name may not start with a dot or a space, or end with one.";
    for (const unsigned char *p = (const unsigned char *)name; *p; p++) {
        if (*p < 0x20 || strchr("/\\:*?\"<>|", *p))
            return "The file name has a character a memory card does not allow.";
    }
    if (!is_doc(name)) return "Only .md, .fountain and .txt files can be uploaded.";
    return NULL;
}

/* ---- open documents --------------------------------------------------------- */

typedef struct {
    const char *path;
    bool open, modified;
} doc_probe_t;

static void probe_cb(editor_doc_t *doc, void *ctx)
{
    doc_probe_t *p = (doc_probe_t *)ctx;
    const char *dp = editor_doc_get_path(doc);
    if (!dp || strcmp(dp, p->path) != 0) return;
    p->open = true;
    if (editor_doc_is_modified(doc)) p->modified = true;
}

static doc_probe_t probe(const char *path)
{
    doc_probe_t p = { path, false, false };
    editor_doc_foreach(probe_cb, &p);
    return p;
}

/* ---- operations --------------------------------------------------------------- */

static void op_list(int id)
{
    /* What is downloaded is what the editor shows. */
    if (linux_pre_sleep_cb) linux_pre_sleep_cb();

    sbuf_t b = { 0 };
    sb_str(&b, "{\"projects\":[");
    int count = 0;
    project_name_t *names = project_list(&count);
    for (int i = 0; i < count; i++) {
        char dir[300];
        project_dir(names[i], dir, sizeof(dir));
        if (i) sb_str(&b, ",");
        sb_str(&b, "{\"name\":");
        sb_json(&b, names[i]);
        sb_str(&b, ",\"files\":[");
        sd_card_file_entry_t *e = NULL;
        int n = 0, shown = 0;
        if (sd_card_list_dir(dir, &e, &n) == ESP_OK) {
            for (int k = 0; k < n; k++) {
                if (e[k].is_dir || !is_doc(e[k].name)) continue;
                char path[600];
                snprintf(path, sizeof(path), "%s/%s", dir, e[k].name);
                doc_probe_t p = probe(path);
                char num[64];
                if (shown++) sb_str(&b, ",");
                sb_str(&b, "{\"name\":");
                sb_json(&b, e[k].name);
                snprintf(num, sizeof(num), ",\"size\":%zu,\"open\":%s,\"modified\":%s}",
                         e[k].size, p.open ? "true" : "false", p.modified ? "true" : "false");
                sb_str(&b, num);
            }
            sd_card_free_list(e);
        }
        sb_str(&b, "]}");
    }
    free(names);
    char tail[96];
    snprintf(tail, sizeof(tail), "],\"maxSize\":%zu}", editor_get_max_doc_size());
    sb_str(&b, tail);
    if (b.oom) reply_error(id, "Out of memory.");
    else reply(id, true, b.s, NULL, 0);
    free(b.s);
}

/* Read a whole file into a malloc()ed buffer. */
static uint8_t *read_all(const char *path, size_t *n)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    uint8_t *buf = NULL;
    size_t len = 0, cap = 0;
    for (;;) {
        if (len == cap) {
            cap = cap ? cap * 2 : 65536;
            uint8_t *nb = realloc(buf, cap);
            if (!nb) {
                free(buf);
                fclose(f);
                return NULL;
            }
            buf = nb;
        }
        size_t r = fread(buf + len, 1, cap - len, f);
        if (r == 0) break;
        len += r;
    }
    bool err = ferror(f);
    fclose(f);
    if (err) {
        free(buf);
        return NULL;
    }
    *n = len;
    return buf ? buf : malloc(1);
}

static bool doc_path(int id, const char *project, const char *name, char *out, size_t sz)
{
    const char *why = project ? project_name_problem(project) : "No project.";
    if (!why && !project_exists(project)) why = "There is no such project.";
    if (!why) why = name ? file_name_problem(name) : "No file name.";
    if (why) {
        reply_error(id, why);
        return false;
    }
    char dir[300];
    project_dir(project, dir, sizeof(dir));
    snprintf(out, sz, "%s/%s", dir, name);
    return true;
}

static void op_read(int id, const char *project, const char *name)
{
    char path[600];
    if (!doc_path(id, project, name, path, sizeof(path))) return;
    size_t n = 0;
    uint8_t *data = read_all(path, &n);
    if (!data) {
        reply_error(id, "The file cannot be read.");
        return;
    }
    reply(id, true, NULL, data, n);
    free(data);
}

/* Add the documents of one project to the archive, under
 * "<project>/". */
static bool zip_project(mz_zip_archive *z, const char *project, int *files)
{
    char dir[300];
    project_dir(project, dir, sizeof(dir));
    sd_card_file_entry_t *e = NULL;
    int n = 0;
    if (sd_card_list_dir(dir, &e, &n) != ESP_OK) return true;
    bool ok = true;
    for (int k = 0; k < n && ok; k++) {
        if (e[k].is_dir || !is_doc(e[k].name)) continue;
        char path[600], arc[400];
        snprintf(path, sizeof(path), "%s/%s", dir, e[k].name);
        snprintf(arc, sizeof(arc), "%s/%s", project, e[k].name);
        size_t len = 0;
        uint8_t *data = read_all(path, &len);
        if (!data) continue;
        ok = mz_zip_writer_add_mem_ex(z, arc, data, len, NULL, 0, 6, 0, 0);
        free(data);
        (*files)++;
    }
    sd_card_free_list(e);
    return ok;
}

static void op_zip(int id, const char *project)
{
    if (project && project[0]) {
        const char *why = project_name_problem(project);
        if (!why && !project_exists(project)) why = "There is no such project.";
        if (why) {
            reply_error(id, why);
            return;
        }
    }
    mz_zip_archive z;
    memset(&z, 0, sizeof(z));
    if (!mz_zip_writer_init_heap(&z, 0, 65536)) {
        reply_error(id, "Out of memory.");
        return;
    }
    bool ok = true;
    int files = 0;
    if (project && project[0]) {
        ok = zip_project(&z, project, &files);
    } else {
        int count = 0;
        project_name_t *names = project_list(&count);
        for (int i = 0; i < count && ok; i++) ok = zip_project(&z, names[i], &files);
        free(names);
    }
    void *buf = NULL;
    size_t size = 0;
    if (ok) ok = mz_zip_writer_finalize_heap_archive(&z, &buf, &size);
    if (ok && files == 0) {
        reply_error(id, "There are no documents to download.");
    } else if (ok) {
        reply(id, true, NULL, buf, size);
    } else {
        reply_error(id, "The archive could not be made (out of memory?).");
    }
    mz_zip_writer_end(&z);
}

static void op_write(int id, const char *project, const char *name, const uint8_t *data, int len)
{
    char path[600];
    if (!doc_path(id, project, name, path, sizeof(path))) return;
    if ((size_t)len > editor_get_max_doc_size()) {
        reply_error(id, "The file is larger than Draftling can open.");
        return;
    }
    doc_probe_t p = probe(path);
    if (p.modified) {
        reply_error(id, "The file is open in Draftling with unsaved changes.");
        return;
    }
    char tmp[640];
    snprintf(tmp, sizeof(tmp), "%s.upload", path);
    FILE *f = fopen(tmp, "wb");
    bool ok = f != NULL;
    if (ok && len > 0) ok = fwrite(data, 1, (size_t)len, f) == (size_t)len;
    if (f && fclose(f) != 0) ok = false;
    if (ok) {
        ok = rename(tmp, path) == 0;
        if (!ok) {
            /* Should the file system refuse to replace a file by
             * renaming onto it: remove it first. */
            unlink(path);
            ok = rename(tmp, path) == 0;
        }
    }
    if (!ok) {
        unlink(tmp);
        reply_error(id, "The file could not be stored.");
        return;
    }
    ESP_LOGI(TAG, "Uploaded %s (%d bytes)", path, len);
    editor_ui_files_changed(p.open ? path : NULL);
    reply(id, true, NULL, NULL, 0);
}

static void op_mkproject(int id, const char *name)
{
    const char *why = name ? project_name_problem(name) : "No name.";
    if (!why && project_exists(name)) why = "A project of that name exists already.";
    if (why) {
        reply_error(id, why);
        return;
    }
    if (project_create(name) != ESP_OK) {
        reply_error(id, "The project could not be created.");
        return;
    }
    editor_ui_files_changed(NULL);
    reply(id, true, NULL, NULL, 0);
}

/* The LVGL loop (io_web.c), with the LVGL lock held. */
void web_files_run(void *req)
{
    files_req_t *r = (files_req_t *)req;
    switch (r->op) {
    case OP_LIST:      op_list(r->id); break;
    case OP_READ:      op_read(r->id, r->a, r->b); break;
    case OP_ZIP:       op_zip(r->id, r->a); break;
    case OP_WRITE:     op_write(r->id, r->a, r->b, r->data, r->len); break;
    case OP_MKPROJECT: op_mkproject(r->id, r->a); break;
    default:           reply_error(r->id, "Unknown request."); break;
    }
    free(r->a);
    free(r->b);
    free(r->data);
    free(r);
}
