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

### Changed

- **Crash-safe file writes.** Saving a document (and its cursor/scroll
  sidecar, and files written by Git sync) now writes to a temporary
  file that is flushed to the MicroSD card and then renamed over the
  target. A power loss or a card pulled out mid-save can no longer
  truncate or corrupt the file being written -- the previous contents
  stay intact until the complete new version is in place.

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
