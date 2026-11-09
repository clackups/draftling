#include "editor_settings.h"

#include <cstdio>
#include <vector>

static int fail(const char *message)
{
    std::fprintf(stderr, "editor_settings_test: %s\n", message);
    return 1;
}

static bool check_rows(settings_item_id_t parent,
                       const std::vector<settings_item_id_t> &expected)
{
    if (editor_settings_row_count(parent) != static_cast<int>(expected.size())) {
        std::fprintf(stderr, "editor_settings_test: unexpected row count for parent %d\n",
                     static_cast<int>(parent));
        return false;
    }

    for (int i = 0; i < static_cast<int>(expected.size()); ++i) {
        settings_item_id_t id = editor_settings_id_for_index(parent, i);
        if (id != expected[i] || editor_settings_index_for_id(parent, id) != i ||
            editor_settings_parent_for_id(id) != parent) {
            std::fprintf(stderr, "editor_settings_test: hierarchy mismatch at parent %d row %d\n",
                         static_cast<int>(parent), i);
            return false;
        }
    }
    return true;
}

int main()
{
    if (!editor_settings_validate()) return fail("settings descriptor validation failed");

    std::vector<settings_item_id_t> display_rows;
#if defined(CONFIG_DRAFTLING_DISPLAY_HAS_BACKLIGHT) && !defined(CONFIG_DRAFTLING_DISPLAY_BACKLIGHT_BINARY)
    display_rows.push_back(SETTINGS_ID_BACKLIGHT);
#endif
#if defined(CONFIG_DRAFTLING_DISPLAY_COLOR)
    display_rows.push_back(SETTINGS_ID_THEME);
#endif
    display_rows.push_back(SETTINGS_ID_ORIENTATION);
#if defined(CONFIG_DRAFTLING_DISPLAY_CAN_ROTATE)
    display_rows.push_back(SETTINGS_ID_ROTATE);
#else
    display_rows.push_back(SETTINGS_ID_UPSIDE_DOWN);
#endif
#if defined(CONFIG_DRAFTLING_DISPLAY_CAN_INVERT)
    display_rows.push_back(SETTINGS_ID_INVERT);
#endif
    display_rows.push_back(SETTINGS_ID_SCALE2X);
    display_rows.push_back(SETTINGS_ID_MARGINS);

    if (!check_rows(SETTINGS_ID_ROOT, {
            SETTINGS_ID_EDITOR,
            SETTINGS_ID_DISPLAY,
            SETTINGS_ID_KEYBOARD,
            SETTINGS_ID_POWER,
            SETTINGS_ID_SYSTEM,
        }) ||
        !check_rows(SETTINGS_ID_EDITOR, {
            SETTINGS_ID_FONTSZ,
            SETTINGS_ID_APPEND_ONLY,
            SETTINGS_ID_MAXFILE,
        }) ||
        !check_rows(SETTINGS_ID_DISPLAY, display_rows) ||
        !check_rows(SETTINGS_ID_MARGINS, {
            SETTINGS_ID_MARGIN_LEFT,
            SETTINGS_ID_MARGIN_RIGHT,
            SETTINGS_ID_MARGIN_TOP,
            SETTINGS_ID_MARGIN_BOTTOM,
        }) ||
        !check_rows(SETTINGS_ID_KEYBOARD, { SETTINGS_ID_LAYOUTS }) ||
        !check_rows(SETTINGS_ID_POWER, { SETTINGS_ID_TIMEOUT }) ||
        !check_rows(SETTINGS_ID_SYSTEM, { SETTINGS_ID_RESET })) {
        return fail("Settings hierarchy or feature-gated row order changed");
    }

    if (editor_settings_id_for_index(SETTINGS_ID_ROOT, -1) != SETTINGS_ID_INVALID ||
        editor_settings_id_for_index(SETTINGS_ID_ROOT, 5) != SETTINGS_ID_INVALID ||
        editor_settings_index_for_id(SETTINGS_ID_ROOT, SETTINGS_ID_RESET) != -1 ||
        editor_settings_parent_for_id(SETTINGS_ID_INVALID) != SETTINGS_ID_INVALID ||
        editor_settings_title(SETTINGS_ID_DISPLAY) == nullptr ||
        editor_settings_title(SETTINGS_ID_FONTSZ) != nullptr) {
        return fail("invalid lookup or category title behavior changed");
    }

    const settings_item_id_t original_ids[] = {
        SETTINGS_ID_TIMEOUT, SETTINGS_ID_FONTSZ, SETTINGS_ID_MAXFILE,
        SETTINGS_ID_BACKLIGHT, SETTINGS_ID_THEME, SETTINGS_ID_ROTATE,
        SETTINGS_ID_INVERT, SETTINGS_ID_APPEND_ONLY, SETTINGS_ID_LAYOUTS,
        SETTINGS_ID_ORIENTATION, SETTINGS_ID_UPSIDE_DOWN, SETTINGS_ID_SCALE2X,
        SETTINGS_ID_MARGIN_LEFT, SETTINGS_ID_MARGIN_RIGHT, SETTINGS_ID_MARGIN_TOP,
        SETTINGS_ID_MARGIN_BOTTOM, SETTINGS_ID_RESET, SETTINGS_ID_BACK,
    };
    for (int i = 0; i < static_cast<int>(sizeof(original_ids) / sizeof(original_ids[0])); ++i) {
        if (original_ids[i] != i) return fail("existing setting ID changed");
        for (int j = 0; j < i; ++j) {
            if (original_ids[i] == original_ids[j]) return fail("duplicate existing setting ID");
        }
    }
    const settings_item_id_t category_ids[] = {
        SETTINGS_ID_ROOT, SETTINGS_ID_EDITOR, SETTINGS_ID_DISPLAY,
        SETTINGS_ID_KEYBOARD, SETTINGS_ID_POWER, SETTINGS_ID_SYSTEM,
        SETTINGS_ID_MARGINS,
    };
    for (int i = 0; i < static_cast<int>(sizeof(category_ids) / sizeof(category_ids[0])); ++i) {
        if (category_ids[i] < 18) return fail("category ID overlaps an existing setting ID");
        for (int j = 0; j < i; ++j) {
            if (category_ids[i] == category_ids[j]) return fail("duplicate category ID");
        }
        for (settings_item_id_t id : original_ids) {
            if (category_ids[i] == id) return fail("category ID overlaps a setting ID");
        }
    }

    return 0;
}
