#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum menu_item_id_t {
    MENU_ID_INVALID = -1,
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
} menu_item_id_t;

int editor_menu_row_count(void);
menu_item_id_t editor_menu_id_for_index(int idx);
int editor_menu_index_for_id(menu_item_id_t id);
int editor_menu_move_index(int current_idx, int delta);

#ifdef __cplusplus
}
#endif
