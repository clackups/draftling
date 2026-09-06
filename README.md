# Draftling Web Flasher

This branch hosts a static, dependency-free web page that flashes
prebuilt [Draftling](https://github.com/clackups/draftling) firmware
onto supported boards directly from the browser, using the
[Web Serial API](https://developer.mozilla.org/en-US/docs/Web/API/Web_Serial_API)
and [esptool-js](https://github.com/espressif/esptool-js).

It is unrelated to the firmware source tree on `main` — this branch
exists only to be published via GitHub Pages.

## Layout

- `index.html`, `style.css`, `app.js` — the flasher UI and logic.
  `app.js` imports `esptool-js`'s prebuilt `bundle.js` from
  [jsdelivr](https://www.jsdelivr.com/), and `spark-md5` from
  [esm.sh](https://esm.sh), as ES modules; there is no build step.

  `esptool-js` must be loaded via its `bundle.js`, not a generic ESM
  CDN (e.g. `esm.sh/esptool-js`): the library selects its stub-flasher
  payload for the connected chip via a runtime `import()` of one of
  several JSON files, and on-the-fly ESM repackagers can corrupt or
  mis-resolve that dynamic JSON import, breaking the base64 decode with
  an `atob` error as soon as flashing starts. `bundle.js` is built with
  `inlineDynamicImports`, so every chip's stub data is embedded as a
  plain JS object at publish time and no runtime JSON loading happens
  at all.
- `manifest.json` — the list of supported boards. Each board has one
  or more `releases` (newest first), and each release carries its own
  flash parameters (chip family, flash mode/freq/size) and the
  firmware files/offsets to write for that board at that version. A
  board's releases don't need to line up with any other board's --
  each is added independently as its firmware gets built and uploaded.
  In the UI, picking a board populates a second dropdown with that
  board's `releases` (defaulting to the first / newest entry).
- `firmware/<tag>/draftling-<board>[-bootloader|-partition-table].bin`
  — the bootloader, partition table, and application binaries for
  each board, named and grouped by the release tag they were built
  from (e.g. [v1.0.2](https://github.com/clackups/draftling/releases/tag/v1.0.2)).
  These mirror the assets attached to the GitHub release of the same
  name, and are served same-origin so the browser can `fetch()` them
  without running into CORS restrictions on GitHub release assets.

## Updating firmware

To publish a new release through the flasher, for whichever boards it
covers:

1. Build the firmware for each board (`idf.py --preset <board> build`,
   see [BUILDING.md](https://github.com/clackups/draftling/blob/main/BUILDING.md)).
2. Create `firmware/<new-tag>/` (if it doesn't already exist) and copy
   in, per board: `draftling-<board>-bootloader.bin` (from
   `bootloader/bootloader.bin`), `draftling-<board>-partition-table.bin`
   (from `partition_table/partition-table.bin`), and
   `draftling-<board>.bin` (from `draftling.bin`).
3. For each board included in this release, prepend a new entry to the
   front of that board's `releases` array in `manifest.json` (so it
   becomes the default) with its `tag`, flash mode/freq/size, and
   `parts` pointing at `firmware/<new-tag>/...`. Boards not part of
   this release keep their existing `releases` untouched.
4. Commit and push to `_flasher` — GitHub Pages redeploys automatically.
   A `firmware/<tag>/` directory can be removed once no board's
   `releases` references it any more.

## Adding a board

Add an entry to `manifest.json` with the board's chip family and a
`releases` array with one entry (tag, flash mode/frequency/size, and
its three binaries under `firmware/<tag>/`, named `draftling-<id>*.bin`
as above). No changes to `app.js` are needed.
