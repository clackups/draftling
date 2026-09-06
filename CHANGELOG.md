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
  unsaved changes it sleeps immediately.

### Changed

- **"Sleep now" moved from F1 -> Settings to the top-level F1 menu**, and
  now prompts about unsaved changes just like Ctrl+P.
- A user-initiated sleep (Ctrl+P, "Sleep now") that chooses "Sleep
  without saving" genuinely discards the unsaved edits. An *automatic*
  sleep on the inactivity timeout still saves everything first, as
  before.

### Fixed

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
