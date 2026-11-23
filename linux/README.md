# Draftling for Linux

The Draftling editor firmware built as a native Linux program. It runs
in a Wayland or X11 window, with the computer's own keyboard, a folder in place of the SD
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
draftling [-d DATA_DIR] [-c CONFIG_DIR] [-o wayland|x11] [-g WxH] [-b NAME|none] [-v]
```

| Option | |
|--------|---|
| `-d`, `--data-dir DIR` | the documents (the device's SD card): projects, `wifi.cfg`. Default `$XDG_DATA_HOME/draftling`, i.e. `~/.local/share/draftling` |
| `-c`, `--config-dir DIR` | the settings (the device's NVS, `settings.nvs`). Default `$XDG_CONFIG_HOME/draftling`, i.e. `~/.config/draftling`. One Draftling runs per settings folder |
| `-o`, `--io NAME` | the display and input to use, `wayland` or `x11`. Without it Wayland if it works, else X11 (as far as compiled in) |
| `-g`, `--geometry WxH` | initial window size (default 1024x640) |
| `-b`, `--backlight NAME` | the screen backlight to set: a device in `/sys/class/backlight`, or `none` to dim the window instead. Default: the screen's backlight, if it can be set (see below) |
| `-v` / `-q` | more (twice: debug) / fewer messages on standard error |
| `-V`, `-h` | version (with the language pack and the backends) / help |

Everything else works as on a device (F1 menu, F10 help; see the
user documentation in `docs/`), with these differences:

- **Keyboard.** Keys are read as physical keys, like a US keyboard
  connected to a device; the layout is chosen in Draftling (Ctrl+L,
  F1 -> Settings -> Keyboard -> Active layouts), not by the desktop.
  The computer's key repeat is ignored (the editor repeats keys
  itself). There is no Bluetooth menu: the computer's own keyboard is
  used.
- **Mouse.** The left button acts as a touch (tap, double-tap selects a
  word, drag selects); the wheel moves the cursor three lines per notch.
- **Window.** It can be resized; the editor lays its screens out again
  once the size has settled -- while a dialog, picker or pop-up is
  open, after it closes. The window splits panes (Ctrl+2 / Ctrl+3)
  top / bottom when it is taller than wide. F1 -> Settings -> Display
  has no orientation, rotate 180 or margin rows; "2x scale" (restart to
  apply) doubles every pixel, for high-density screens.
- **Backlight.** Ctrl+B and F1 -> Settings -> Display -> Backlight
  set the screen's backlight (`/sys/class/backlight`; with several,
  a `firmware` one before `platform` before `raw`) when Draftling may:
  by writing its `brightness` file (root, or a udev rule giving the
  user write access), else through systemd-logind's
  `SetBrightness` (with `busctl`; any user logged in at the computer).
  Until the backlight is first changed in Draftling, the screen keeps
  the level it had. Without a backlight that can be set, the window's
  picture is dimmed instead (full brightness until changed). `-v`
  logs which one is used.
- **Sleep and quit.** "Sleep now" (Ctrl+P) and the inactivity timeout
  (F1 -> Settings -> System -> Standby timeout) save every document and
  turn the screen off: the window goes black. The next key, click,
  wheel turn or pointer movement over the window turns it on again;
  that input only wakes it and is not typed. F1 -> "Quit Draftling"
  (Q) ends the program, asking first about unsaved changes like "Sleep
  now" does; closing the window and SIGTERM / SIGINT / SIGHUP save
  every document and end it too.
- **Storage.** "Format SD card" empties the data folder, but only one
  Draftling created (it carries the marker file `.draftling-data`); a
  folder given with `--data-dir` that already held files is never
  emptied. The battery indicator shows the laptop's battery, if any.
- **Network.** Git sync and Draftbox use the computer's network (and
  its CA certificates for TLS). "WiFi connected" means the computer has
  a network connection; the status names the WiFi network or the
  active connection. With NetworkManager running, F1 -> Connectivity ->
  WiFi scans and connects through `nmcli`, and Ctrl+W
  connects to the network in `wifi.cfg` or the one saved before. A
  network NetworkManager already has a profile for is connected with
  that profile (`nmcli connection up`); "Choose network..." marks it
  `[saved]` and does not ask for its password, unless NetworkManager
  refused the saved one the last time. A typed password goes to `nmcli connection up` in a file only
  the user can read; for a new network it is an
  argument of `nmcli device wifi connect`, visible in the process list
  while it runs. The profile that creates has autoconnect switched off
  (`connection.autoconnect no`), so the computer joins that network
  only when asked. Without NetworkManager the WiFi commands report that the system
  manages the network.
- **Screenshot.** `kill -USR1 <pid>` writes the current screen to
  `screenshot.ppm` in the settings folder (for bug reports).

## A minimal Draftling computer

To make a computer (a laptop, a single-board computer with a screen)
a dedicated Draftling writing machine, no desktop is needed: Weston,
the reference Wayland compositor, in its kiosk shell runs Draftling
full screen with nothing else around it.

1. Install Debian (12 "bookworm" or later: the package, built on
   Ubuntu 22.04, needs glibc 2.34) from the netinst image. On the
   "Software selection" page, untick "Debian desktop environment"
   and every desktop; keep "standard system utilities" (and "SSH
   server" if you want to reach the machine over the network).
2. Log in on the console and install Weston, NetworkManager and the
   Draftling package for the machine (`dpkg --print-architecture`
   names it; the packages are on https://flasher.draftling.art/,
   board "Linux computer"):

   ```
   sudo apt install weston network-manager ./draftling_<version>_arm64.deb
   ```

   NetworkManager leaves alone an interface listed in
   `/etc/network/interfaces`. If the installer set the WiFi up there,
   remove its lines (keep `lo`) and reboot, so that Draftling's WiFi
   menu can control it.
3. Create `~/.config/weston.ini`:

   ```
   [core]
   shell=kiosk
   # Draftling has its own standby (F1 -> Settings -> System); do not
   # let Weston blank the screen as well.
   idle-time=0

   [autolaunch]
   path=/usr/bin/draftling
   # Leave Weston when Draftling quits (F1 -> Quit Draftling).
   watch=true
   ```

   The kiosk shell shows every window full screen, without
   decorations or a panel. If the screen shows the text too small,
   use Draftling's F1 -> Settings -> Display -> 2x scale, or Weston's
   per-output `scale=2` (an `[output]` section with the output's
   `name=`, see `man weston.ini`).
4. Run `weston` on the console to try it. To start it on every boot,
   log the user in automatically on the first console
   (`sudo systemctl edit getty@tty1`, then add)

   ```
   [Service]
   ExecStart=
   ExecStart=-/sbin/agetty --autologin <user> --noclear %I $TERM
   ```

   and start Weston from the login, at the end of `~/.profile`:

   ```
   if [ -z "$WAYLAND_DISPLAY" ] && [ "$(tty)" = /dev/tty1 ]; then
       exec weston
   fi
   ```

   Quitting Draftling then ends the login, and the console logs in
   and starts it again; the other consoles (Ctrl+Alt+F2) are still
   there for maintenance.

Weston is started from a console login, so systemd-logind gives it the
screen and input devices, and Draftling can set the backlight through
logind as well. Cage, another kiosk compositor, runs Draftling too,
but it always shows the mouse cursor in the middle of the screen, on
top of the text; it is not recommended. Weston's kiosk shell works
best.

## Build requirements

A Linux host with the packages below; tested on Ubuntu 22.04 (gcc 11,
meson 0.61, glibc 2.35) and Ubuntu 26.04. Build on the oldest system
the program should run on: it needs the glibc it was built with or a
newer one (built on Ubuntu 22.04: glibc 2.34 or later).

| What | Debian / Ubuntu packages | Needed for |
|------|--------------------------|------------|
| C and C++ compiler, make | `build-essential` | everything |
| git, curl, python3, pkg-config | `git curl python3 pkgconf` | fetching the pinned sources, Kconfig, the build |
| OpenSSL headers and static libraries | `libssl-dev`, and what its static link needs there: nothing more on Ubuntu 22.04 / Debian 12 (OpenSSL 3.0), `libjitterentropy3-dev zlib1g-dev libzstd-dev` on Ubuntu 26.04 | Git sync / Draftbox (TLS, SHA-1) |
| meson, ninja, expat, libffi | `meson ninja-build libexpat1-dev libffi-dev` | the Wayland backend (libwayland-client and wayland-scanner are built from source, the library as a static one; the scanner needs expat) |
| XCB headers and static libraries | `libxcb1-dev libxcb-xkb-dev libxau-dev libxdmcp-dev` | the X11 backend |

All of them in one command (Debian 12 and later, Ubuntu 22.04 and
later; `libjitterentropy3-dev` is added only where it is packaged, and
`zlib1g-dev libzstd-dev` do no harm where OpenSSL does not need them):

```bash
sudo apt install build-essential git curl python3 pkgconf \
    libssl-dev zlib1g-dev libzstd-dev meson ninja-build libexpat1-dev \
    libffi-dev libxcb1-dev libxcb-xkb-dev libxau-dev libxdmcp-dev \
    $(apt-cache show libjitterentropy3-dev >/dev/null 2>&1 && echo libjitterentropy3-dev)
```

`make check-host` lists what is missing (for the backends in `IO`).
Which static archives OpenSSL needs differs between distributions: the
build takes the list from `pkg-config --static --libs libssl
libcrypto`. On Fedora the corresponding packages are `gcc gcc-c++ make
git curl python3 pkgconf openssl-devel openssl-static meson ninja-build
expat-devel libffi-devel libxcb-devel` (static archives are split out
differently there; `make check-host` names what is missing).

The Makefile fetches the rest itself, at pinned versions, into
`build/`: LVGL `v9.6.0`, kconfiglib `v14.1.0` (one Python file),
miniz `v114` (the ESP32 ROM's zlib), libwayland `1.24.0` and
wayland-protocols `1.47`. wayland-scanner is built from the libwayland
source too (`build/wayland-scanner-inst/`), since libwayland's build
wants a scanner of exactly its own version, which a distribution's need
not be (Ubuntu 22.04 has 1.20).

Everything but the C library is linked statically -- LVGL, libwayland
and libffi, XCB and its libraries, OpenSSL, libstdc++ and libgcc -- so
the program runs on any distribution with an equal or newer glibc, with
nothing else installed. NetworkManager (`nmcli`) is used when present.

## Building

```bash
cd linux
make -j8                          # build/bin/draftling: every language, Wayland + X11
make -j8 PACKS="all default"      # also build/bin/draftling-default (smaller)
make -j8 PACKS=japanese MAIN_PACK=japanese  # build/bin/draftling: Japanese only
make -j8 IO=x11                   # only some I/O backends
make run ARGS="-v"                # build and run
make install PREFIX=/usr/local    # also DESTDIR=... for staging
make deb                          # dist/draftling_<version>_<arch>.deb
make deb ARCH=arm64               # cross-built: dist/draftling_<version>_arm64.deb
make flatpak                      # dist/draftling_<version>_{x86_64,aarch64}.flatpak
make clean                        # build output (keeps fetched sources)
make distclean                    # everything not in git
```

**Language packs** are the firmware's (`default`, `vietnamese`,
`korean`, `japanese`, `chinese`, `indic`; `firmware/
sdkconfig.defaults.lang_<pack>`) plus **`all`**, one program each
(`PACKS`, default `all`): `draftling` for `MAIN_PACK` (default `all`),
`draftling-<pack>` for the others. `all` is a pack only the Linux
build has (`DRAFTLING_LANG_PACK_ALL`, too large for a device): every
layout, input method and font in one program (39 layouts; the
active-layout set starts with US only, the others are switched on in
F1 -> Settings -> Keyboard), about 15 MB stripped where `default` is
about 9 MB. Its CJK font
is the ja, zh and ko sets merged at build time by
`tools/merge_cjk_fonts.py` (kana from ja, bopomofo from zh, Hangul
from ko, Han characters in the Japanese style where the Japanese set
has them, else the Chinese one).

**I/O backends** (`IO`, default `wayland x11`): either or both can be
compiled in; at run time the first that works is used, in that order,
unless `--io` names one. A backend left out needs none of its build
requirements.

| File | Builds |
|------|--------|
| `Makefile` | fetches the dependencies, runs the files below, `install`, `deb`, `flatpak`, `check-host` |
| `arch.mk` | the target architecture (`ARCH`): toolchain, `pkg-config` folder, output folder |
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

## Cross-building

`ARCH=<Debian architecture>` (`arm64`, `armhf`, `riscv64`, `amd64`;
default: this host's) builds for another architecture with the
`<triplet>-` toolchain (`arch.mk`): everything goes to `build/<arch>/`
(the fetched sources in `build/` are shared), `pkg-config` reads only
the target's `/usr/lib/<triplet>/pkgconfig`, and libwayland is built
with a generated meson cross file. Every make target takes it:
`make ARCH=arm64`, `make deb ARCH=arm64`, `make check-host ARCH=arm64`.
`CROSS_COMPILE=<prefix>` overrides the toolchain prefix.

The host needs the cross compiler and the target's static libraries,
from the Debian / Ubuntu multiarch packages; `make check-host
ARCH=<arch>` lists what is missing. On Ubuntu the arm64 packages come
from `ports.ubuntu.com`, and `archive.ubuntu.com` carries only amd64
(and i386), so every apt source must name its architectures:
otherwise `apt update` fails with `404 Not Found` on
`.../binary-arm64/Packages` from the archive and
`.../binary-amd64/Packages` from ports, and no `:arm64` package can be
installed.

- Ubuntu 24.04 and later (deb822 sources): add `Architectures: amd64`
  to `/etc/apt/sources.list.d/ubuntu.sources`, and a sources file for
  `http://ports.ubuntu.com/ubuntu-ports/` with `Architectures: arm64`.
- Ubuntu 22.04 (`/etc/apt/sources.list`): put `[arch=amd64]` after
  each `deb` there, and the ports lines in a file of their own,
  e.g. `/etc/apt/sources.list.d/arm.list`:

  ```
  deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports jammy main restricted universe multiverse
  deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports jammy-updates main restricted universe multiverse
  deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports jammy-backports main restricted universe multiverse
  deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports jammy-security main restricted universe multiverse
  ```

Then

```bash
sudo dpkg --add-architecture arm64 && sudo apt update
sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu binutils-aarch64-linux-gnu \
  libssl-dev:arm64 libjitterentropy3-dev:arm64 zlib1g-dev:arm64 libzstd-dev:arm64 \
  libffi-dev:arm64 libxcb1-dev:arm64 libxcb-xkb-dev:arm64 libxau-dev:arm64 libxdmcp-dev:arm64
```

(Ubuntu 22.04 has no `libjitterentropy3-dev`, and its OpenSSL 3.0
does not need it: leave it out there.)

For armhf (32-bit Arm, ARMv7 with hardware floating point) the same
with `armhf`: `Architectures: arm64 armhf` in the ports sources file,
`sudo dpkg --add-architecture armhf`, and

```bash
sudo apt install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf binutils-arm-linux-gnueabihf \
  libssl-dev:armhf libjitterentropy3-dev:armhf zlib1g-dev:armhf libzstd-dev:armhf \
  libffi-dev:armhf libxcb1-dev:armhf libxcb-xkb-dev:armhf libxau-dev:armhf libxdmcp-dev:armhf
```

(`libjitterentropy3-dev`, `zlib1g-dev` and `libzstd-dev` only where
OpenSSL's static link needs them, as above.)

32-bit targets are built with 64-bit file offsets and `time_t`
(`ABI_FLAGS` in `arch.mk`), as the distributions build their 32-bit
libraries. Changing compiler flags does not rebuild existing objects:
run `make clean ARCH=<arch>` after such a change.

The host's own meson, ninja and python3 are used as they are, and
wayland-scanner is built for the host (with the host's `cc` and its
`libexpat1-dev`, not the target's). `wayland.mk` handles two
differences of old meson versions (Ubuntu 22.04: 0.61): the cross
file's pkg-config entry is called `pkgconfig` before meson 1.3 (with
the newer `pkg-config` name 0.61 finds no pkg-config for the target
and stops at libffi, "Pkg-config binary for machine 1 not found"),
and build-machine dependencies such as wayland-scanner are looked up
with `PKG_CONFIG_PATH_FOR_BUILD`, not `PKG_CONFIG_PATH`. When the
meson or ninja step of libwayland fails, make prints the end of its
log (`build/<arch>/wayland-build-setup.log`, `-ninja.log`). (Debian needs no extra sources:
its mirrors carry every architecture.)

## Packaging

`make install` (with `DESTDIR` and `PREFIX`) installs the programs,
`draftling.desktop`, the AppStream metadata, the man page and the
documentation; packaging for any system starts from that tree.
`make deb` builds a Debian package from it
(`packaging/deb/build-deb.sh`: `dpkg-deb`, `Depends: libc6 (>= the
newest glibc symbol version used)`, `Recommends: network-manager`),
with the packs in `PACKS` (by default just `draftling`, the `all`
pack), e.g. `make deb PACKS="all default"`.
Other formats (RPM, Arch, BSD ports) would add a script next to
`packaging/deb/`.

### Flatpak

`make flatpak` builds one single-file bundle per architecture,
`dist/draftling_<version>_x86_64.flatpak` and `..._aarch64.flatpak`
(`FLATPAK_ARCHES="x86_64"` for just one), from the manifest
`packaging/flatpak/art.draftling.Draftling.yml`, run by
`packaging/flatpak/build-flatpak.sh`. Flathub's runtimes exist for
x86_64 and aarch64 only, so there is no 32-bit Arm (armhf) Flatpak;
armhf keeps the Debian package.

The bundle holds the `all` pack with both I/O backends, built in the
Freedesktop SDK 25.08 with `FLATPAK=1`:

- `sdkconfig.defaults.flatpak` (`DRAFTLING_LINUX_FLATPAK`) drops what
  the sandbox cannot do: no backlight setting (no Ctrl+B, no
  `--backlight`), no battery indicator, and no WiFi control.
  The network is the computer's (`DRAFTLING_NETWORK_HOST_MANAGED`,
  `src/wifi_host.c` in place of `src/wifi_linux.c`): Draftling takes
  it as always up, so Git sync and Draftbox just try it, and there is no
  WiFi menu, F1 -> Connectivity, Ctrl+W, WiFi icon or WiFi step in
  "Format SD card".
- `LINK=dynamic`: the runtime's shared OpenSSL, XCB and
  libwayland-client instead of static archives (the SDK has none), and
  the SDK's wayland-scanner and wayland-protocols.
- Documents and settings are in the app's own folders,
  `~/.var/app/art.draftling.Draftling/data/draftling` and
  `~/.var/app/art.draftling.Draftling/config/draftling` (the program's
  usual `$XDG_DATA_HOME` / `$XDG_CONFIG_HOME` folders, which Flatpak
  points there), so the app has no access to the home folder.
- Permissions: `--socket=wayland`, `--socket=fallback-x11` (X11 only
  when the session has no Wayland) with `--share=ipc`, and
  `--share=network`.

The manifest builds this checkout's sources (`linux/`,
`firmware/main`, `firmware/components`); the LVGL, Kconfiglib and
miniz downloads of the Makefile are pinned in it, since flatpak-builder
builds without network access. Change them together with the versions
in the `Makefile`. `make FLATPAK=1` builds the same program in
`build/flatpak/bin/` (run it inside the SDK: `flatpak run
--filesystem=$PWD/.. --command=make org.freedesktop.Sdk//25.08 -C
$PWD FLATPAK=1`).

Requirements (Ubuntu / Debian), on top of `flatpak` and the Flathub
remote:

```bash
sudo apt install flatpak-builder qemu-user-binfmt
flatpak install --user -y flathub org.freedesktop.Sdk//25.08 org.freedesktop.Platform//25.08
flatpak install --user -y --arch=aarch64 flathub org.freedesktop.Sdk//25.08 org.freedesktop.Platform//25.08
```

The SDK and runtime are also installed by the build when missing.
`qemu-user-binfmt` runs the other architecture's build under
emulation (the aarch64 build on an x86_64 computer takes a good while
longer than the native one). Installing and running a bundle:

```bash
flatpak install --user dist/draftling_<version>_x86_64.flatpak
flatpak run art.draftling.Draftling          # or from the desktop's menu
flatpak run art.draftling.Draftling -v       # options as above
```

`--socket=fallback-x11` gives the app an X display only when the
session has no Wayland -- even when `--socket=x11` is added -- so on a
Wayland desktop `--io x11` needs the fallback turned off for that run:
`flatpak run --nosocket=fallback-x11 --socket=x11
art.draftling.Draftling --io x11`.

## How it works

| Part | Firmware | Linux |
|------|----------|-------|
| Board settings | `sdkconfig` from the board's and the pack's defaults | `build/p/<pack>-<io>/sdkconfig.h`, from `linux/sdkconfig.defaults` and the pack's defaults |
| Startup | `main/main.cpp` | `src/main_linux.cpp`: options, folders, single-instance lock, then the same init order |
| Display | panel drivers, `lvgl_port.cpp` | `src/lvgl_port_linux.c`: LVGL renders RGB565 tiles into a 32-bit frame of the window's size (2x2 blocks with "2x scale"); the changed rectangle goes to the backend. Resize: `editor_ui_can_relayout()` / `editor_ui_relayout()` |
| Wayland | | `src/io_wayland.c`: xdg-shell window, two SHM buffers, server-side decorations where offered (xdg-decoration) |
| X11 | | `src/io_x11.c`: XCB window, PutImage in bands, XKB detectable auto-repeat |
| Keyboard | BLE HID (`ble_keyboard`) | `src/keyboard_linux.c`: Linux key codes -> HID usage codes and modifier byte (the kernel's usbkbd table), through the BLE keyboard API |
| Touch | GT911 etc. | the window's mouse -> LVGL pointer |
| SD card | FAT on MicroSD | the data folder; the process runs in it and `sd_card.cpp` mounts it at `.` (`src/sd_hw_linux.c`) |
| NVS | flash | `src/nvs_linux.cpp`: `settings.nvs`, written atomically on every commit |
| FreeRTOS | tasks, queues | `src/freertos_linux.c`: POSIX threads; the LVGL lock is a recursive mutex, as on the device |
| WiFi | `wifi_manager` | `src/wifi_linux.c`: system network state, NetworkManager through `nmcli`, a monitor thread |
| Git sync, Draftbox | `git_sync` over `esp_http_client` | the same code over `src/http_client_linux.c` (sockets + OpenSSL) and miniz |
| Battery | ADC / fuel gauges | `src/battery_linux.c`: `/sys/class/power_supply` |
| Sleep / restart | deep sleep, `esp_restart()` | `src/standby_linux.c`: save and blank the window (`DRAFTLING_STANDBY_DISPLAY_OFF`) until input; "Quit Draftling" (`DRAFTLING_HAS_QUIT`, `standby_quit()`); restart re-executes the program |

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
Wayland and X11 run on the BSDs as well, and the Linux-specific
`/proc` / `/sys` lookups are isolated (`src/battery_linux.c`,
`src/esp_linux.c`, `src/wifi_linux.c`). A FreeBSD or NetBSD port
would add stand-ins for those (e.g. `sysctl` for the battery) and a
package script.
