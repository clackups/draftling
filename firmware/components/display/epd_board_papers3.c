/*
 * Custom epdiy board definition for the M5Stack PaperS3.
 *
 * The PaperS3 ships with the same 4.7" 960x540 ED047TC1 e-paper
 * panel as the LilyGO T5 E-Paper S3 Pro, but with a markedly
 * simpler driver topology:
 *
 *  - 8-bit parallel data bus on direct ESP32-S3 GPIOs (different
 *    pin map from epdiy's lilygo_board_s3 / epd_board_v7);
 *  - control lines (CKH, CKV, STH, SPV, XLE) on direct GPIOs,
 *    driven by epdiy's LCD-peripheral output path;
 *  - NO TPS65185 high-voltage supply chip and NO PCA9555/PCA9535
 *    I/O expander: instead the panel power rail is gated by two
 *    push-pull MOSFETs commanded directly by EPD_EN (GPIO45) and
 *    BST_EN (GPIO46);
 *  - NO controllable VCOM (the boost network produces a fixed
 *    common-electrode voltage), NO panel temperature sensor.
 *
 * Pin map references:
 *  - M5Stack's M5Unified driver
 *    (M5Stack/M5Unified panels/Panel_M5PaperS3.cpp), which is the
 *    authoritative wiring source for the PaperS3.
 *  - jifanchn/micropython-papers3m5 papers3/papers3_pins.h, which
 *    independently lists the same pin numbers.
 *
 * The same panel timing constants the in-tree LilyGO board uses
 * (ckv_high_time = 60, line_front_porch = 4, le_high_time = 4)
 * are reused here -- the ED047TC1 silicon and its 480/540-line
 * timing are identical across both products. The pixel clock
 * comes from `epd_get_display()->bus_speed` so it tracks the
 * panel descriptor (ED047TC1 ships with bus_speed = 12 MHz which
 * is the documented sweet spot).
 */

#include "sdkconfig.h"
#if defined(CONFIG_DRAFTLING_EPDIY_BOARD_PAPERS3)

#include <stdbool.h>
#include <stdint.h>

#include "epd_board.h"
#include "epdiy.h"
#include "output_lcd/lcd_driver.h"

#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_rom_sys.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *TAG = "epd_papers3";

/* --- Parallel data bus (D0..D7) --- */
#define PAPERS3_D0   GPIO_NUM_6
#define PAPERS3_D1   GPIO_NUM_14
#define PAPERS3_D2   GPIO_NUM_7
#define PAPERS3_D3   GPIO_NUM_12
#define PAPERS3_D4   GPIO_NUM_9
#define PAPERS3_D5   GPIO_NUM_11
#define PAPERS3_D6   GPIO_NUM_8
#define PAPERS3_D7   GPIO_NUM_10

/* --- Control lines --- */
#define PAPERS3_CKH  GPIO_NUM_16   /* horizontal clock           */
#define PAPERS3_CKV  GPIO_NUM_18   /* vertical clock             */
#define PAPERS3_STH  GPIO_NUM_13   /* horizontal start pulse     */
#define PAPERS3_SPV  GPIO_NUM_17   /* vertical start pulse / STV */
#define PAPERS3_XLE  GPIO_NUM_15   /* latch enable               */

/* --- Power gates (push-pull MOSFETs to the panel HV supply) --- */
#define PAPERS3_EPD_EN  GPIO_NUM_45   /* main panel power enable */
#define PAPERS3_BST_EN  GPIO_NUM_46   /* HV boost converter enable */

static const lcd_bus_config_t papers3_lcd_config = {
    .clock       = PAPERS3_CKH,
    .ckv         = PAPERS3_CKV,
    .leh         = PAPERS3_XLE,
    .start_pulse = PAPERS3_STH,
    .stv         = PAPERS3_SPV,
    .data        = {
        PAPERS3_D0, PAPERS3_D1, PAPERS3_D2, PAPERS3_D3,
        PAPERS3_D4, PAPERS3_D5, PAPERS3_D6, PAPERS3_D7,
    },
};

static void power_pin_init(gpio_num_t pin)
{
    gpio_reset_pin(pin);
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, 0);
}

static void papers3_board_init(uint32_t epd_row_width,
                                const EpdInitConfig *init_config)
{
    (void)epd_row_width;
    (void)init_config;  /* PaperS3 has no I2C devices to share */

    /* If CKH was held across deep sleep release it before the LCD
     * peripheral re-claims the pin. Matches lilygo_board_s3. */
    gpio_hold_dis(PAPERS3_CKH);

    /* Bring up the power gate pins HIGH-Z (level 0). The actual
     * sequenced power-up happens in papers3_board_poweron(). */
    power_pin_init(PAPERS3_EPD_EN);
    power_pin_init(PAPERS3_BST_EN);

    const EpdDisplay_t *display = epd_get_display();
    LcdEpdConfig_t config = {
        .pixel_clock       = display->bus_speed * 1000 * 1000,
        .ckv_high_time     = 60,
        .line_front_porch  = 4,
        .le_high_time      = 4,
        .bus_width         = display->bus_width,
        .bus               = papers3_lcd_config,
    };
    epd_lcd_init(&config, display->width, display->height);
}

static void papers3_board_deinit(void)
{
    epd_lcd_deinit();
    /* Make sure the HV rail and boost are off so the panel does
     * not hold gate-driver voltages when idle. */
    gpio_set_level(PAPERS3_BST_EN, 0);
    gpio_set_level(PAPERS3_EPD_EN, 0);
}

static void papers3_board_set_ctrl(epd_ctrl_state_t *state,
                                    const epd_ctrl_state_t *const mask)
{
    /* PaperS3 has no I/O expander; the MODE / OE / STH / STV
     * lines that lilygo_board_s3 routes through a PCA9555 here
     * are driven directly by epdiy's LCD-peripheral output path
     * on this board. Nothing to do. */
    (void)state;
    (void)mask;
}

static void papers3_board_poweron(epd_ctrl_state_t *state)
{
    /* HV power-up sequence:
     *   1. Enable the main panel rail (EPD_EN).
     *   2. Wait for it to settle.
     *   3. Enable the HV boost converter (BST_EN).
     *   4. Wait for the gate-driver rails (+/-22 V / +15 V / -20 V)
     *      to come up before the LCD peripheral starts toggling
     *      data and clocks.
     *
     * Settling times are deliberately conservative; the panel
     * power network is RC-limited and the boost takes a few ms to
     * reach regulation. Matches the M5Unified PaperS3 driver. */
    gpio_set_level(PAPERS3_EPD_EN, 1);
    vTaskDelay(pdMS_TO_TICKS(5));
    gpio_set_level(PAPERS3_BST_EN, 1);
    vTaskDelay(pdMS_TO_TICKS(10));

    state->ep_stv          = true;
    state->ep_output_enable = true;
    state->ep_mode          = false;
}

static void papers3_board_poweroff(epd_ctrl_state_t *state)
{
    /* Reverse the sequence: boost off first so the gate-driver
     * rails collapse before the main panel power is removed. */
    gpio_set_level(PAPERS3_BST_EN, 0);
    esp_rom_delay_us(200);
    gpio_set_level(PAPERS3_EPD_EN, 0);

    state->ep_stv           = false;
    state->ep_output_enable = false;
    state->ep_mode          = false;
}

static float papers3_board_temperature(void)
{
    /* PaperS3 carries no panel-side temperature sensor. Return a
     * sane room-temperature default so epdiy's waveform-lookup
     * code can still pick a reasonable LUT.  25 deg C matches the
     * ED047TC1 waveform calibration point. */
    return 25.0f;
}

const EpdBoardDefinition epd_board_papers3 = {
    .init             = papers3_board_init,
    .deinit           = papers3_board_deinit,
    .set_ctrl         = papers3_board_set_ctrl,
    .poweron          = papers3_board_poweron,
    .measure_vcom     = NULL,   /* No TPS65185 -> no VCOM kick-back */
    .poweroff         = papers3_board_poweroff,
    .set_vcom         = NULL,   /* VCOM is fixed by the boost network */
    .get_temperature  = papers3_board_temperature,
    .gpio_set_direction = NULL,
    .gpio_read          = NULL,
    .gpio_write         = NULL,
};

#endif /* CONFIG_DRAFTLING_EPDIY_BOARD_PAPERS3 */
