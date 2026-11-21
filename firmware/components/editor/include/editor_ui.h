#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void editor_ui_init(void);
void editor_ui_refresh(void);
void editor_ui_handle_key(const void *event);
void editor_ui_show_file_browser(void);
void editor_ui_show_editor(void);
void editor_ui_set_status(const char *msg);

/* Ask the editor to enter deep sleep, as Ctrl+P does: with unsaved
 * changes in the open document the Save / Discard / Cancel prompt is
 * shown instead. For a hardware Power button. Safe to call from any
 * task; the request is handled by the UI task. */
void editor_ui_request_sleep(void);

/* Display a persistent (non-auto-clearing) status message. Used for
 * fatal boot-time errors where the firmware halts and the message
 * must remain on screen until the user resets the device. */
void editor_ui_show_fatal(const char *msg);

/* Replace the text on the boot "BLE prompt" screen. Used by main.cpp
 * to delay the "Searching for BLE keyboard..." prompt until after
 * the USB host probe window, so a wired keyboard never sees that
 * message at boot. Safe to call before editor_ui_init() (no-op) and
 * after the BLE prompt screen has been torn down. */
void editor_ui_set_ble_prompt_text(const char *text);

/* Board name shown on the F1 -> "Hardware information" page. The
 * string must stay valid for the life of the program. */
void editor_ui_set_board_name(const char *name);

/* Radio co-processor shown on the "Hardware information" page, for
 * boards whose Wi-Fi / Bluetooth come from a second chip (e.g. "ESP32-C6"
 * and "esp-hosted 3.1.0"). Without a call the section is left out. The
 * strings must stay valid for the life of the program. */
void editor_ui_set_coprocessor(const char *chip, const char *firmware);

/* Boards with CONFIG_DRAFTLING_DISPLAY_RESIZABLE (a window): after the
 * window changed size, the LVGL port asks editor_ui_can_relayout()
 * (false while a dialog, picker or pop-up is open -- ask again later),
 * then sets the LVGL display's new resolution and calls
 * editor_ui_relayout(), both with the LVGL lock held. */
bool editor_ui_can_relayout(void);
void editor_ui_relayout(void);

#ifdef __cplusplus
}
#endif
