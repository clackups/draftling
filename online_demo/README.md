# Draftling online demo

The Draftling editor firmware compiled to WebAssembly, running in a web
page on a simulated device. It is published at
https://demo.draftling.art/.

The visitor picks the screen and the language pack; each combination
is the firmware image that device would run:

| Screen | Simulated board |
|--------|-----------------|
| 4.0" color LCD | **Freenove FNK0104S** (ESP32-S3, ST7796 480x320 color LCD, FT6336U touch) |
| 3.97" e-paper | **Seeed reTerminal Sticky** (ESP32-S3, SSD1677 800x480 e-paper, GT911 touch) |

Language packs: `default` (European layouts), `vietnamese`, `korean`,
`japanese`, `chinese`, `indic` -- the same packs as
`idf.py --lang-pack`, with their fonts, input methods and (Indic)
HarfBuzz shaping.

The demo runs the firmware's own code unchanged -- the editor
(`components/editor`), keyboard layouts (`components/kb_layout`),
fonts (`components/fonts`), input methods (`components/ime`), HarfBuzz
(`components/harfbuzz`), the SD card file layer
(`components/sd_card/sd_card.cpp`) and the display settings
(`components/display/display_*.cpp`) -- with LVGL 9.6.0, the version
the ESP-IDF component manager resolves for the firmware. The hardware,
the network and FreeRTOS are replaced by small stand-ins in `src/`.

## Toolchain installation

The build needs the Emscripten SDK, plus `git`, `curl`, `python3` and
GNU `make` (present on most Linux systems; nothing needs `pip`).

These are the steps used on the build host (Linux, installing into
`/opt/emsdk`; any directory works):

```bash
# 1. Get the Emscripten SDK manager
git clone https://github.com/emscripten-core/emsdk.git /opt/emsdk
cd /opt/emsdk

# 2. Install and activate the SDK. The demo was built and tested with
#    Emscripten 7.0.0; "latest" also works.
./emsdk install 7.0.0
./emsdk activate 7.0.0

# 3. Put emcc / em++ on PATH -- in every new shell, or add this line
#    to ~/.bashrc
source /opt/emsdk/emsdk_env.sh

# 4. Check
emcc --version
```

To update later: `cd /opt/emsdk && git pull && ./emsdk install latest
&& ./emsdk activate latest`.

The Makefile fetches the other two dependencies itself, at pinned
versions, into `build/`: LVGL (`git clone`, tag `v9.6.0`) and
kconfiglib (`curl`, one Python file, tag `v14.1.0`).

## Building

```bash
cd online_demo
source /opt/emsdk/emsdk_env.sh
make -j8        # every screen x language pack -> dist/
make serve      # build, then serve dist/ on http://localhost:8000/
```

The full build is 12 firmware variants (2 boards x 6 packs). For
quicker work, build only some; the page offers only the variants found
in `dist/fw/`:

```bash
make -j8 BOARDS=seeed_reterminal_sticky PACKS="default japanese"
```

`make clean` removes the build output, `make distclean` also the
fetched sources. Everything in `build/` and `dist/` -- the compiled
`draftling.js` / `draftling.wasm` included -- stays out of git
(`.gitignore`).

| File | Builds |
|------|--------|
| `Makefile` | fetches LVGL and kconfiglib, runs the two files below, copies `web/` and writes `dist/variants.js` (the variants built) |
| `lvgl.mk` | LVGL as one static library shared by every variant |
| `variant.mk` | one `BOARD` x `PACK` variant into `dist/fw/<board>/<pack>/` |

`variant.mk` generates the variant's `sdkconfig.h` from the firmware's
own `Kconfig.projbuild` files with the board's
`sdkconfig.defaults.<board>` and the pack's
`sdkconfig.defaults.lang_<pack>` (`tools/gen_sdkconfig.py`,
kconfiglib), reads the `CONFIG_*` values back into make, and picks the
font sets, font data, input methods and HarfBuzz the way the
firmware's component `CMakeLists.txt` files do. Font and dictionary
blobs (ESP-IDF `EMBED_FILES`) are linked in through an assembler file
of `.incbin`s that defines the same `_binary_*_start` / `_end` symbols
(`tools/gen_embed.py`).

## Deploying

`dist/` is the whole site -- static files only:

| File | |
|------|---|
| `index.html` | the page (the firmware version is filled in by `make`) |
| `demo.js`, `demo.css` | the page's script and styles |
| `variants.js` | the firmware variants present in `fw/` |
| `fw/<board>/<pack>/draftling.js`, `.wasm` | the firmware variants |

Copy them to the web root of demo.draftling.art. The server must send
`.wasm` files as `application/wasm` (most servers do). No special
headers are needed: the demo runs on the browser's main thread and does
not use `SharedArrayBuffer`. The page loads its files with
`?v=<firmware version>`, so long cache lifetimes are fine.

## How it works

| Part | Firmware | Demo |
|------|----------|------|
| Board settings | `sdkconfig` from the board's and the pack's defaults | `build/v/<board>/<pack>/sdkconfig.h`, from the same files (see Building) |
| Startup | `main/main.cpp` | `src/main_demo.cpp`: the same init order, with `main/app_config.h` and the board header |
| Color LCD | ST7796 over SPI, `lvgl_port.cpp` | `src/lvgl_port_demo.c`: LVGL renders into a frame buffer, changed rectangles go to a `<canvas>`; the page scales it with crisp pixels and dims it for the backlight setting |
| E-paper | `display_reterminal_sticky.cpp` | `src/display_demo.c`: a simulated SSD1677 panel, see below |
| Touch | FT6336U / GT911 | mouse / pen / finger on the canvas -> LVGL pointer |
| Buttons | GPIO pollers in `main.cpp` | `src/buttons_demo.c` + buttons beside the screen: FNK0104S BOOT (hold 2 s: forget keyboards); Sticky Power (press: sleep, hold 2 s: forget keyboards), Up / Down (Page Up / Page Down) |
| Keyboard | BLE HID (`ble_keyboard`) | `src/ble_keyboard_demo.c` + `web/demo.js`: browser key events -> HID usage codes and modifier byte, so the firmware's own layouts, dead keys, input methods and key repeat apply |
| SD card | FAT on MicroSD | Emscripten file system at `/sdcard`, kept in IndexedDB; `sd_card.cpp` runs unchanged on top of `src/sd_hw_demo.c`. Shared by all variants, like a card moved between devices |
| NVS | flash | `src/nvs_demo.cpp`, one file per board in IndexedDB (`/persist/nvs-<board>.bin`) |
| FreeRTOS | tasks, queues | `src/freertos_demo.c`: single-threaded; a task runs from a one-shot LVGL timer |
| WiFi | `wifi_manager` | `src/wifi_demo.c`: made-up networks; secured ones accept any password of 8+ characters |
| Git sync | `git_sync` | `src/git_sync_demo.cpp`: progress messages, then the project is copied to `<project>/.git/demo-server/`, so file status, Alt+I and Delete behave as after a real push. Draftbox is not available |
| USB | MSC, Kermit | `src/usb_demo.c`: both start, but no computer ever connects |
| Sleep / restart | deep sleep, `esp_restart()` | the page shows the device asleep, or reloads |
| Battery | ADC / BQ27220 | fixed at 75 %, not charging |

### The e-paper simulation

`src/display_demo.c` follows `display_reterminal_sticky.cpp`: the LVGL
port pushes RGB565 tiles into a 1-bpp frame buffer (the `epd_fb.h`
threshold) and calls `display_flush()` after the last tile. A flush
whose dirty box meets the editor's partial clip refreshes the panel
with the whole frame: the first refresh after start-up is a full one
(about 1.5 s), every later one a fast differential one (about 0.4 s,
the SSD1677 waveform the Sticky backend measures), plus the SPI
transfer. While a refresh runs the panel is busy; the device's LVGL
task blocks in `sticky_wait_idle()` before the next one, and the demo's
main loop likewise holds LVGL back while the panel is busy and there
is something new to draw. Keys typed meanwhile wait in the editor's key
queue and are handled together afterwards, as on the device. The page
shows each refresh as the panel does: changing pixels pass through grey
during a fast refresh, the whole screen flashes during the full one,
and a "refresh" mark lights under the screen while the panel is busy.

### Storage and "Reset demo"

On the first visit, `samples/` (embedded in every variant) is copied
onto the card: a `MyDrafts` project with Markdown, Fountain and
plain-text examples and a `git.cfg`, and a `wifi.cfg` naming an open
network, so Ctrl+G works straight away. "Reset demo" on the page
deletes all of the demo's browser data -- the card, every board's
settings and the remembered screen / pack -- and starts a fresh demo
(the IndexedDB databases are deleted at the next start, before the
firmware opens them).

### Limits

Phones and tablets need a hardware keyboard (a canvas does not bring up
the on-screen keyboard). Browsers keep some shortcuts for themselves
(Ctrl+N, Ctrl+W, Ctrl+T, Ctrl+Tab), which the buttons under the screen
send instead.

## When the firmware changes

The demo follows the firmware automatically as long as the editor's
dependencies stay the same. If a build fails after a firmware change,
the usual causes are:

- a new ESP-IDF / FreeRTOS call in the editor: add it to
  `shim/include/` (declaration) and `src/` (stand-in);
- a new function in the API of `wifi_manager`, `git_sync`,
  `ble_keyboard`, `standby`, `battery`, `display`, `usb_msc` or
  `kermit`: add it to the matching `src/*_demo.*` file;
- a new source or data file in `components/fonts` or
  `components/ime`: mirror its selection in `variant.mk` (new editor
  sources are picked up by a wildcard).

To simulate another board, add it to `BOARDS` in the Makefile and to
`BOARDS` in `web/demo.js` (name, buttons), and check
`src/main_demo.cpp` (SD card bus, battery backend), `src/buttons_demo.c`
and, for a new kind of panel, `src/display_demo.c`.
