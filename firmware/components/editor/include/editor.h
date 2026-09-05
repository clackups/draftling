#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>
#include <esp_err.h>
#include <sdkconfig.h>

/* Maximum document size held in the in-memory gap buffer, in bytes.
 *
 * Sized dynamically at startup from the free PSRAM available when
 * editor_init() runs (after the display, LVGL widget heap, and
 * editor UI have taken their share). Two buffers of this size are
 * allocated from PSRAM -- the gap buffer and a flattened text cache
 * used by the UI refresh path -- so the editor's total SPIRAM cost
 * is roughly 2x this value.
 *
 * Returns 0 before editor_init() has run. */
size_t editor_get_max_doc_size(void);

typedef enum {
    EDITOR_MODE_NORMAL,
    EDITOR_MODE_EDITING,
} editor_mode_t;

/* ---- Multi-document support ----
 *
 * The text engine keeps a small fixed pool of documents. Each document
 * (an editor_doc_t) owns its own gap buffer, flattened text cache, file
 * path, modified flag, cursor, scroll line, and selection. The entire
 * existing public API below (editor_open_file, editor_move_*,
 * editor_get_text, ...) operates on the single "active" document, so the
 * current single-pane UI keeps working unchanged.
 *
 * To present two documents side by side, a UI acquires documents from
 * the pool and switches the active one before issuing engine calls:
 *
 *   editor_doc_t *d = editor_doc_acquire("/sdcard/notes.md");
 *   editor_set_active(d);
 *   ... editor_move_down(); editor_get_text(...); ...
 *   editor_doc_release(d);
 *
 * The clipboard is shared across all documents (a single system
 * clipboard). Reference counting lets two panes that open the same path
 * share one editor_doc_t (and therefore one underlying buffer): the
 * second editor_doc_acquire() of an already-open path returns the same
 * handle with an incremented reference count. */
typedef struct editor_doc_s editor_doc_t;

/* Acquire a document for the given file path.
 *
 *  - path == NULL: returns a fresh empty (untitled) document.
 *  - path already open in the pool: returns the existing handle with its
 *    reference count incremented (shared buffer).
 *  - otherwise: allocates a pool slot, opens the file into it, and
 *    returns it with reference count 1.
 *
 * The acquired document becomes the active document. Returns NULL if the
 * pool is exhausted or the file could not be opened. */
editor_doc_t *editor_doc_acquire(const char *path);

/* Release a previously acquired document. Decrements its reference
 * count; when it reaches zero the document's cursor/scroll metadata is
 * persisted and the pool slot is freed for reuse. Safe to call with
 * NULL. If the released document was active, another in-use document (or
 * NULL) becomes active. */
void editor_doc_release(editor_doc_t *doc);

/* Make doc the active document. All subsequent calls to the public API
 * below operate on it. Safe to call with NULL (clears the active doc). */
void editor_set_active(editor_doc_t *doc);

/* Return the currently active document, or NULL. */
editor_doc_t *editor_get_active(void);

/* Return the file path bound to a document (NULL for an untitled one). */
const char *editor_doc_get_path(const editor_doc_t *doc);

/* Return whether a document has unsaved changes. */
bool editor_doc_is_modified(const editor_doc_t *doc);

/* Save the file backing doc (if it has a path) without disturbing the
 * active document. Returns ESP_OK on success, ESP_ERR_INVALID_STATE if
 * the document is untitled, or NULL is passed. Used by auto-save /
 * standby / git-sync hooks that must persist every open document. */
esp_err_t editor_doc_save(editor_doc_t *doc);

/* Persist cursor/scroll sidecar metadata for doc without disturbing the
 * active document. No-op for an untitled document or NULL. */
void editor_doc_save_meta(editor_doc_t *doc);

/* Invoke cb once for each in-use document in the pool. Lets multi-doc
 * aware callers (auto-save, git-sync "save before sync") iterate every
 * open document. */
void editor_doc_foreach(void (*cb)(editor_doc_t *doc, void *ctx), void *ctx);

void editor_init(void);
esp_err_t editor_open_file(const char *path);
esp_err_t editor_save_file(void);
esp_err_t editor_save_file_as(const char *path);
void editor_new_file(void);
void editor_close_file(void);
/* Persist per-file metadata (cursor, scroll line, ...) to the
 * sidecar next to the currently-open file. No-op if no file is
 * open. Safe to call repeatedly. */
void editor_save_meta(void);
const char *editor_get_file_path(void);
bool editor_is_modified(void);
const char *editor_get_text(size_t *out_len);
size_t editor_get_cursor(void);
void editor_get_cursor_pos(int *line, int *col);
int editor_get_line_count(void);
const char *editor_get_line(int line_num, size_t *out_len);
int editor_get_scroll_line(void);
void editor_set_scroll_line(int line);

/* Raw selection-anchor accessors (logical byte offset; < 0 = none).
 * Used by the split-screen UI to save / restore a per-pane selection. */
int editor_get_sel_anchor(void);
void editor_set_sel_anchor(int anchor);

void editor_move_left(void);
void editor_move_right(void);
void editor_move_up(void);
void editor_move_down(void);
void editor_move_home(void);
void editor_move_end(void);
void editor_move_page_up(int visible_lines);
void editor_move_page_down(int visible_lines);
void editor_move_doc_start(void);
void editor_move_doc_end(void);
void editor_move_word_left(void);
void editor_move_word_right(void);

/* Editing primitives. Return true on success, false if the operation
 * was rejected because the document would exceed the editor buffer. */
bool editor_insert_char(char c);
bool editor_insert_text(const char *text, size_t len);
bool editor_insert_newline(void);
void editor_delete_back(void);
void editor_delete_forward(void);
void editor_delete_line(void);

void editor_set_cursor(size_t pos);

/* Find / Replace */
int editor_find(const char *needle, size_t from_pos);
esp_err_t editor_replace_range(size_t start, size_t end, const char *replacement);

/* Selection */
bool editor_selection_active(void);
void editor_set_selection_anchor(void);
void editor_clear_selection(void);
void editor_get_selection_range(size_t *start, size_t *end);
bool editor_delete_selection(void);
void editor_select_all(void);

/* Clipboard */
bool editor_copy(void);
bool editor_cut(void);
bool editor_paste(void);
bool editor_has_clipboard(void);

editor_mode_t editor_get_mode(void);
void editor_set_mode(editor_mode_t mode);

#ifdef __cplusplus
}
#endif
