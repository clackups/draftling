# Menu development

## Current architecture

The F1 menu remains a flat LVGL list. Its stable `menu_item_id_t` values
are declared in `include/editor_menu.h`; visible row order is defined in
`editor_menu.cpp` by `s_menu_row_ids`. `editor_ui.cpp` supplies the dynamic
labels, hotkeys, activation handlers, and touch behavior. The list renderer
and menu selection state are still owned by `editor_ui.cpp`.

The Settings screen is also flat. Its setting IDs, feature-gated row list,
label/value callbacks, and action/cycle callbacks are in `editor_ui.cpp`.
Existing persistence and hardware behavior stays in those callbacks and in
the existing component APIs.

## Current F1 order

The menu rows appear in this order:

1. Settings
2. BLE connection status
3. BLE: Start scan
4. WiFi connection status / Connect
5. WiFi: New connection
6. WiFi: Disconnect
7. Git Sync
8. Connect a Draftbox repository
9. Keyboard layout
10. SD card via USB (only with `CONFIG_DRAFTLING_HAS_USB_MSC`)
11. Sync via browser (only with `CONFIG_DRAFTLING_HAS_KERMIT`)
12. Format SD card
13. Hardware information
14. Help
15. Sleep now
16. Close menu

Rows 10 and 11 are omitted when their feature is unavailable. Their stable
IDs do not shift when omitted.

## Current Settings order

The flat Settings rows are: standby timeout, base font size, maximum file
size, backlight when adjustable backlight is supported, color theme on color
displays, live rotation where supported, inversion where supported,
append-only editing, active keyboard layouts, display orientation, display
upside down where the live rotation control is absent, 2x scale, left/right/
top/bottom screen margins, factory reset, and Back.

Backlight, theme, rotation, inversion, and upside-down rows are conditionally
compiled. Do not remove their existing conditions or change their callbacks'
NVS keys, ranges, defaults, confirmation prompts, or restart behavior.

## Adding a flat F1 item

1. Add a unique, explicit `MENU_ID_*` value in `include/editor_menu.h`.
   Do not derive its value from another ID or the current row order.
2. Add the ID to `s_menu_row_ids` in `editor_menu.cpp` at the intended
   position. Keep any hardware feature guard around the row.
3. Add the matching label at the same position in `refresh_menu_items()` in
   `editor_ui.cpp`. Keep its label synchronized with any hotkey character
   position in `s_menu_hotkeys`.
4. Add or update the `MENU_ID_*` branch in `menu_activate_item()`.
5. Add a regression assertion to `tests/editor_menu_test.cpp` and run the
   host test matrix described in `tests/README.md`.

The row table and label list are separate today; changing either alone can
misalign displayed labels and actions. The host tests verify the ID table but
do not currently verify the LVGL labels or action dispatch.

## Adding or moving a setting

1. Add a `SETTINGS_ID_*` entry in the settings ID definition in
   `editor_ui.cpp`; preserve existing NVS and value semantics.
2. Add the ID to `s_settings_row_ids` in its desired position and add its
   registry entry to `s_settings_registry`.
3. Supply the label/value callback and, as appropriate, a cycle or activation
   callback. Reuse the current component and persistence APIs.
4. Add the same feature condition to the ID, row, registry entry, and any
   supporting handler so unsupported boards do not expose the setting.
5. Check the setting's keyboard/touch interaction, value limits, persistence,
   restart/confirmation behavior, and relevant board builds.

The Settings IDs still use feature-dependent enum arithmetic and compatibility
index aliases. Do not treat them as independent stable numeric identities or
reorder their enum values in this transitional implementation.

## Submenus and known limitations

Nested submenus are not implemented yet. There is no parent/child descriptor
tree or bounded navigation stack, so a submenu cannot currently be added by
editing a declarative definition alone. Implementing one safely requires
extracting label/action definitions, rendering, and keyboard/touch navigation
from `editor_ui.cpp`, then adding hierarchy and navigation tests. Until then,
retain existing flat-menu behavior rather than adding a one-off submenu path.

The host regression matrix currently validates ID order, uniqueness,
feature-gated visibility, ID/index round trips, invalid lookups, and selection
wrapping for USB MSC and Kermit build combinations. It does not validate the
full firmware, LVGL interactions, settings persistence, or hardware behavior.
