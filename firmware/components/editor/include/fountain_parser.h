#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

typedef enum {
    FOUNTAIN_LINE_EMPTY,
    FOUNTAIN_LINE_SCENE_HEADING,
    FOUNTAIN_LINE_ACTION,
    FOUNTAIN_LINE_CHARACTER,
    FOUNTAIN_LINE_DIALOGUE,
    FOUNTAIN_LINE_PARENTHETICAL,
    FOUNTAIN_LINE_TRANSITION,
    FOUNTAIN_LINE_CENTERED,
    FOUNTAIN_LINE_PAGE_BREAK,
    FOUNTAIN_LINE_NOTE,
    FOUNTAIN_LINE_BONEYARD,
} fountain_line_type_t;

typedef enum {
    FOUNTAIN_STATE_NORMAL,
    FOUNTAIN_STATE_DIALOGUE,
    FOUNTAIN_STATE_BONEYARD,
} fountain_parse_state_t;

typedef struct {
    fountain_parse_state_t state;
} fountain_parser_state_t;

typedef struct {
    fountain_line_type_t type;

    /*
     * Pointer/length into the original source line.
     * No allocation is performed by the parser.
     */
    const char *content;
    size_t content_len;

    /*
     * Reserved for future indentation/layout information.
     * Currently always zero.
     */
    int indent_level;

    /*
     * True when the Fountain syntax explicitly forced this
     * classification, e.g. ".SCENE" or "!Action".
     */
    bool forced;
} fountain_line_info_t;

/*
 * Reset parser state before parsing a document from the beginning.
 */
void fountain_parser_reset(fountain_parser_state_t *state);

/*
 * Parse one Fountain source line.
 *
 * `state` must be maintained across successive lines when parsing
 * a document. It may be NULL for stateless classification.
 */
void fountain_parse_line(const char *line,
                         size_t len,
                         fountain_line_info_t *info,
                         fountain_parser_state_t *state);

/*
 * Helpers used by editor_ui.cpp when reconstructing parser state
 * or determining document structure.
 */
bool fountain_is_scene_heading(const char *line, size_t len);
bool fountain_is_page_break(const char *line, size_t len);

/*
 * Useful for diagnostics/tests and later outline/navigation code.
 */
const char *fountain_line_type_name(fountain_line_type_t type);

#ifdef __cplusplus
}
#endif
