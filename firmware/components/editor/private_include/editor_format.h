#pragma once

/* Per-format editing behaviour, private to the editor component.
 *
 * A document is Markdown, Fountain or plain text (editor_get_format()).
 * editor_ui.cpp owns everything the formats share -- line labels,
 * cursor, selection, the WYSIWYG decoration painter -- and asks the
 * active document's editor_format_t for what differs: how a line is
 * classified (with any state carried from the lines above it), which
 * raw bytes are hidden markers and which get a line-wide decoration,
 * the per-element label layout, whether the e-paper typing fast path
 * may be used, and format-specific Enter / Tab behaviour.
 *
 * md_editor.cpp implements Markdown, fountain_editor.cpp Fountain and
 * text_editor.cpp plain text. */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "lvgl.h"
#include "editor.h"
#include "md_parser.h"
#include "fountain_parser.h"

/* Per-character decoration flags painted over a line label. */
enum {
    DECO_BOLD   = 0x01,
    DECO_ITALIC = 0x02,
    DECO_STRIKE = 0x04,
    DECO_CODE   = 0x08,
    DECO_BULLET = 0x10,   /* list-marker cell: draw a bullet dot */
    DECO_UNDER  = 0x20,   /* Fountain _underline_ */
};

/* State threaded through a document's lines from the top. */
typedef struct {
    const char *flat;       /* flattened document text */
    size_t      flat_len;
    size_t      cursor;     /* cursor byte offset in flat */
    bool        in_code;    /* Markdown: inside a fenced code block */
    ftn_state_t ftn;        /* Fountain: element state */
} fmt_scan_t;

/* Line-wide marks a format sets for one classified line. hide and
 * rflags have one entry per raw byte (ll of them); content_off /
 * content_end delimit md_line_info_t::content within the line. */
typedef struct {
    uint8_t *hide;          /* 1 = marker byte not shown */
    uint8_t *rflags;        /* DECO_* per raw byte */
    size_t   ll;
    size_t   content_off;
    size_t   content_end;
    bool     hr;            /* draw a horizontal rule */
    uint8_t  bullet_level;  /* list nesting, picks the bullet shape */
} fmt_marks_t;

typedef struct {
    void (*scan_init)(fmt_scan_t *sc);
    /* Classify lt[0, ll) -- the next line, lying inside sc->flat --
     * and advance sc past it. mi may be NULL. */
    void (*parse_line)(fmt_scan_t *sc, const char *lt, size_t ll,
                       md_line_info_t *mi);
    /* Hide the line's markers (unless reveal: the focused cursor line)
     * and set line-wide decorations. Inline spans are applied by the
     * caller. */
    void (*mark_line)(const md_line_info_t *mi, bool reveal, fmt_marks_t *m);
    /* Local styles on top of the line type's shared style; w is the
     * label width. */
    void (*apply_layout)(lv_obj_t *label, md_line_type_t type, int32_t w);
    /* The line displays as its raw text with no styling, so the
     * e-paper typing fast path's per-character math applies. */
    bool (*line_is_plain)(const char *lt, size_t ll);
    /* Key hooks for the focused document: return true when the key was
     * handled (the text may have changed). */
    bool (*handle_enter)(bool shift);
    bool (*handle_tab)(bool append_only);
} editor_format_t;

extern const editor_format_t md_editor_format;
extern const editor_format_t fountain_editor_format;
extern const editor_format_t text_editor_format;

/* The format of the active document. */
static inline const editor_format_t *editor_format_active(void)
{
    switch (editor_get_format()) {
    case EDITOR_DOC_FOUNTAIN: return &fountain_editor_format;
    case EDITOR_DOC_TEXT:     return &text_editor_format;
    default:                  return &md_editor_format;
    }
}

/* Start a top-to-bottom scan of the active document. */
static inline void editor_format_scan_begin(const editor_format_t *f, fmt_scan_t *sc)
{
    sc->flat = editor_get_text(&sc->flat_len);
    sc->cursor = editor_get_cursor();
    f->scan_init(sc);
}
