#pragma once

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

#ifdef __cplusplus
}
#endif
