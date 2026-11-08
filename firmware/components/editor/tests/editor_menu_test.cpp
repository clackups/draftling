#include "editor_menu.h"
#include "editor_settings.h"

#include <climits>
#include <cstdio>
#include <vector>

static int fail(const char *message)
{
    std::fprintf(stderr, "editor_menu_test: %s\n", message);
    return 1;
}

static int normalized(int value, int count)
{
    int result = value % count;
    return result < 0 ? result + count : result;
}

static bool check_rows(menu_item_id_t parent,
                       const std::vector<menu_item_id_t> &expected)
{
    if (editor_menu_row_count(parent) != static_cast<int>(expected.size())) {
        std::fprintf(stderr, "editor_menu_test: unexpected row count for parent %d\n",
                     static_cast<int>(parent));
        return false;
    }

    for (int i = 0; i < static_cast<int>(expected.size()); ++i) {
        menu_item_id_t id = editor_menu_id_for_index(parent, i);
        if (id != expected[i] || editor_menu_index_for_id(parent, id) != i ||
            editor_menu_parent_for_id(id) != parent) {
            std::fprintf(stderr, "editor_menu_test: hierarchy mismatch at parent %d row %d\n",
                         static_cast<int>(parent), i);
            return false;
        }
        for (int j = 0; j < i; ++j) {
            if (expected[j] == id) return false;
        }
    }
    return true;
}

int main()
{
    if (!editor_menu_validate()) return fail("menu descriptor validation failed");

    if (!check_rows(MENU_ID_ROOT, {
            MENU_ID_SETTINGS,
            MENU_ID_CONNECTIVITY,
            MENU_ID_WRITING,
            MENU_ID_STORAGE,
            MENU_ID_SYSTEM,
            MENU_ID_CLOSE,
        }) ||
        !check_rows(MENU_ID_CONNECTIVITY, {
            MENU_ID_BLUETOOTH,
            MENU_ID_WIFI,
            MENU_ID_GIT_SYNC,
            MENU_ID_DRAFTBOX,
#if defined(CONFIG_DRAFTLING_HAS_KERMIT)
            MENU_ID_KERMIT,
#endif
        }) ||
        !check_rows(MENU_ID_BLUETOOTH, {
            MENU_ID_BLE_STATUS,
            MENU_ID_BLE_SCAN,
        }) ||
        !check_rows(MENU_ID_WIFI, {
            MENU_ID_WIFI_STATUS,
            MENU_ID_WIFI_CONNECT,
            MENU_ID_WIFI_NEW,
            MENU_ID_WIFI_DISCONNECT,
        }) ||
        !check_rows(MENU_ID_WRITING, {
            MENU_ID_KB_LAYOUT,
            MENU_ID_PROJECT_INFO,
        }) ||
        !check_rows(MENU_ID_STORAGE, {
#if defined(CONFIG_DRAFTLING_HAS_USB_MSC)
            MENU_ID_USB_MSC,
#endif
            MENU_ID_FORMAT_SD,
        }) ||
        !check_rows(MENU_ID_SYSTEM, {
            MENU_ID_HWINFO,
            MENU_ID_HELP,
            MENU_ID_SLEEP,
        })) {
        return fail("menu hierarchy does not match the feature-gated definition");
    }

    if (editor_menu_id_for_index(MENU_ID_ROOT, -1) != MENU_ID_INVALID ||
        editor_menu_id_for_index(MENU_ID_ROOT, editor_menu_row_count(MENU_ID_ROOT)) != MENU_ID_INVALID ||
        editor_menu_index_for_id(MENU_ID_ROOT, MENU_ID_INVALID) != -1 ||
        editor_menu_parent_for_id(MENU_ID_INVALID) != MENU_ID_INVALID ||
        editor_menu_title(MENU_ID_CONNECTIVITY) == nullptr ||
        editor_menu_title(MENU_ID_BLE_STATUS) != nullptr) {
        return fail("invalid lookup or container title behavior changed");
    }

#if !defined(CONFIG_DRAFTLING_HAS_USB_MSC)
    if (editor_menu_index_for_id(MENU_ID_STORAGE, MENU_ID_USB_MSC) != -1) {
        return fail("USB menu item is visible without USB MSC support");
    }
#endif
#if !defined(CONFIG_DRAFTLING_HAS_KERMIT)
    if (editor_menu_index_for_id(MENU_ID_CONNECTIVITY, MENU_ID_KERMIT) != -1) {
        return fail("Kermit menu item is visible without Kermit support");
    }
#endif

    const menu_item_id_t parents[] = {
        MENU_ID_ROOT, MENU_ID_CONNECTIVITY, MENU_ID_BLUETOOTH, MENU_ID_WIFI,
        MENU_ID_WRITING, MENU_ID_STORAGE, MENU_ID_SYSTEM,
    };
    for (menu_item_id_t parent : parents) {
        const int count = editor_menu_row_count(parent);
        if (count <= 0 ||
            editor_menu_move_index(parent, 0, -1) != count - 1 ||
            editor_menu_move_index(parent, count - 1, 1) != 0 ||
            editor_menu_move_index(parent, -1, 0) != count - 1 ||
            editor_menu_move_index(parent, count, 0) != 0) {
            return fail("menu selection did not wrap correctly");
        }
        if (editor_menu_move_index(parent, INT_MAX, INT_MIN) !=
            (normalized(INT_MAX, count) + normalized(INT_MIN, count)) % count) {
            return fail("menu navigation failed for large deltas");
        }
        for (int current = -count - 1; current <= count + 1; ++current) {
            for (int delta = -2 * count; delta <= 2 * count; ++delta) {
                const int expected_index =
                    (normalized(current, count) + normalized(delta, count)) % count;
                if (editor_menu_move_index(parent, current, delta) != expected_index) {
                    return fail("menu navigation failed for a wrapped index or delta");
                }
            }
        }
    }

    const settings_item_id_t setting_ids[] = {
        SETTINGS_ID_TIMEOUT,
        SETTINGS_ID_FONTSZ,
        SETTINGS_ID_MAXFILE,
        SETTINGS_ID_BACKLIGHT,
        SETTINGS_ID_THEME,
        SETTINGS_ID_ROTATE,
        SETTINGS_ID_INVERT,
        SETTINGS_ID_APPEND_ONLY,
        SETTINGS_ID_LAYOUTS,
        SETTINGS_ID_ORIENTATION,
        SETTINGS_ID_UPSIDE_DOWN,
        SETTINGS_ID_SCALE2X,
        SETTINGS_ID_MARGIN_LEFT,
        SETTINGS_ID_MARGIN_RIGHT,
        SETTINGS_ID_MARGIN_TOP,
        SETTINGS_ID_MARGIN_BOTTOM,
        SETTINGS_ID_RESET,
        SETTINGS_ID_BACK,
    };
    if (SETTINGS_ID_INVALID != -1) return fail("invalid setting ID changed");
    for (int i = 0; i < static_cast<int>(sizeof(setting_ids) / sizeof(setting_ids[0])); ++i) {
        if (setting_ids[i] != i) return fail("setting ID changed with build feature configuration");
        for (int j = 0; j < i; ++j) {
            if (setting_ids[j] == setting_ids[i]) return fail("duplicate setting ID");
        }
    }

    return 0;
}
