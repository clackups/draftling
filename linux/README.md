# Draftling for Linux

The Draftling editor firmware built as a native Linux program. It runs
in a Wayland or X11 window, or full screen on the Linux framebuffer
console, with the computer's own keyboard, a folder in place of the SD
card, the computer's network for Git sync, and NetworkManager for WiFi.

As with the [online demo](../online_demo/README.md), the firmware's own
code runs unchanged -- the editor (`components/editor`), keyboard
layouts (`components/kb_layout`), fonts, input methods, HarfBuzz, the
SD card layer (`components/sd_card/sd_card.cpp`) and the real Git
client (`components/git_sync`) -- with LVGL 9.6.0. The hardware and
FreeRTOS are replaced by the stand-ins in `src/`. The Linux "board" is
a hardware model of its own, `DRAFTLING_MODEL_LINUX` in
`firmware/main/Kconfig.projbuild` (`main/boards/linux.h`), so its
feature flags come from the same Kconfig rules as every device.

## Using it

```
draftling [-d DATA_DIR] [-c CONFIG_DIR] [-o wayland|x11|fbdev] [-g WxH] [-v]
```

| Option | |
|--------|---|
| `-d`, `--data-dir DIR` | the documents (the device's SD card): projects, `wifi.cfg`. Default `$XDG_DATA_HOME/draftling`, i.e. `~/.local/share/draftling` |
| `-c`, `--config-dir DIR` | the settings (the device's NVS, `settings.nvs`). Default `$XDG_CONFIG_HOME/draftling`, i.e. `~/.config/draftling`. One Draftling runs per settings folder |
| `-o`, `--io NAME` | the display and input to use. Without it the first that works of Wayland, X11, framebuffer (as far as compiled in) |
| `-f`, `--fbdev DEVICE` | framebuffer device (default `/dev/fb0`) |
| `-g`, `--geometry WxH` | initial window size (default 1024x640) |
| `-v` / `-q` | more (twice: debug) / fewer messages on standard error |
| `-V`, `-h` | version (with the language pack and the backends) / help |

Everything else works as on a device (F1 menu, F10 help; see the
user documentation in `docs/`), with these differences:

- **Keyboard.** Keys are read as physical keys, like a US keyboard
  connected to a device; the layout is chosen in Draftling (Ctrl+L,
  F1 -> Settings -> Keyboard -> Active layouts), not by the desktop.
  The computer's key repeat is ignored (the editor repeats keys
  itself). The Bluetooth menu shows the computer keyboard as connected.
- **Mouse.** The left button acts as a touch (tap, double-tap selects a
  word, drag selects); the wheel moves the cursor three lines per notch.
- **Window.** It can be resized; the editor lays its screens out again
  once the size has settled -- while a dialog, picker or pop-up is
  open, after it closes. The window splits panes (Ctrl+2 / Ctrl+3)
  top / bottom when it is taller than wide. F1 -> Settings -> Display
  has no orientation, rotate 180 or margin rows; "2x scale" (restart to
  apply) doubles every pixel, for high-density screens.
- **Sleep.** "Sleep now" (Ctrl+P) saves every document and quits, as
  do closing the window and SIGTERM / SIGINT / SIGHUP. The inactivity
  timeout never quits the program.
- **Storage.** "Format SD card" empties the data folder, but only one
  Draftling created (it carries the marker file `.draftling-data`); a
  folder given with `--data-dir` that already held files is never
  emptied. The battery indicator shows the laptop's battery, if any.
- **Network.** Git sync and Draftbox use the computer's network (and
  its CA certificates for TLS). "WiFi connected" means the computer has
  a network connection; the status names the WiFi network or the
  active connection. With NetworkManager running, F1 -> Connectivity ->
  WiFi scans and connects through `nmcli` (the password is an argument
  of `nmcli`, visible in the process list while it runs), and Ctrl+W
  connects to the network in `wifi.cfg` or the one saved before.
  Without NetworkManager the WiFi commands report that the system
  manages the network.
- **Framebuffer console** (`--io fbdev`): needs write access to the
  framebuffer (the `video` group) and read access to
  `/dev/input/event*` (the `input` group); every keyboard is used, and
  keyboards plugged in later are picked up. Started on a virtual
  console, Draftling puts it into graphics mode and switches its
  keyboard off while it runs (so typing does not reach the shell, and
  Ctrl+Alt+Fn does not switch consoles), and restores both on exit.
  No mouse; the screen size is the framebuffer's.
- **Screenshot.** `kill -USR1 <pid>` writes the current screen to
  `screenshot.ppm` in the settings folder (for bug reports).

## Build requirements

A Linux host with:

| What | Debian / Ubuntu packages | Needed for |
|------|--------------------------|------------|
| C and C++ compiler, make | `build-essential` | everything |
| git, curl, python3, pkg-config | `git curl python3 pkgconf` | fetching the pinned sources, Kconfig, the build |
| OpenSSL headers and static libraries | `libssl-dev`, and what its static link needs: `libjitterentropy3-dev zlib1g-dev libzstd-dev` (Ubuntu 26.04) | Git sync / Draftbox (TLS, SHA-1) |
| meson, ninja, wayland-scanner, libffi | `meson ninja-build libwayland-bin libffi-dev` | the Wayland backend (libwayland-client is built from source as a static library) |
| XCB headers and static libraries | `libxcb1-dev libxcb-xkb-dev libxau-dev libxdmcp-dev` | the X11 backend |
| Linux kernel headers | `linux-libc-dev` | the framebuffer backend |

`make check-host` lists what is missing (for the backends in `IO`).
Which static archives OpenSSL needs differs between distributions: the
build takes the list from `pkg-config --static --libs libssl
libcrypto`. On Fedora the corresponding packages are `gcc gcc-c++ make
git curl python3 pkgconf openssl-devel openssl-static meson ninja-build
wayland-devel libffi-devel libxcb-devel` (static archives are split out
differently there; `make check-host` names what is missing).

The Makefile fetches the rest itself, at pinned versions, into
`build/`: LVGL `v9.6.0`, kconfiglib `v14.1.0` (one Python file),
miniz `v114` (the ESP32 ROM's zlib), libwayland `1.24.0` and
wayland-protocols `1.47`.

Everything but the C library is linked statically -- LVGL, libwayland
and libffi, XCB and its libraries, OpenSSL, libstdc++ and libgcc -- so
the program runs on any distribution with an equal or newer glibc, with
nothing else installed. NetworkManager (`nmcli`) is used when present.

## Building

```bash
cd linux
make -j8                          # build/bin/draftling, default pack
make -j8 PACKS="default japanese" # also build/bin/draftling-japanese
make -j8 PACKS=all                # build/bin/draftling-all: every language
make -j8 IO="x11 fbdev"           # only some I/O backends
make run ARGS="-v"                # build and run
make install PREFIX=/usr/local    # also DESTDIR=... for staging
make deb                          # dist/draftling_<version>_<arch>.deb
make clean                        # build output (keeps fetched sources)
make distclean                    # everything not in git
```

**Language packs** are the firmware's (`default`, `vietnamese`,
`korean`, `japanese`, `chinese`, `indic`; `firmware/
sdkconfig.defaults.lang_<pack>`), one program each: `draftling` for
`default`, `draftling-<pack>` for the others. **`all`** is a pack only
the Linux build has (`DRAFTLING_LANG_PACK_ALL`, too large for a
device): every layout, input method and font in one program (about
40 layouts; the active-layout set starts with US only). Its CJK font
is the ja, zh and ko sets merged at build time by
`tools/merge_cjk_fonts.py` (kana from ja, bopomofo from zh, Hangul
from ko, Han characters in the Japanese style where the Japanese set
has them, else the Chinese one).

**I/O backends** (`IO`, default `wayland x11 fbdev`): any subset can be
compiled in; at run time the first that works is used, in that order,
unless `--io` names one. Backends left out need none of their build
requirements. The framebuffer backend is Linux-only.

| File | Builds |
|------|--------|
| `Makefile` | fetches the dependencies, runs the files below, `install`, `deb`, `check-host` |
| `lvgl.mk` | LVGL as one static library shared by every pack |
| `wayland.mk` | libwayland-client as a static library (meson) |
| `draftling.mk` | one program: one language pack, with the backends in `IO` |

`draftling.mk` generates the pack's `sdkconfig.h` from the firmware's
own `Kconfig.projbuild` files with `linux/sdkconfig.defaults` (the
`DRAFTLING_MODEL_LINUX` board) and the pack's
`sdkconfig.defaults.lang_<pack>` (`tools/gen_sdkconfig.py`,
kconfiglib), reads the `CONFIG_*` values back into make and picks the
sources, font data and input methods the way the firmware's component
`CMakeLists.txt` files do. Font and dictionary blobs (ESP-IDF
`EMBED_FILES`) are linked in through an assembler file of `.incbin`s
(`tools/gen_embed.py`).

## Packaging

`make install` (with `DESTDIR` and `PREFIX`) installs the programs,
`draftling.desktop`, the AppStream metadata, the man page and the
documentation; packaging for any system starts from that tree.
`make deb` builds a Debian package from it
(`packaging/deb/build-deb.sh`: `dpkg-deb`, `Depends: libc6 (>= the
newest glibc symbol version used)`, `Recommends: network-manager`),
with the packs in `PACKS`, e.g. `make deb PACKS="default all"`.
Other formats (RPM, Arch, BSD ports) would add a script next to
`packaging/deb/`.

## How it works

| Part | Firmware | Linux |
|------|----------|-------|
| Board settings | `sdkconfig` from the board's and the pack's defaults | `build/p/<pack>-<io>/sdkconfig.h`, from `linux/sdkconfig.defaults` and the pack's defaults |
| Startup | `main/main.cpp` | `src/main_linux.cpp`: options, folders, single-instance lock, then the same init order |
| Display | panel drivers, `lvgl_port.cpp` | `src/lvgl_port_linux.c`: LVGL renders RGB565 tiles into a 32-bit frame of the window's size (2x2 blocks with "2x scale"); the changed rectangle goes to the backend. Resize: `editor_ui_can_relayout()` / `editor_ui_relayout()` |
| Wayland | | `src/io_wayland.c`: xdg-shell window, two SHM buffers, server-side decorations where offered (xdg-decoration) |
| X11 | | `src/io_x11.c`: XCB window, PutImage in bands, XKB detectable auto-repeat |
| Framebuffer | | `src/io_fbdev.c`: `/dev/fb*` (16 / 32 bpp), evdev keyboards, the virtual console in graphics mode with its keyboard off |
| Keyboard | BLE HID (`ble_keyboard`) | `src/keyboard_linux.c`: Linux key codes -> HID usage codes and modifier byte (the kernel's usbkbd table), through the BLE keyboard API |
| Touch | GT911 etc. | the window's mouse -> LVGL pointer |
| SD card | FAT on MicroSD | the data folder; the process runs in it and `sd_card.cpp` mounts it at `.` (`src/sd_hw_linux.c`) |
| NVS | flash | `src/nvs_linux.cpp`: `settings.nvs`, written atomically on every commit |
| FreeRTOS | tasks, queues | `src/freertos_linux.c`: POSIX threads; the LVGL lock is a recursive mutex, as on the device |
| WiFi | `wifi_manager` | `src/wifi_linux.c`: system network state, NetworkManager through `nmcli`, a monitor thread |
| Git sync, Draftbox | `git_sync` over `esp_http_client` | the same code over `src/http_client_linux.c` (sockets + OpenSSL) and miniz |
| Battery | ADC / fuel gauges | `src/battery_linux.c`: `/sys/class/power_supply` |
| Sleep / restart | deep sleep, `esp_restart()` | save and quit; restart re-executes the program |

## When the firmware changes

As for the online demo: a new ESP-IDF / FreeRTOS call in the editor or
Git client needs a declaration in `shim/include/` and a stand-in in
`src/`; a new function in the API of `wifi_manager`, `ble_keyboard`,
`standby`, `battery` or `display` needs it in the matching
`src/*_linux.*` file; a new source or data file in `components/fonts`,
`components/ime` or `components/git_sync` needs its selection in
`draftling.mk`.

## Future platforms

The build and the program are kept portable where it costs little:
the framebuffer backend (Linux `fbdev` / `evdev` / virtual console) and
`/proc` / `/sys` lookups are Linux-specific and isolated
(`src/io_fbdev.c`, `src/battery_linux.c`, `src/esp_linux.c`,
`src/wifi_linux.c`); the Makefile leaves the framebuffer backend out
on other systems. A FreeBSD or NetBSD port would add stand-ins for
those (e.g. `sysctl` for the battery) and a package script.
