# P4 co-processor firmware upgrade

Installs ESP-Hosted firmware on the radio co-processor of an ESP32-P4
board from the P4 itself, over the SDIO link between them. Each board
has a preset (`idf.py --preset <board>`) in `cp/CMakePresets.json` and
`p4/CMakePresets.json`. Supported boards:

| Preset | Board | Co-processor |
|--------|-------|--------------|
| `m5stack_tab5` | M5Stack Tab5 | ESP32-C6 |

## M5Stack Tab5

The M5Stack Tab5 has no 2.4 GHz radio on its ESP32-P4; Wi-Fi and
Bluetooth come from an on-board ESP32-C6 running
[ESP-Hosted](https://github.com/espressif/esp-hosted-mcu) firmware,
reached over SDIO. Draftling needs **esp-hosted 3.1** on the C6. This
tool installs it from the P4, over the existing SDIO link: no jumper
wires and no USB-serial adapter on the C6.

It works whatever esp-hosted generation the C6 runs now:

| C6 firmware | How the tool recognises it |
|-------------|----------------------------|
| M5Stack factory image ("V1.4.1", esp-hosted-mcu 1.4.1) and other 0.0.x / 1.x builds | start-up message without a version |
| esp-hosted-mcu 2.x (what earlier Draftling docs asked for) | version in the start-up message |
| esp-hosted 3.x | version and RPC version in the start-up message |
| ESP-Hosted-FG / 3.x built for Linux FG hosts | legacy interface numbers, `ctrlResp` endpoint |

Progress is shown on the Tab5 screen.

### What it does

1. Switches the C6 supply on (WLAN_PWR_EN on the I/O expander), resets
   the C6 (GPIO15) and brings the SDIO card up. Bus settings are tried
   from fast to slow: 4-bit 20 MHz, 4-bit 10 MHz, 1-bit 10 MHz, 1-bit
   400 kHz.
2. Opens the data path and reads the C6's start-up message (INIT
   event): firmware version, RPC version, interface numbering.
3. Probes the RPC dialects with a harmless MAC-address request:
   esp-hosted-mcu (`RPCRsp`, RPC ids 0x1xx; 0.0.x to 3.x) and
   ESP-Hosted-FG (`ctrlResp`, RPC ids 1xx), the one the start-up
   message points to first.
4. Installs the embedded image by OTA (begin, write in 1 KB chunks,
   end, and activate on 2.6+ firmware; 1.x and FG firmware switch the
   boot partition in OTA end).
5. Resets the C6 and reconnects: the start-up message must now report
   the new version.

OTA writes the partition the C6 is not running from and switches only
after the image is validated, so if anything fails the C6 keeps its old
firmware. Press RESET to run the tool again.

The new image is Wi-Fi plus a BLE controller over hosted HCI, SDIO
packet mode, built from `cp/`. It is optimised for size: OTA cannot
change the C6's partition table, and the smallest app slot of the
tables in use (esp-hosted-mcu 1.x) is 1536 KB; the build fails if the
image grows past that.

### Build and run

ESP-IDF v6.1 or later:

```bash
. $IDF_PATH/export.sh
misc/p4_upgrade_cp/build.sh m5stack_tab5
cd misc/p4_upgrade_cp/p4
idf.py --preset m5stack_tab5 -p /dev/ttyACM0 flash monitor
```

`build.sh <preset>` builds the co-processor image
(`cp/build/<preset>/cp_hosted.bin`) and then the P4 tool, which embeds
it.

The tool replaces Draftling on the P4. When the screen says "Done",
flash Draftling again (a build for esp-hosted 3.1, see
`docs/tab5-esp-hosted.md`). The C6 firmware stays across P4 reflashes.

If the C6 already runs the version this tool carries, the screen says
so and offers to install it again.

### If the C6 does not answer

"The C6 does not answer in any known esp-hosted dialect" means the C6
runs something that is not esp-hosted, or is wedged. Power the Tab5
off and on (not just RESET) and run the tool again. As a last resort,
flash `cp/build/m5stack_tab5/` over the C6's own serial port with its
GPIO9 strap held low, as described in `docs/tab5-esp-hosted.md`.

## Adding a board

1. `cp/`: a `sdkconfig.defaults.<board>` (target, flash size,
   partition table) and a preset in `cp/CMakePresets.json` with
   `CP_MAX_APP_SIZE`, the smallest app slot the co-processor's current
   firmware may have (OTA cannot change the partition table).
2. `p4/`: a `P4UP_BOARD_<BOARD>` option and its `P4UP_BOARD_ID` in
   `main/Kconfig.projbuild`, a header in `main/boards/` (see
   `main/board.h` for what it defines), its `#elif` in `main/board.h`,
   the board's BSP in `main/idf_component.yml` with a
   `$CONFIG{P4UP_BOARD_<BOARD>}` rule, a `sdkconfig.defaults.<board>`
   selecting the option, and a preset in `p4/CMakePresets.json`.
