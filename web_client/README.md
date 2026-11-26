# Draftling web client

The Draftling editor firmware compiled to WebAssembly, as a web
application that fills the browser window. Unlike the
[online demo](../online_demo/README.md), which simulates a device, it
works like the [Linux program](../linux/README.md): the screen is the
window, at its size in device pixels, with the high-resolution (Hack)
fonts; the documents are kept for good; Git sync and Draftbox use the
browser's network connection; copy and paste use the system clipboard.

As in the Linux program and the demo, the firmware's own code runs
unchanged -- the editor, keyboard layouts, fonts, input methods,
HarfBuzz, the SD card layer and the real Git client -- with LVGL 9.6.0.
The web client is a hardware model of its own, `DRAFTLING_MODEL_WEB`
in `firmware/main/Kconfig.projbuild` (`main/boards/web.h`), and is
built with every language (the `all` pack, as the Linux program).

## Using it

Open the site in a current Chrome, Edge, Firefox or Safari (not a
private window). Draftling then works as on a device (F1 menu, F10
help; see the user documentation in `docs/`), with these differences:

- **Storage.** Documents and settings are kept in the browser's
  storage for the site (the origin-private file system). Draftling
  starts only once the browser marks that storage as persistent, so
  the browser does not delete it when the disk runs low; the page
  explains how to get that in each browser (Chrome / Edge: install
  Draftling as an app or bookmark the page; Firefox: allow it when
  asked; Safari: add it to the Dock or Home Screen). Only one tab or
  window of a browser runs Draftling at a time. "Format SD card"
  empties the storage. Clearing the site's data in the browser
  deletes the documents: synchronize with Git or download them.
- **Files panel (F9, or "Files" at the top right).** Lists the
  projects and their documents; downloads a document, a project or
  everything as a ZIP file (documents only, not the Git history or
  `git.cfg`, which holds the access token); uploads .md, .fountain and
  .txt files into a project (the "Upload..." button, or drop the files
  on the project), asking before replacing one; creates a project.
  Opening the panel saves the open documents first. A document open
  in Draftling is reloaded after an upload replaces it.
- **Network.** Git sync and Draftbox go through the browser, so the
  Git server must allow the site with CORS headers (Draftbox does, for
  its Git and API paths). There is no WiFi menu or Ctrl+W: the
  browser's connection is used as it is.
- **Keyboard.** Keys are read as physical keys, like a US keyboard on
  a device; the layout is chosen in Draftling (Ctrl+L, F1 -> Settings
  -> Keyboard -> Active layouts), not by the computer. On a Mac,
  Command works as Ctrl. Browsers keep some shortcuts for themselves
  (Ctrl+W, Ctrl+N, Ctrl+T, Ctrl+Tab), except in full screen in Chrome
  and Edge.
- **Full screen (F11, or "Full screen" at the top right).** Chrome and
  Edge also lock the keyboard there: Esc and the browser's shortcuts
  go to Draftling, and holding Esc leaves full screen. Firefox and
  Safari leave full screen on Esc.
- **Clipboard.** Ctrl+C / Ctrl+X put the selection on the system
  clipboard, Ctrl+V pastes from it (the browser may ask for permission
  the first time). The search, file-name and other small fields do not
  paste.
- **Mouse.** The left button acts as a touch (tap, double-tap selects
  a word, drag selects); the wheel moves the cursor.
- **Leaving.** Hiding the tab saves the open documents; closing it
  with unsaved changes asks first. "Sleep now" and the standby timeout
  save and blank the screen until the next key or click. A setting
  that applies after a restart reloads the page.
- **Offline.** Once loaded, the site starts without a network
  connection (its service worker keeps the files of the current
  build).

## Toolchain installation

The same as the online demo's: the Emscripten SDK (tested with 7.0.0),
`git`, `curl`, `python3` and GNU `make`:

```bash
git clone https://github.com/emscripten-core/emsdk.git /opt/emsdk
cd /opt/emsdk && ./emsdk install 7.0.0 && ./emsdk activate 7.0.0
source /opt/emsdk/emsdk_env.sh     # in every new shell
```

The Makefile fetches LVGL `v9.6.0`, kconfiglib `v14.1.0` and miniz
`v114` into `build/` itself, at the versions the Linux program pins.

## Building

```bash
cd web_client
source /opt/emsdk/emsdk_env.sh
make -j8        # -> dist/
make serve      # build, then serve dist/ on http://localhost:8001/
```

`make clean` removes the build output, `make distclean` also the
fetched sources. `build/` and `dist/` stay out of git.

| File | Builds |
|------|--------|
| `Makefile` | fetches the dependencies, runs the two files below, assembles `dist/` |
| `lvgl.mk` | LVGL as a static library (with `-pthread`) |
| `client.mk` | the program, `draftling.js` + `draftling.wasm` |

`client.mk` generates `sdkconfig.h` from the firmware's own
`Kconfig.projbuild` files with `web_client/sdkconfig.defaults` (the
`DRAFTLING_MODEL_WEB` board) and `firmware/sdkconfig.defaults.lang_all`
(`tools/gen_sdkconfig.py`, root `tools/Kconfig.web`), reads the
`CONFIG_*` values back into make and picks the sources, font data and
input methods the way `linux/draftling.mk` does. The `all` pack's CJK
font is merged by `linux/tools/merge_cjk_fonts.py`; font and dictionary
blobs are linked in with `.incbin` (`tools/gen_embed.py`).

## Deploying

`dist/` is the whole site -- static files only:

| File | |
|------|---|
| `index.html` | the page (`make` fills in the version and the build id) |
| `client.js`, `client.css` | the page's script and styles |
| `draftling.js`, `draftling.wasm` | the program |
| `sw.js` | the service worker: isolation headers and offline cache |
| `manifest.webmanifest`, `icon.svg` | installation as an app |

The site must be served over HTTPS (or from `localhost`). The program
uses threads, which need the page to be cross-origin isolated:

    Cross-Origin-Opener-Policy: same-origin
    Cross-Origin-Embedder-Policy: require-corp

A server that sends these on every response needs nothing more. On one
that cannot (GitHub Pages and the like) the service worker adds them:
the first visit loads once without them, registers `sw.js` and reloads.
`.wasm` must be sent as `application/wasm`. `index.html` and `sw.js`
should be sent with `Cache-Control: no-cache`; every other file is
loaded with `?v=<build id>` (a hash of the build's files), so long
cache lifetimes are fine for them. `tools/serve.py` (`make serve`)
serves `dist/` this way for testing.

The Git servers the users sync with must allow the site's origin with
CORS headers (`Access-Control-Allow-Origin`, and the `Authorization`,
`Content-Type` and `Git-Protocol` request headers); Draftbox does so by
default (its `corsOrigins` setting).

## How it works

| Part | Firmware | Web client |
|------|----------|------------|
| Board settings | `sdkconfig` from the board's and the pack's defaults | `build/p/sdkconfig.h`, from `web_client/sdkconfig.defaults` and the `all` pack's defaults |
| Threads | FreeRTOS tasks | Emscripten pthreads (Web Workers); `main()` runs on one too (`PROXY_TO_PTHREAD`), so the LVGL loop and the Git client may block while the page's own thread handles the DOM. The FreeRTOS stand-in is the Linux program's (`linux/src/freertos_linux.c`) |
| Startup | `main/main.cpp` | `src/main_web.cpp`: mounts the origin-private file system (WasmFS OPFS backend) at `/opfs`, documents in `/opfs/documents` (the "SD card", the current directory), settings in `/opfs/settings`; then the Linux program's init order |
| Display | panel drivers, `lvgl_port.cpp` | the Linux program's LVGL port and main loop (`linux/src/lvgl_port_linux.c`) with `src/io_web.c` as its I/O backend: the changed rectangle goes to the page as RGBA (`Module.webPresent`), drawn on a canvas the size of the window in device pixels; a new size lays the screens out again (`editor_ui_relayout()`) |
| Input | BLE HID, touch | `web/client.js`: `KeyboardEvent.code` -> Linux key codes, pointer and wheel -> a lock-free queue (`web_input()`) the LVGL loop drains; the Linux program's keyboard layer (`keyboard_linux.c`) turns the keys into HID usages |
| Clipboard | the editor's own | also the system's: `src/io_web.c` implements the Linux backend's `clip_set` / `clip_request` over the page (`navigator.clipboard.writeText()`, Safari's `ClipboardItem` with a promise; the browser's `paste` event, else `navigator.clipboard.readText()`); `linux/src/clipboard_linux.c` hands the text to the editor |
| SD card | FAT on MicroSD | the OPFS folder; `src/sd_hw_web.c` reports the browser's quota and use (`navigator.storage.estimate()`) as the card's size |
| NVS | flash | the Linux program's `nvs_linux.cpp`: `settings.nvs` in `/opfs/settings` |
| Git sync, Draftbox | `git_sync` over `esp_http_client` | the same code over `src/http_client_web.c`: one synchronous `XMLHttpRequest` per request, allowed on the worker thread the sync task runs on; the browser does TLS and decompression. SHA-1 in `src/sha1_web.c`, zlib from miniz |
| Files panel | -- | `web/client.js` and `src/files_web.c`: requests run on the LVGL loop, with the LVGL lock held; ZIP files with miniz; uploads reload open documents through `editor_ui_files_changed()` |
| Standby | deep sleep | the Linux program's `standby_linux.c` (`DRAFTLING_STANDBY_DISPLAY_OFF`): save, then blank the canvas until input; the page asks the program to save when it is hidden (`WEB_EV_SAVE`) |
| Network | `wifi_manager` | `linux/src/wifi_host.c` (`DRAFTLING_NETWORK_HOST_MANAGED`): no WiFi control |
| Restart | `esp_restart()` | reloads the page |

## When the firmware changes

As for the Linux program: a new ESP-IDF / FreeRTOS call in the editor
or the Git client needs a declaration in `linux/shim/include/` (shared;
`web_client/shim/include/` overrides it where the browser differs) and
a stand-in; a new function in the API of `wifi_manager`, `ble_keyboard`,
`standby`, `battery` or `display` needs it in the shared
`linux/src/*_linux.*` file the web client compiles (see `LINUX_SRCS` in
`client.mk`) or in `src/`; a new source or data file in
`components/fonts`, `components/ime` or `components/git_sync` needs its
selection in `client.mk`. Check that `make` in `web_client/` still
builds.
