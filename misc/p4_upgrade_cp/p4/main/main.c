/*
 * Co-processor upgrade tool: installs the esp-hosted image built by ../cp
 * on the board's co-processor (Tab5: ESP32-C6) from the ESP32-P4, over
 * the existing SDIO link, whatever esp-hosted version it runs now.
 * Progress is shown on the board's screen. See ../README.md.
 */
#include "board.h"
#include "ui.h"
#include "upgrader.h"

void app_main(void)
{
    ui_init(BOARD_NAME " " BOARD_CP_SHORT " firmware upgrade");
    upgrader_start();
}
