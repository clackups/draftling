#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

typedef enum {
    MD_LINE_PARAGRAPH,
    MD_LINE_H1,
    MD_LINE_H2,
    MD_LINE_H3,
    MD_LINE_H4,
    MD_LINE_BULLET,
    MD_LINE_NUMBERED,
    MD_LINE_BLOCKQUOTE,
    MD_LINE_CODE_FENCE,
    MD_LINE_CODE_CONTENT,
    MD_LINE_HR,
    MD_LINE_EMPTY,

    /* Fountain screenplay elements, produced only by fountain_parse_line()
     * (see fountain_parser.h). Sections reuse MD_LINE_H1..H3 and page
     * breaks reuse MD_LINE_HR; everything else has its own type. */
    MD_LINE_FTN_ACTION,
    MD_LINE_FTN_SCENE,
    MD_LINE_FTN_CHARACTER,
    MD_LINE_FTN_DIALOGUE,
    MD_LINE_FTN_PARENTHETICAL,
    MD_LINE_FTN_TRANSITION,
    MD_LINE_FTN_CENTERED,
    MD_LINE_FTN_LYRIC,
    MD_LINE_FTN_SYNOPSIS,
    MD_LINE_FTN_TITLE,
    MD_LINE_FTN_NOTE,       /* line entirely inside a [[note]] */
    MD_LINE_FTN_BONEYARD,   /* line entirely inside a boneyard comment */
} md_line_type_t;

/* An inline span. start/end delimit the styled text (the part between
 * the markers), as byte offsets relative to md_line_info_t::content;
 * mark_len is the length of the opening marker immediately before
 * start and of the identical closing marker immediately at end (e.g. 2
 * for "**bold**", 1 for "`code`"). Spans may nest ("**a *b* c**"
 * yields an outer bold span and an inner italic one), so a consumer
 * should OR the flags of every span covering a character. The
 * Fountain parser also emits spans with mark_len 0 (markers that stay
 * visible, e.g. notes, or no markers at all, e.g. a title-page key). */
typedef struct {
    size_t start;
    size_t end;
    size_t mark_len;
    bool bold;
    bool italic;
    bool code;
    bool strikethrough;
    bool underline;     /* Fountain _underline_ */
} md_span_t;

#define MD_MAX_SPANS 32

typedef struct {
    md_line_type_t type;
    const char *content;
    size_t content_len;
    int indent_level;
    md_span_t spans[MD_MAX_SPANS];
    int span_count;
} md_line_info_t;

void md_parse_line(const char *line, size_t len, md_line_info_t *info, bool in_code_block);
bool md_is_code_fence(const char *line, size_t len);

#ifdef __cplusplus
}
#endif
