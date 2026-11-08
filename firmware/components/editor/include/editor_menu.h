#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum menu_item_id_t {
    MENU_ID_INVALID = -1,
    MENU_ID_ROOT = -2,
    MENU_ID_SETTINGS = 0,
    MENU_ID_BLE_STATUS = 1,
    MENU_ID_BLE_SCAN = 2,
    MENU_ID_WIFI_CONNECT = 3,
    MENU_ID_WIFI_NEW = 4,
    MENU_ID_WIFI_DISCONNECT = 5,
    MENU_ID_GIT_SYNC = 6,
    MENU_ID_DRAFTBOX = 7,
    MENU_ID_KB_LAYOUT = 8,
    MENU_ID_USB_MSC = 9,
    MENU_ID_KERMIT = 10,
    MENU_ID_FORMAT_SD = 11,
    MENU_ID_HWINFO = 12,
    MENU_ID_HELP = 13,
    MENU_ID_SLEEP = 14,
    MENU_ID_CLOSE = 15,
    MENU_ID_PROJECT_INFO = 16,
    MENU_ID_WIFI_STATUS = 17,
    MENU_ID_CONNECTIVITY = 20,
    MENU_ID_BLUETOOTH = 21,
    MENU_ID_WIFI = 22,
    MENU_ID_WRITING = 23,
    MENU_ID_STORAGE = 24,
    MENU_ID_SYSTEM = 25,
} menu_item_id_t;

int editor_menu_row_count(menu_item_id_t parent);
menu_item_id_t editor_menu_id_for_index(menu_item_id_t parent, int idx);
int editor_menu_index_for_id(menu_item_id_t parent, menu_item_id_t id);
menu_item_id_t editor_menu_parent_for_id(menu_item_id_t id);
const char *editor_menu_title(menu_item_id_t id);
int editor_menu_move_index(menu_item_id_t parent, int current_idx, int delta);
int editor_menu_validate(void);

#ifdef __cplusplus
}
#endif
