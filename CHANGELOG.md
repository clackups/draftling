# Changelog

All notable user-facing changes to Draftling are recorded here, starting
from the first published release (`v1.0.1`). Older history is available
in the git log.

## [Unreleased]

### Added

- **Active language layouts** setting (F1 -> Settings -> "Active
  layouts"): choose which of the compiled-in keyboard layouts Ctrl+L /
  Win+Space rotate through, instead of always cycling every layout
  enabled in the firmware build. Defaults to US and UA. The title bar
  only shows the current layout's name when more than one layout is
  active, since there is nothing to switch to otherwise. In the picker,
  Space toggles the highlighted layout the same as Enter.
- All five keyboard layouts (US, UA, DE, FR, HE) are now enabled by
  default in `idf.py menuconfig` (`DRAFTLING Keyboard Layouts`);
  previously only US and UA were on by default.
- **Ctrl+P puts the device into deep sleep.** If the open document has
  unsaved changes the editor first asks: **Save and sleep**, **Sleep
  without saving** (the edits are dropped), or **Cancel**. With no
  unsaved changes it sleeps immediately. `Ctrl+P` and the split
  shortcuts `Ctrl+1` / `Ctrl+2` / `Ctrl+3` now also work from the file
  browser (a split set there shows on the next file open).
- **Portrait display orientation** (F1 -> Settings -> "Display
  orientation": Landscape / Portrait). Portrait turns the whole UI a
  quarter turn; split-screen editing then divides the screen top/bottom
  instead of left/right, since the split always follows the display's
  long side. Takes effect after a restart (the editor offers one on the
  way out of Settings), the same as the screen margins. On the Xteink
  X4 Pro portrait turns the opposite way from the other boards, to suit
  the enclosure.

### Changed

- **"Sleep now" moved from F1 -> Settings to the top-level F1 menu**, and
  now prompts about unsaved changes just like Ctrl+P.
- A user-initiated sleep (Ctrl+P, "Sleep now") that chooses "Sleep
  without saving" genuinely discards the unsaved edits. An *automatic*
  sleep on the inactivity timeout still saves everything first, as
  before.
- **Crash-safe file writes.** Saving a document (and its cursor/scroll
  sidecar, and files written by Git sync) now writes to a temporary
  file that is flushed to the MicroSD card and then renamed over the
  target. A power loss or a card pulled out mid-save can no longer
  truncate or corrupt the file being written -- the previous contents
  stay intact until the complete new version is in place.

### Fixed

- **Xteink X4 Pro**: the flashed partition table now keeps an `otadata`
  partition and a dual-OTA app layout (`ota_0` / `ota_1`), instead of a
  single `factory` partition. The stock Xteink firmware and the
  third-party Crosspoint firmware are installed by OTA updaters that
  abort with "Partition table has no otadata partition" against the old
  layout; with this change the device can be returned to stock or moved
  to Crosspoint without re-flashing a partition table over USB first.
  (Re-flashing Draftling on an X4 Pro that already runs it clears the
  BLE keyboard pairings once, because the NVS partition is resized.)
- Ctrl-letter shortcuts (and the file browser's unmodified "N: New
  file" key) no longer silently stop working when a non-Latin layout
  (Ukrainian, Hebrew) is active -- they now resolve to the same US
  physical key position as if the US layout were selected. Under a
  Latin layout (German, French) they instead resolve to whatever
  letter that physical key produces on the national layout, even when
  that letter has moved off the classic US letter-key block: Ctrl+Z
  lands on the key printed "Z" on a German (QWERTZ) keyboard, and
  Ctrl+M on a French (AZERTY) keyboard is the semicolon key, since
  AZERTY moves "m" there.

## [1.0.1] - first release

Baseline release. See git history up to tag `v1.0.1` for everything
included.
