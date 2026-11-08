#include "editor_menu.h"

struct menu_item_t {
    menu_item_id_t id;
    menu_item_id_t parent;
    const char *title;
    bool submenu;
};

static const menu_item_t s_menu_items[] = {
    { MENU_ID_SETTINGS, MENU_ID_ROOT, "Settings", false },
    { MENU_ID_CONNECTIVITY, MENU_ID_ROOT, "Connectivity", true },
    { MENU_ID_WRITING, MENU_ID_ROOT, "Writing", true },
    { MENU_ID_STORAGE, MENU_ID_ROOT, "Storage", true },
    { MENU_ID_SYSTEM, MENU_ID_ROOT, "System", true },
    { MENU_ID_CLOSE, MENU_ID_ROOT, "Close menu (Esc / F1)", false },
    { MENU_ID_BLUETOOTH, MENU_ID_CONNECTIVITY, "Bluetooth", true },
    { MENU_ID_WIFI, MENU_ID_CONNECTIVITY, "WiFi", true },
    { MENU_ID_GIT_SYNC, MENU_ID_CONNECTIVITY, nullptr, false },
    { MENU_ID_DRAFTBOX, MENU_ID_CONNECTIVITY, "Connect a Draftbox repository...", false },
#if defined(CONFIG_DRAFTLING_HAS_KERMIT)
    { MENU_ID_KERMIT, MENU_ID_CONNECTIVITY, nullptr, false },
#endif
    { MENU_ID_WIFI_STATUS, MENU_ID_WIFI, nullptr, false },
    { MENU_ID_WIFI_CONNECT, MENU_ID_WIFI, nullptr, false },
    { MENU_ID_WIFI_NEW, MENU_ID_WIFI, "WiFi: New connection...", false },
    { MENU_ID_WIFI_DISCONNECT, MENU_ID_WIFI, "WiFi: Disconnect", false },
    { MENU_ID_BLE_STATUS, MENU_ID_BLUETOOTH, nullptr, false },
    { MENU_ID_BLE_SCAN, MENU_ID_BLUETOOTH, "BLE: Start scan", false },
    { MENU_ID_KB_LAYOUT, MENU_ID_WRITING, nullptr, false },
    { MENU_ID_PROJECT_INFO, MENU_ID_WRITING, "Project information", false },
#if defined(CONFIG_DRAFTLING_HAS_USB_MSC)
    { MENU_ID_USB_MSC, MENU_ID_STORAGE, nullptr, false },
#endif
    { MENU_ID_FORMAT_SD, MENU_ID_STORAGE, nullptr, false },
    { MENU_ID_HWINFO, MENU_ID_SYSTEM, "Hardware information", false },
    { MENU_ID_HELP, MENU_ID_SYSTEM, "Help (F10)", false },
    { MENU_ID_SLEEP, MENU_ID_SYSTEM, "Sleep now", false },
};

static constexpr int s_menu_item_count =
    sizeof(s_menu_items) / sizeof(s_menu_items[0]);

static_assert(MENU_ID_SETTINGS == 0 && MENU_ID_BLE_STATUS == 1 &&
              MENU_ID_BLE_SCAN == 2 && MENU_ID_WIFI_CONNECT == 3 &&
              MENU_ID_WIFI_NEW == 4 && MENU_ID_WIFI_DISCONNECT == 5 &&
              MENU_ID_GIT_SYNC == 6 && MENU_ID_DRAFTBOX == 7 &&
              MENU_ID_KB_LAYOUT == 8 && MENU_ID_USB_MSC == 9 &&
              MENU_ID_KERMIT == 10 && MENU_ID_FORMAT_SD == 11 &&
              MENU_ID_HWINFO == 12 && MENU_ID_HELP == 13 &&
              MENU_ID_SLEEP == 14 && MENU_ID_CLOSE == 15 &&
              MENU_ID_PROJECT_INFO == 16 && MENU_ID_WIFI_STATUS == 17,
              "Action IDs must remain stable across feature configurations");

int editor_menu_row_count(menu_item_id_t parent)
{
    int count = 0;
    for (int i = 0; i < s_menu_item_count; ++i) {
        if (s_menu_items[i].parent == parent) ++count;
    }
    return count;
}

menu_item_id_t editor_menu_id_for_index(menu_item_id_t parent, int idx)
{
    if (idx < 0) return MENU_ID_INVALID;
    for (int i = 0; i < s_menu_item_count; ++i) {
        if (s_menu_items[i].parent != parent) continue;
        if (idx-- == 0) return s_menu_items[i].id;
    }
    return MENU_ID_INVALID;
}

int editor_menu_index_for_id(menu_item_id_t parent, menu_item_id_t id)
{
    int idx = 0;
    for (int i = 0; i < s_menu_item_count; ++i) {
        if (s_menu_items[i].parent != parent) continue;
        if (s_menu_items[i].id == id) return idx;
        ++idx;
    }
    return -1;
}

menu_item_id_t editor_menu_parent_for_id(menu_item_id_t id)
{
    for (int i = 0; i < s_menu_item_count; ++i) {
        if (s_menu_items[i].id == id) return s_menu_items[i].parent;
    }
    return MENU_ID_INVALID;
}

const char *editor_menu_title(menu_item_id_t id)
{
    for (int i = 0; i < s_menu_item_count; ++i) {
        if (s_menu_items[i].id == id) return s_menu_items[i].title;
    }
    return nullptr;
}

int editor_menu_move_index(menu_item_id_t parent, int current_idx, int delta)
{
    int count = editor_menu_row_count(parent);
    if (count <= 0) return 0;

    int current = current_idx % count;
    int offset = delta % count;
    if (current < 0) current += count;
    if (offset < 0) offset += count;
    return (current + offset) % count;
}

int editor_menu_validate(void)
{
    for (int i = 0; i < s_menu_item_count; ++i) {
        const menu_item_t &item = s_menu_items[i];
        if (item.id == MENU_ID_INVALID || item.id == MENU_ID_ROOT) return 0;
        for (int j = 0; j < i; ++j) {
            if (s_menu_items[j].id == item.id) return 0;
        }

        if (item.parent != MENU_ID_ROOT &&
            editor_menu_row_count(item.parent) == 0) {
            return 0;
        }

        menu_item_id_t ancestor = item.parent;
        for (int depth = 0; ancestor != MENU_ID_ROOT; ++depth) {
            if (depth >= s_menu_item_count) return 0;
            bool found = false;
            for (int j = 0; j < s_menu_item_count; ++j) {
                if (s_menu_items[j].id != ancestor) continue;
                ancestor = s_menu_items[j].parent;
                found = true;
                break;
            }
            if (!found) return 0;
        }

        const bool has_children = editor_menu_row_count(item.id) > 0;
        if (has_children != item.submenu || (item.submenu && !item.title))
            return 0;
    }

    return editor_menu_row_count(MENU_ID_ROOT) > 0;
}
