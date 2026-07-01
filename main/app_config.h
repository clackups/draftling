#pragma once

#include "sdkconfig.h"

/* Display dimensions (derived from Kconfig hardware model selection) */
#define DISPLAY_WIDTH   CONFIG_DRAFTLING_DISPLAY_WIDTH
#define DISPLAY_HEIGHT  CONFIG_DRAFTLING_DISPLAY_HEIGHT
#define DISPLAY_ROTATE  CONFIG_DRAFTLING_DISPLAY_ROTATE_ANGLE

/* Logical pixel size: every logical LVGL pixel is rendered as
 * DISPLAY_SCALE x DISPLAY_SCALE panel pixels by the display backend
 * (nearest-neighbor expansion). The editor and LVGL canvas use
 * DISPLAY_LOGICAL_WIDTH / DISPLAY_LOGICAL_HEIGHT; only the display
 * backend deals in physical panel pixels. */
#define DISPLAY_SCALE          CONFIG_DRAFTLING_DISPLAY_SCALE
#define DISPLAY_LOGICAL_WIDTH  (DISPLAY_WIDTH  / DISPLAY_SCALE)
#define DISPLAY_LOGICAL_HEIGHT (DISPLAY_HEIGHT / DISPLAY_SCALE)

/* SD Card mount point (shared across all hardware models) */
#define SD_MOUNT_POINT  "/sdcard"

/* Per-board pin definitions and constants.  Each file defines
 * BOARD_NAME and all GPIO / ADC macros for exactly one board. */
#if defined(CONFIG_DRAFTLING_MODEL_WAVESHARE_RLCD42)
#include "boards/waveshare_rlcd42.h"
#elif defined(CONFIG_DRAFTLING_MODEL_LILYGO_T5_EPD_S3_PRO)
#include "boards/lilygo_t5_epd_s3_pro.h"
#elif defined(CONFIG_DRAFTLING_MODEL_LILYGO_T5_EPD_S3_PRO_H752)
#include "boards/lilygo_t5_epd_s3_pro_h752.h"
#elif defined(CONFIG_DRAFTLING_MODEL_M5STACK_PAPERS3)
#include "boards/m5stack_papers3.h"
#elif defined(CONFIG_DRAFTLING_MODEL_WAVESHARE_TOUCH_LCD_349)
#include "boards/waveshare_touch_lcd_349.h"
#elif defined(CONFIG_DRAFTLING_MODEL_JC3248W535)
#include "boards/jc3248w535.h"
#elif defined(CONFIG_DRAFTLING_MODEL_M5STACK_TAB5)
#include "boards/m5stack_tab5.h"
#elif defined(CONFIG_DRAFTLING_MODEL_SUNTON_8048S070)
#include "boards/sunton_8048s070.h"
#elif defined(CONFIG_DRAFTLING_MODEL_SUNTON_8048S043)
#include "boards/sunton_8048s043.h"
#else
#error "No hardware model selected. Run idf.py menuconfig and choose a model."
#endif

/* Default: no deep-sleep-only BL cut pin. Boards whose BL cannot
 * be cut via the normal LCD_BL_PIN path (because the BL boost
 * circuit does not tolerate the ESP32 driving the BL enable pin
 * during normal operation) may override this to drive a separate
 * pin LOW only at deep-sleep entry. No current board uses this
 * fallback; kept available for future boards that need it. */
#ifndef LCD_BL_DEEP_SLEEP_CUT_PIN
#define LCD_BL_DEEP_SLEEP_CUT_PIN   -1
#endif

/* Default: BL pin is active HIGH (LEDC duty MAX = full brightness,
 * duty 0 = off). Boards whose BL boost circuit is active LOW
 * (LEDC duty 0 = full brightness, duty MAX = off) override this
 * to 1 -- see the boards/waveshare_touch_lcd_349.h for the
 * Waveshare ESP32-S3-Touch-LCD-3.49. */
#ifndef LCD_BL_ACTIVE_LOW
#define LCD_BL_ACTIVE_LOW   0
#endif
