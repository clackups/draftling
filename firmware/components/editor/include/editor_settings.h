#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum settings_item_id_t {
    SETTINGS_ID_INVALID = -1,
    SETTINGS_ID_TIMEOUT = 0,
    SETTINGS_ID_FONTSZ = 1,
    SETTINGS_ID_MAXFILE = 2,
    SETTINGS_ID_BACKLIGHT = 3,
    SETTINGS_ID_THEME = 4,
    SETTINGS_ID_ROTATE = 5,
    SETTINGS_ID_INVERT = 6,
    SETTINGS_ID_APPEND_ONLY = 7,
    SETTINGS_ID_LAYOUTS = 8,
    SETTINGS_ID_ORIENTATION = 9,
    SETTINGS_ID_UPSIDE_DOWN = 10,
    SETTINGS_ID_SCALE2X = 11,
    SETTINGS_ID_MARGIN_LEFT = 12,
    SETTINGS_ID_MARGIN_RIGHT = 13,
    SETTINGS_ID_MARGIN_TOP = 14,
    SETTINGS_ID_MARGIN_BOTTOM = 15,
    SETTINGS_ID_RESET = 16,
    SETTINGS_ID_BACK = 17,
    SETTINGS_ID_ROOT = 18,
    SETTINGS_ID_EDITOR = 19,
    SETTINGS_ID_DISPLAY = 20,
    SETTINGS_ID_KEYBOARD = 21,
    SETTINGS_ID_POWER = 22,
    SETTINGS_ID_SYSTEM = 23,
    SETTINGS_ID_MARGINS = 24,
} settings_item_id_t;

int editor_settings_row_count(settings_item_id_t parent);
settings_item_id_t editor_settings_id_for_index(settings_item_id_t parent, int index);
int editor_settings_index_for_id(settings_item_id_t parent, settings_item_id_t id);
settings_item_id_t editor_settings_parent_for_id(settings_item_id_t id);
const char *editor_settings_title(settings_item_id_t id);
int editor_settings_validate(void);

#ifdef __cplusplus
}
#endif
