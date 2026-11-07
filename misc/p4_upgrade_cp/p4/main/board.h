/*
 * The board this upgrade tool is built for (P4UP_BOARD in Kconfig, set
 * by the preset's sdkconfig.defaults.<board>). A board header defines:
 *
 *   BOARD_NAME, BOARD_CP_NAME, BOARD_CP_SHORT   names shown on screen
 *   BOARD_CP_CHIP_ID                            chip id the image must have
 *   BOARD_CP_SDMMC_SLOT, BOARD_CP_PIN_*         SDIO wiring, reset line
 *   board_cp_power_on()                         co-processor supply on
 *   board_display_start/lock/unlock()           LVGL display
 */
#pragma once

#include "sdkconfig.h"

#if defined(CONFIG_P4UP_BOARD_M5STACK_TAB5)
#include "boards/m5stack_tab5.h"
#else
#error "No board selected: build with a preset (idf.py --preset <board> build)"
#endif
