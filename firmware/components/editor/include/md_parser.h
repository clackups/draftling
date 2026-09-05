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
} md_line_type_t;

typedef struct {
    size_t start;
    size_t end;
    bool bold;
    bool italic;
    bool code;
    bool strikethrough;
} md_span_t;

typedef struct {
    md_line_type_t type;
    const char *content;
    size_t content_len;
    int indent_level;
    md_span_t spans[16];
    int span_count;
} md_line_info_t;

void md_parse_line(const char *line, size_t len, md_line_info_t *info, bool in_code_block);
bool md_is_code_fence(const char *line, size_t len);

#ifdef __cplusplus
}
#endif
