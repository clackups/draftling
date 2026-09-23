#pragma once

/* Line classifier for the Fountain screenplay format
 * (https://fountain.io/syntax/).
 *
 * Unlike Markdown, a Fountain element depends on its neighbours: a
 * Character name must follow a blank line and be followed by text, a
 * line after a Character is Dialogue, the title page is only recognised
 * at the top of the document, and boneyard comments and notes can span
 * lines. The caller therefore walks the document from its first line,
 * threading one ftn_state_t through every fountain_parse_line() call,
 * and says for each line whether the line after it is blank.
 *
 * The result is reported in an md_line_info_t so the editor's WYSIWYG
 * renderer can treat both formats alike: the line type is one of the
 * MD_LINE_FTN_* values (plus MD_LINE_H1..H3 for sections, MD_LINE_HR
 * for page breaks and MD_LINE_EMPTY), content / content_len delimit the
 * text without the element's markers (forcing characters, the dual-
 * dialogue caret, centering brackets), and the spans carry emphasis:
 * *italic*, **bold**, ***bold italic***, _underline_, notes (code flag,
 * markers kept visible) and boneyard text (strikethrough flag, markers
 * kept visible). */

#include <stddef.h>
#include <stdbool.h>
#include "md_parser.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool at_start;      /* no line parsed yet: a title page may begin */
    bool title_page;    /* inside the title page block */
    bool prev_blank;    /* the previous element line was blank */
    bool in_dialogue;   /* inside a Character / Dialogue block */
    bool in_boneyard;   /* inside an unterminated boneyard comment */
    bool in_note;       /* inside an unterminated [[note]] */
} ftn_state_t;

/* Reset st to the state before the first line of a document. */
void ftn_state_init(ftn_state_t *st);

/* Classify one line (without its '\n') and advance st past it.
 * next_blank tells whether the following line is blank; the caller may
 * report a line that is still being typed as non-blank so an element
 * keeps its formatting while the user writes the next line. info may be
 * NULL when only the state needs advancing. */
void fountain_parse_line(const char *line, size_t len, bool next_blank,
                         ftn_state_t *st, md_line_info_t *info);

#ifdef __cplusplus
}
#endif
