#pragma once

#include <stdbool.h>

typedef int editor_doc_format_t;

bool editor_path_format(const char *path, editor_doc_format_t *out);
