/* epub.h -- reading EPUB books: the ZIP container, the package (OPF)
 * and table of contents, and each chapter's XHTML converted to the
 * Markdown the editor displays. Plain C++ over stdio; no ESP-IDF
 * dependency beyond heap_caps_* and the ROM's miniz inflater. */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct epub_book epub_book_t;

/* Largest XHTML file (uncompressed) a chapter may be. */
#define EPUB_MAX_ENTRY (8u * 1024u * 1024u)

/* Open the book at path and read its package and table of contents.
 * Returns NULL on failure; *why (may be NULL) then receives a short
 * reason for the user. */
epub_book_t *epub_open(const char *path, const char **why);
void         epub_close(epub_book_t *book);
/* The file was renamed: read it from its new path. */
void         epub_set_path(epub_book_t *book, const char *path);

/* Title from the package (dc:title), or NULL if it has none. */
const char  *epub_title(const epub_book_t *book);
/* Number of chapters: the files of the spine, in reading order. */
int          epub_chapter_count(const epub_book_t *book);
/* Chapter title from the table of contents, or NULL when the TOC does
 * not name it. */
const char  *epub_chapter_title(const epub_book_t *book, int chapter);

/* Convert a chapter to Markdown into dst (at most cap bytes, no
 * terminator). A chapter that does not fit is cut short with a note.
 * Returns the length written, or -1 on failure (*why set). */
long         epub_chapter_markdown(epub_book_t *book, int chapter,
                                   char *dst, size_t cap, const char **why);

/* XHTML (len bytes) to Markdown, as epub_chapter_markdown() does it;
 * exposed for the host test. */
size_t       epub_xhtml_to_markdown(const char *xhtml, size_t len,
                                    char *dst, size_t cap);

#ifdef __cplusplus
}
#endif
