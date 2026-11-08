#include "editor_menu.h"

static const menu_item_id_t s_menu_row_ids[] = {
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

static constexpr int s_menu_row_count =
    sizeof(s_menu_row_ids) / sizeof(s_menu_row_ids[0]);

static_assert(MENU_ID_SETTINGS == 0 && MENU_ID_BLE_STATUS == 1 &&
              MENU_ID_BLE_SCAN == 2 && MENU_ID_WIFI_CONNECT == 3 &&
              MENU_ID_WIFI_NEW == 4 && MENU_ID_WIFI_DISCONNECT == 5 &&
              MENU_ID_GIT_SYNC == 6 && MENU_ID_DRAFTBOX == 7 &&
              MENU_ID_KB_LAYOUT == 8 && MENU_ID_USB_MSC == 9 &&
              MENU_ID_KERMIT == 10 && MENU_ID_FORMAT_SD == 11 &&
              MENU_ID_HWINFO == 12 && MENU_ID_HELP == 13 &&
              MENU_ID_SLEEP == 14 && MENU_ID_CLOSE == 15,
              "Menu IDs must remain stable across feature configurations");

int editor_menu_row_count(void)
{
    return s_menu_row_count;
}

menu_item_id_t editor_menu_id_for_index(int idx)
{
    if (idx >= 0 && idx < s_menu_row_count) return s_menu_row_ids[idx];
    return MENU_ID_INVALID;
}

int editor_menu_index_for_id(menu_item_id_t id)
{
    for (int i = 0; i < s_menu_row_count; i++) {
        if (s_menu_row_ids[i] == id) return i;
    }
    return -1;
}

int editor_menu_move_index(int current_idx, int delta)
{
    if (s_menu_row_count <= 0) return 0;

    int current = current_idx % s_menu_row_count;
    int offset = delta % s_menu_row_count;
    if (current < 0) current += s_menu_row_count;
    if (offset < 0) offset += s_menu_row_count;
    return (current + offset) % s_menu_row_count;
}
