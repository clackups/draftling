# Changelog

All notable user-facing changes to Draftling are recorded here, starting
from the first published release (`v1.0.1`). Older history is available
in the git log.

## [Unreleased]

### Added

- **Delete files from the file browser**: `Del` or `Alt+D` deletes the
  selected file after a confirmation. To prevent accidental loss this
  only works for a file whose current content is already committed
  and pushed to the Git server, so it can always be recovered from the
  repository history; otherwise the status bar says to sync first.
- **F2 renames files** in the file browser, the same as `Alt+R`.
- **Built-in help**: `F10` (or the new "Help" item in the `F1` menu)
  opens a page listing the shortcuts of the current screen -- the
  editor or the file browser. In the editor it also lists the
  formatting syntax of the open document: Markdown or Fountain. Scroll
  with Up/Down, PgUp/PgDn, Home/End; `Esc` or `Enter` closes it.

### Fixed

- **Battery level missing while waiting for a keyboard**: the battery
  indicator on the start-up "searching for keyboard" screen stayed
  empty until the keyboard connected. It now shows the level right
  away (and no longer goes blank after changing the color theme).
- **Long file names broke the editor's title bar**: a name too long
  for the screen wrapped onto a second line. It is now shortened with
  an ellipsis so the line and column counters stay visible. The
  ellipsis character is also displayed properly in documents now.

## [1.0.7] - 2026-09-24

### Added

- **Plain text files**: files ending in `.txt` now show up in the file
  browser and open with no formatting at all -- no hidden markers,
  headings or bullets, just the text as typed. Git sync includes
  `*.txt` files alongside `*.md` and `*.fountain`.
- **Choose the format of a new file**: `Ctrl+N` (in the editor or the
  file browser) and `N` in the file browser now ask whether the new
  file is Markdown, a Fountain screenplay or plain text. Pick with
  Up/Down and Enter, or press M, F or T.
- **Rename files**: `Alt+R` in the file browser renames the selected
  file. Its saved cursor position moves with it, and a file that is
  open in the other split pane keeps editing under the new name.
  Changing the extension (e.g. `.md` to `.txt`) changes how the file
  is displayed.

- **Fountain screenplays**: files ending in `.fountain` open in a
  screenplay mode that formats the script as you type, following the
  [Fountain](https://fountain.io/syntax/) rules: bold scene headings,
  indented character names, dialogue and parentheticals, right-aligned
  transitions, centered text, emphasis including `_underline_`, notes,
  boneyard, sections, synopses, page breaks and the title page. Pick
  "Fountain screenplay" when creating a new file to start one, and
  `Ctrl+S` offers a `draft_NNN.fountain` name for it (draft numbers
  continue across `.md`, `.fountain` and `.txt` drafts). Enter after a scene heading or
  transition adds the blank line that follows it, and Tab completes
  character names and scene headings already used in the script. In
  split mode each pane keeps its own format. Git sync now includes
  `*.fountain` files alongside `*.md`.
- **WYSIWYG Markdown rendering**: the editor now displays Markdown
  formatting instead of plain text. **Bold**, *italic*, ***bold
  italic***, ~~strikethrough~~ and `inline code` (boxed) are drawn
  styled; headings are bold; list items get drawn bullets (a different
  shape per nesting level); `---` becomes a horizontal rule. The
  markup characters (`**`, `*`, `_`, `~~`, backticks, `#`, `>`,
  ```` ``` ````) are hidden, except on the line holding the cursor,
  which keeps them visible so they can be edited (its styling is still
  shown). Numbered lists show their numbers again. Inline parsing is
  stricter and closer to CommonMark: `_` / `__` / `***` delimiters,
  nested styles and backslash escapes are supported, while
  `2 * 3 * 4` and `snake_case_names` stay plain text. Leading
  indentation of paragraphs and list items is shown as typed, and tabs
  display as four spaces.
- **WiFi network picker (F1 menu)**: "WiFi: New connection..." scans
  for visible networks and lets you pick one from a list; a secured
  network asks for its password first (the typed text is shown, not
  masked). A successful connection is saved to `/sdcard/wifi.cfg`, so
  `Ctrl+W` and the next boot reconnect to it. "WiFi: Connect to
  <SSID>" keeps the quick reconnect to the saved network. The status
  bar now reports the attempt's outcome: connected, "wrong password",
  or failed.
- **Seeed Studio reTerminal E1001 support**: 7.5-inch 800x480 e-paper
  (UC8179) with fast partial updates (~450 ms) and periodic full
  refreshes, on-board MicroSD. Holding KEY0 for 2 seconds forgets all
  paired BLE keyboards. USB mass storage is not available on this
  board (its USB port goes through a UART bridge chip).

### Fixed

- **Dropped characters on e-paper boards when typing fast**: when
  several keystrokes were processed between two panel refreshes, only
  the last one's area was refreshed, leaving blank gaps in freshly
  typed text. Affected every e-paper board.
- **WiFi: `Ctrl+W` stopped working after a failed connection attempt**
  until a reboot. Reconnecting now works right away. Rejected
  credentials fail immediately instead of being retried; other errors,
  such as a weak signal, are still retried.
- **File browser, F1 menu and Settings lists overlapped the title bar**
  on high-density boards (e.g. the reTerminal E1001).
- **Cursor drawn in the middle of a letter on blockquote lines** (and
  offset on lines inside code blocks); taps on those lines were
  slightly off too.

## [1.0.6] - 2026-09-19

### Added

- **IPv4/IPv6 dual stack**: the device now brings up IPv6 alongside
  IPv4 on the WiFi station interface. When the network advertises a
  global IPv6 prefix (SLAAC router advertisements), the WiFi status
  icon in the editor and file browser status bars switches to a
  "6"-badged variant, and Git sync tries the server's `AAAA` DNS
  record first, falling back to `A` (IPv4) if the connection attempt
  fails or the server has no AAAA record.
- **"SD card via USB" (F1 menu)**: on boards whose USB port wires the
  ESP32-S3's native USB-OTG controller straight to the connector
  (Xteink X4 Pro / Classic, LilyGO T5 E-Paper S3 Pro / Pro Lite /
  H752, M5Stack PaperS3, Waveshare ESP32-S3-ePaper-3.97,
  ESP32-S3-RLCD-4.2, ESP32-S3-Touch-LCD-7 and ESP32-S3-Touch-LCD-3.49,
  and all three Freenove FNK0104 boards -- confirmed working on
  physical hardware for every one of these boards), a new F1 menu
  item opens a picker for Off / Read-only / Read-write; the choice
  takes effect as soon as you leave that picker (Enter to confirm, or
  Esc to back out unchanged), not while you are still choosing.
  Turning it on exposes the SD card to a connected computer as a
  normal USB mass-storage drive;
  editing, creating files, and Git sync are all refused until it is
  switched back off (both from the file browser and from the F1 menu
  itself), switching between Read-only and Read-write while already
  connected forces a clean USB disconnect/reconnect so the computer
  re-reads the new write permission instead of keeping the one it saw
  at first mount, and it switches itself back off automatically if no
  USB host has been connected for 5 minutes (configurable), so
  leaving it on by mistake cannot lock the device out of its own SD
  card indefinitely. Turning it off (manually or via that automatic
  timeout) restarts the device: the ESP32-S3's native USB pins are
  shared between the OTG controller this feature uses and the
  separate USB-Serial-JTAG controller boards without a UART bridge
  chip rely on for flashing and the console, and handing the pins
  back at runtime is not enough by itself to reliably restore
  flashing on every board, so a restart -- the same fix a physical
  reset button press provides -- runs as a safety net every time. The
  menu item is disabled ("no SD card") and does nothing when picked
  if no card is mounted; once a session is running it stays available
  regardless of card state, so a card pulled out mid-session can
  still be switched back off. M5Stack Tab5 was evaluated (its USB-C
  charging port sits on a USB-OTG controller separate from the one
  the wired-keyboard feature already uses in host mode on its USB-A
  port) but does not work there in practice, so it is not on this
  list.

### Fixed

- **WiFi: editing `wifi.cfg` on the SD card now takes effect.**
  Previously `wifi_manager_connect()` only read the file the very
  first time (to seed NVS); every later `Ctrl+W` reconnect used the
  cached NVS credentials and ignored the file entirely, so changing
  the SSID or password on the card had no effect until NVS was wiped
  by some other means. The file is now read on every connect attempt,
  and a mismatch against the cached NVS credentials (new SSID, or a
  changed password for the same one) replaces them before connecting.
- **Git sync: crash on a brand-new sync against an empty remote with
  no local files.** Syncing an empty repository for the first time
  (no commits on the remote, no `*.md` files on the SD card) crashed
  the firmware (`LoadProhibited` at address 0) instead of completing
  as a no-op. The crash was a `NULL` pointer read in the "which files
  changed" bookkeeping, reached only when both sides start out empty.
- **Git sync: pushing to an empty (branch-less) remote never actually
  pushed anything.** The push-trigger check compared the local commit
  against itself instead of against the remote in this case, so it was
  always trivially "nothing changed" and the block never ran -- local
  commits synced to the SD card's own `.git` history but never reached
  the server, silently, with no error shown.
- **Git sync: commit timestamps could drift minutes into the future
  after the device's first-ever boot.** The wall-clock SNTP sync
  skipped itself whenever `time(NULL)` already read a "plausible"
  post-2025 date -- but ESP-IDF's system clock keeps ticking through
  deep sleep via the RTC timer domain (the internal RC oscillator on
  boards with no external 32kHz crystal, which drifts), so it always
  looked plausible after the very first boot and the real correction
  never ran again. SNTP is now attempted once every boot regardless,
  and the query now goes to `2.pool.ntp.org`.

## [1.0.5] - 2026-09-16

### Added

- **Display upside down** setting (F1 -> Settings): turns the whole
  UI 180 degrees, for boards or enclosures that can be mounted either
  way up. Persisted in NVS and applied on the next restart, which is
  offered right when you leave the Settings screen. Boards that
  already expose a live-apply "Rotate 180" toggle keep using that
  instead (no duplicate setting).
- **18px and 22px base font size** options (F1 -> Settings -> "Base
  font size") on non-HIDPI (Greybeard font) boards, alongside the
  existing 11/14/16px. Two new Greybeard heading slots (30px, 34px)
  back their H1/H2 headings.
- **Deep sleep from the rocker button** on the Waveshare
  ESP32-S3-ePaper-3.97: holding the Up/Function/Down rocker's center
  (Function) press for 2 seconds now enters deep sleep, alongside its
  existing short-press Enter injection. Exposed as a generic Kconfig
  option (`DRAFTLING_SLEEP_BUTTON_GPIO`) other boards can opt into.

### Changed

- **Faster e-paper refresh on the Elecrow CrowPanel 5.79"**: full
  refresh now uses the same fast differential waveform as partial
  refresh (matching the Waveshare ESP32-S3-ePaper-3.97) instead of the
  slow flood-fill waveform, and per-keystroke partial refresh fires a
  single activation trigger instead of two. User-tested on real
  hardware as noticeably faster and smoother typing.

### Fixed

- **Screen margin settings ("Margin left" / "Margin top") moved the
  wrong edge of the screen** on every board except the Waveshare
  ESP32-S3-ePaper-3.97 and the Xteink e-paper boards -- e.g. "Margin
  left" visibly shrank the *right* margin on the Waveshare RLCD-4.2.
  The left/top offset is now applied consistently across all display
  backends.
- **Dropped scanlines in the 18px, 22px and 26px Greybeard fonts**:
  freetype's autohinter was dropping rows out of glyphs (a blank row
  mid-stroke on diagonals like "V"/"X", a missing row on "O"/"Q"'s
  ring, etc.) when rendering at exactly these sizes. Regenerated with
  autohinting disabled; only cosmetic at 22px, unnoticed before now
  because 18px only backed headings until this release added it as a
  body size.

## [1.0.4] - 2026-09-11

### Fixed

- **E-paper boards in portrait orientation stopped updating while
  typing** (reproduced on the Waveshare ESP32-S3-ePaper-3.97 and
  Xteink X4 Pro): the fast partial-refresh path clipped the panel
  refresh to a rectangle computed in LVGL's logical, pre-rotation
  coordinates, but the panel driver intersects that clip against a
  dirty region tracked in physical (post-rotation) coordinates. In
  portrait the two spaces don't line up, so the clip usually
  intersected to nothing and every keystroke's refresh was silently
  dropped -- new text was written to the off-screen framebuffer but
  never reached the panel until a full refresh (Ctrl+R) redrew
  everything at once. The clip rectangle is now mapped through the
  same rotation transform the flush path already applies to pixel
  data.

## [1.0.3] - 2026-09-11

### Added

- **Waveshare ESP32-S3-ePaper-3.97** board support: a 3.97" 800x480
  e-paper board with an SSD1677-family panel controller, an AXP2101
  PMIC (battery monitoring, and switching the panel's own analog
  supply rail), no touchscreen (four buttons drive editor navigation
  instead), and on-board MicroSD over SDMMC. Build it with `idf.py
  --preset waveshare_epaper_397`.
- **Xteink X4 Classic** (also sold as "X4 v2") support: the
  buttons-only, no-front-light sibling of the X4 Pro. Shares the X4
  Pro's ESP32-S3, 800x480 e-paper panel (SSD1677 / UC8179 / UC8279,
  auto-detected), CW2017 fuel gauge and SDMMC card slot. Eight buttons
  drive the whole editor without a keyboard: Power = F1 / hold to
  forget keyboards, two side keys = Up/Down, four bottom keys =
  Left/Right/Enter/Esc. Added without on-hardware testing (pin map
  from the FreeInk SDK); build it with `idf.py --preset
  xteink_x4_classic`.
- **Ctrl+ArrowDown / Ctrl+ArrowUp** as equivalents to Page Down / Page
  Up.

### Changed

- Page Up/Down (and the new Ctrl+Down/Up) now jump by however many
  lines actually fit on screen instead of a fixed estimate, so they no
  longer overshoot past the end of a document made of long,
  word-wrapped lines.
- Scrolling at the edge of the view -- typing or navigating off the
  bottom/top of the screen, including mid-paragraph word-wrap -- now
  jumps by about 60% of the screen instead of one line at a time, so
  continuous typing/navigation near the edge needs far fewer redraws.
  Especially noticeable on e-paper, where every redraw is a visible,
  non-instant refresh.

### Fixed

- **E-paper full refresh no longer leaves black text visibly faded**
  on SSD1677-family panels (Waveshare ESP32-S3-ePaper-3.97, Xteink X4
  Pro / Classic): full refresh now uses the same differential waveform
  already used for partial refreshes, which reaches true black
  cleanly.
- **Fewer unnecessary full e-paper refreshes** on the Waveshare
  ESP32-S3-ePaper-3.97: the check for "is this edit too big for a fast
  partial refresh" now measures how much content actually changed,
  instead of the bounding box between two small, far-apart changes
  (e.g. the title bar and the cursor), which could by itself span most
  of the screen.
- **Selecting part of a word-wrapped line across the wrap point**
  (e.g. Shift+ArrowUp, or several Ctrl+Shift+ArrowLeft, from the end
  of a long line) no longer highlights the whole line as if it were
  fully selected. Backspace was previously deleting only the true,
  smaller selection, leaving the apparently-selected beginning of the
  line behind.

## [1.0.2] - 2026-09-06

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
  browser: enabling a split switches to the editor so it is visible
  right away, and `Ctrl+1` collapses a split from anywhere (including
  the file selector a split editor shows on Esc).
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

- **Touch input was a quarter turn off** on any board rendering at a 90
  or 270 degree rotation -- the natively-portrait Freenove FNK0104B /
  FNK0104S, and any board in the new portrait orientation. The LVGL
  display rotation fed to the touch-point transform did not match the
  sense of the framebuffer rotation; taps now land under the finger.
- **Xteink X4 Pro**: the touchscreen no longer comes up dead after a
  restart from the Settings "restart to apply" prompt (or a panic /
  watchdog). A warm reboot does not reset the GT911, which then came
  back stuck at its fallback I2C address and never scanning; the
  controller is now fully power-cycled and hardware-reset with the
  datasheet address-select timing early in boot, before the shared I2C
  bus is created.
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
