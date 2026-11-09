# Menu development

## Current architecture

The F1 menu is a static parent/child hierarchy rendered as one LVGL list.
Stable `menu_item_id_t` values are declared in `include/editor_menu.h`;
each item's parent and display order are defined by the descriptor table in
`editor_menu.cpp`. `editor_ui.cpp` supplies dynamic labels, hotkeys,
activation handlers, and LVGL touch integration. A bounded four-entry
navigation stack tracks the root and nested categories; keyboard and touch
activate the same selected row, and Escape returns to the parent before
closing at the root.

Settings uses the same static parent/child model, with stable IDs and
feature-gated row order in `editor_settings.cpp`. `editor_ui.cpp` provides
dynamic labels and reuses the existing action/cycle callbacks. The nested
screen uses Escape to return to its parent, with a bounded path of
Settings -> Display -> Screen margins. Existing persistence and hardware
behavior stays in the callbacks and component APIs.

## Current F1 hierarchy

```text
F1
|- Settings
|- Connectivity
|  |- Bluetooth
|  |  |- Connection status
|  |  `- Scan for keyboard
|  |- WiFi
|  |  |- Connection status
|  |  |- Connect
|  |  |- Choose network
|  |  `- Disconnect
|  |- Git Sync
|  |- Connect Draftbox repository
|  `- Sync via browser (CONFIG_DRAFTLING_HAS_KERMIT)
|- Writing
|  |- Keyboard layout
|  `- Project information
|- Storage
|  |- SD card via USB (CONFIG_DRAFTLING_HAS_USB_MSC)
|  `- Format SD card
|- System
|  |- Hardware information
|  |- Help
|  `- Sleep now
`- Close menu
```

Unsupported USB MSC and Kermit rows are omitted at compile time. Their stable
IDs remain fixed across configurations. Project information reuses the
existing project-info view and reports when there is no selected project.

## Settings hierarchy

```text
Settings
|- Editor
|  |- Base font size
|  |- Append-only editing
|  `- Maximum file size (read-only)
|- Display
|  |- Brightness / backlight (when adjustable)
|  |- Color theme (color displays)
|  |- Orientation
|  |- Rotate 180 (the live or restart-applied control supported by the board)
|  |- Invert display (when supported)
|  |- 2x scale
|  `- Screen margins
|     |- Left
|     |- Right
|     |- Top
|     `- Bottom
|- Keyboard
|  `- Active layouts
|- Power
|  `- Standby timeout
`- System
   `- Factory reset
```

The maximum file-size row remains visible and read-only. Backlight, color
theme, live rotation, inversion, and restart-applied 180-degree rotation
follow their existing board feature conditions. Escape returns to the
previous Settings category; at the Settings root it returns to the F1 menu.
Do not change existing callbacks' NVS keys, ranges, defaults, confirmation
prompts, or restart behavior when moving a setting.

## Adding or moving an F1 item

1. Add a unique, explicit `MENU_ID_*` value in `include/editor_menu.h`.
   Do not derive its value from another ID or the current row order.
2. Add its descriptor to `s_menu_items` in `editor_menu.cpp`, using the
   desired parent ID. Moving an existing entry only requires changing this
   parent.
3. For a new leaf, add its dynamic label case to `menu_label_for_id()` and
   its behavior to `menu_activate_item()` in `editor_ui.cpp`. Add a hotkey
   only when appropriate, and ensure the key remains available from its
   submenu.
4. For a new submenu, give it a stable ID and add child descriptors. The
   shared navigation code detects containers from their child count; no
   separate renderer or key handler is needed.
5. Add a regression assertion to `tests/editor_menu_test.cpp` and run the
   host test matrix described in `tests/README.md`.

The host tests verify the hierarchy and feature conditions but do not
currently verify the LVGL labels, touch interaction, or action dispatch.

## Adding or moving a setting

1. Add a unique, explicit `SETTINGS_ID_*` value in
   `include/editor_settings.h`; do not put feature guards around the ID
   declaration or change existing NVS/value semantics.
2. Add the feature-gated parent/child descriptor to `editor_settings.cpp`
   and add its label/action entry to `s_settings_registry`.
3. Supply the label/value callback and, as appropriate, a cycle or activation
   callback. Reuse the current component and persistence APIs.
4. Add the same feature condition to the ID, row, registry entry, and any
   supporting handler so unsupported boards do not expose the setting.
5. Check the setting's keyboard/touch interaction, value limits, persistence,
   restart/confirmation behavior, and relevant board builds.

Settings IDs are explicit and feature-independent in
`include/editor_settings.h`. Feature guards belong on the visible settings
rows and registry entries, not around the ID declarations. Tests verify that
the IDs remain unique and unchanged across the host feature matrix.

## Known limitations

The descriptor tables record stable IDs, parent, order, and static titles,
while specialized action behavior and dynamic labels remain in `editor_ui.cpp`.
The host regression matrix validates F1 and Settings hierarchy, feature-gated
visibility, stable IDs, and lookups. It does not validate the full firmware,
LVGL interactions, settings persistence, or hardware behavior.
