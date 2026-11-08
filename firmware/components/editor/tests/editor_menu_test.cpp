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

int main()
{
    const std::vector<menu_item_id_t> expected = {
        MENU_ID_SETTINGS,
        MENU_ID_BLE_STATUS,
        MENU_ID_BLE_SCAN,
        MENU_ID_WIFI_CONNECT,
        MENU_ID_WIFI_NEW,
        MENU_ID_WIFI_DISCONNECT,
        MENU_ID_GIT_SYNC,
        MENU_ID_DRAFTBOX,
        MENU_ID_KB_LAYOUT,
#if defined(CONFIG_DRAFTLING_HAS_USB_MSC)
        MENU_ID_USB_MSC,
#endif
#if defined(CONFIG_DRAFTLING_HAS_KERMIT)
        MENU_ID_KERMIT,
#endif
        MENU_ID_FORMAT_SD,
        MENU_ID_HWINFO,
        MENU_ID_HELP,
        MENU_ID_SLEEP,
        MENU_ID_CLOSE,
    };

    const int count = editor_menu_row_count();
    if (count != static_cast<int>(expected.size())) {
        return fail("visible row count does not match the feature-gated menu");
    }

    for (int i = 0; i < count; ++i) {
        const menu_item_id_t id = editor_menu_id_for_index(i);
        if (id != expected[i]) return fail("visible menu row order changed");
        if (editor_menu_index_for_id(id) != i) {
            return fail("visible row ID does not round-trip to its index");
        }
        for (int j = 0; j < i; ++j) {
            if (expected[j] == id) return fail("visible menu contains a duplicate ID");
        }
    }

    if (editor_menu_id_for_index(-1) != MENU_ID_INVALID ||
        editor_menu_id_for_index(count) != MENU_ID_INVALID) {
        return fail("out-of-range row lookup did not return MENU_ID_INVALID");
    }
    if (editor_menu_index_for_id(MENU_ID_INVALID) != -1 ||
        editor_menu_index_for_id(static_cast<menu_item_id_t>(1000)) != -1) {
        return fail("unknown ID lookup did not return -1");
    }

#if !defined(CONFIG_DRAFTLING_HAS_USB_MSC)
    if (editor_menu_index_for_id(MENU_ID_USB_MSC) != -1) {
        return fail("USB menu item is visible without USB MSC support");
    }
#endif
#if !defined(CONFIG_DRAFTLING_HAS_KERMIT)
    if (editor_menu_index_for_id(MENU_ID_KERMIT) != -1) {
        return fail("Kermit menu item is visible without Kermit support");
    }
#endif

    if (editor_menu_move_index(0, -1) != count - 1 ||
        editor_menu_move_index(count - 1, 1) != 0 ||
        editor_menu_move_index(-1, 0) != count - 1 ||
        editor_menu_move_index(count, 0) != 0) {
        return fail("menu selection did not wrap correctly");
    }
    if (editor_menu_move_index(INT_MAX, INT_MIN) !=
        (normalized(INT_MAX, count) + normalized(INT_MIN, count)) % count) {
        return fail("menu navigation failed for large deltas");
    }
    for (int current = -count - 1; current <= count + 1; ++current) {
        for (int delta = -2 * count; delta <= 2 * count; ++delta) {
            const int expected_index =
                (normalized(current, count) + normalized(delta, count)) % count;
            if (editor_menu_move_index(current, delta) != expected_index) {
                return fail("menu navigation failed for a wrapped index or delta");
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
        if (setting_ids[i] != i) {
            return fail("setting ID changed with build feature configuration");
        }
        for (int j = 0; j < i; ++j) {
            if (setting_ids[j] == setting_ids[i]) {
                return fail("duplicate setting ID");
            }
        }
    }

    return 0;
}
