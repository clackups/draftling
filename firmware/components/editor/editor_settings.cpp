#include "editor_settings.h"

#include <stddef.h>

typedef struct settings_node_t {
    settings_item_id_t id;
    settings_item_id_t parent;
    const char *title;
} settings_node_t;

static const settings_node_t s_settings_nodes[] = {
    { SETTINGS_ID_ROOT, SETTINGS_ID_INVALID, "Settings" },
    { SETTINGS_ID_EDITOR, SETTINGS_ID_ROOT, "Editor" },
    { SETTINGS_ID_FONTSZ, SETTINGS_ID_EDITOR, NULL },
    { SETTINGS_ID_APPEND_ONLY, SETTINGS_ID_EDITOR, NULL },
    { SETTINGS_ID_MAXFILE, SETTINGS_ID_EDITOR, NULL },
    { SETTINGS_ID_DISPLAY, SETTINGS_ID_ROOT, "Display" },
#if defined(CONFIG_DRAFTLING_DISPLAY_HAS_BACKLIGHT) && !defined(CONFIG_DRAFTLING_DISPLAY_BACKLIGHT_BINARY)
    { SETTINGS_ID_BACKLIGHT, SETTINGS_ID_DISPLAY, NULL },
#endif
#if defined(CONFIG_DRAFTLING_DISPLAY_COLOR)
    { SETTINGS_ID_THEME, SETTINGS_ID_DISPLAY, NULL },
#endif
    { SETTINGS_ID_ORIENTATION, SETTINGS_ID_DISPLAY, NULL },
#if defined(CONFIG_DRAFTLING_DISPLAY_CAN_ROTATE)
    { SETTINGS_ID_ROTATE, SETTINGS_ID_DISPLAY, NULL },
#else
    { SETTINGS_ID_UPSIDE_DOWN, SETTINGS_ID_DISPLAY, NULL },
#endif
#if defined(CONFIG_DRAFTLING_DISPLAY_CAN_INVERT)
    { SETTINGS_ID_INVERT, SETTINGS_ID_DISPLAY, NULL },
#endif
    { SETTINGS_ID_SCALE2X, SETTINGS_ID_DISPLAY, NULL },
    { SETTINGS_ID_MARGINS, SETTINGS_ID_DISPLAY, "Screen margins" },
    { SETTINGS_ID_MARGIN_LEFT, SETTINGS_ID_MARGINS, NULL },
    { SETTINGS_ID_MARGIN_RIGHT, SETTINGS_ID_MARGINS, NULL },
    { SETTINGS_ID_MARGIN_TOP, SETTINGS_ID_MARGINS, NULL },
    { SETTINGS_ID_MARGIN_BOTTOM, SETTINGS_ID_MARGINS, NULL },
    { SETTINGS_ID_KEYBOARD, SETTINGS_ID_ROOT, "Keyboard" },
    { SETTINGS_ID_LAYOUTS, SETTINGS_ID_KEYBOARD, NULL },
    { SETTINGS_ID_POWER, SETTINGS_ID_ROOT, "Power" },
    { SETTINGS_ID_TIMEOUT, SETTINGS_ID_POWER, NULL },
    { SETTINGS_ID_SYSTEM, SETTINGS_ID_ROOT, "System" },
    { SETTINGS_ID_RESET, SETTINGS_ID_SYSTEM, NULL },
};

static const settings_node_t *node_for_id(settings_item_id_t id)
{
    for (const settings_node_t &node : s_settings_nodes) {
        if (node.id == id) return &node;
    }
    return NULL;
}

int editor_settings_row_count(settings_item_id_t parent)
{
    int count = 0;
    for (const settings_node_t &node : s_settings_nodes) {
        if (node.parent == parent) ++count;
    }
    return count;
}

settings_item_id_t editor_settings_id_for_index(settings_item_id_t parent, int index)
{
    if (index < 0) return SETTINGS_ID_INVALID;
    for (const settings_node_t &node : s_settings_nodes) {
        if (node.parent == parent && index-- == 0) return node.id;
    }
    return SETTINGS_ID_INVALID;
}

int editor_settings_index_for_id(settings_item_id_t parent, settings_item_id_t id)
{
    int index = 0;
    for (const settings_node_t &node : s_settings_nodes) {
        if (node.parent != parent) continue;
        if (node.id == id) return index;
        ++index;
    }
    return -1;
}

settings_item_id_t editor_settings_parent_for_id(settings_item_id_t id)
{
    const settings_node_t *node = node_for_id(id);
    return node ? node->parent : SETTINGS_ID_INVALID;
}

const char *editor_settings_title(settings_item_id_t id)
{
    const settings_node_t *node = node_for_id(id);
    return node ? node->title : NULL;
}

int editor_settings_validate(void)
{
    for (size_t i = 0; i < sizeof(s_settings_nodes) / sizeof(s_settings_nodes[0]); ++i) {
        const settings_node_t &node = s_settings_nodes[i];
        if (node.id == SETTINGS_ID_INVALID) return 0;
        for (size_t j = i + 1; j < sizeof(s_settings_nodes) / sizeof(s_settings_nodes[0]); ++j) {
            if (s_settings_nodes[j].id == node.id) return 0;
        }

        if (node.parent != SETTINGS_ID_INVALID) {
            const settings_node_t *parent = node_for_id(node.parent);
            if (!parent || !parent->title) return 0;
        }

        const int children = editor_settings_row_count(node.id);
        if ((node.title != NULL) != (children > 0)) return 0;
    }

    return node_for_id(SETTINGS_ID_ROOT) &&
           editor_settings_row_count(SETTINGS_ID_ROOT) > 0;
}
