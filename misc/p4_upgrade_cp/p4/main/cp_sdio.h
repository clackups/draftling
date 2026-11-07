/*
 * Minimal SDIO host for the co-processor (wiring from board.h).
 *
 * Every esp-hosted generation (0.0.x, 1.x, 2.x, 3.x) runs the C6's
 * hardware SDIO slave through ESP-IDF's sdio_slave driver, so the
 * register protocol below is the same for all of them; it is the one
 * ESP-IDF's esp_serial_slave_link (essl_sdio) implements:
 *
 *   - PKT_LEN (0x60): running count of bytes the slave has queued for
 *     the host (20 bits); the host keeps its own count of bytes read.
 *   - TOKEN_RDATA (0x44), bits 27..16: running count of receive buffers
 *     the slave has made available (12 bits).
 *   - Data is read from / written to function 1 at 0x1F800 - length.
 *   - CONF_W7 (0x8C): one bit per host->slave interrupt; esp-hosted uses
 *     bit 0 = open data path, bit 1 = close data path, bit 2 = reset.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* esp-hosted host->slave interrupts (CONF_W7 bits). */
#define CP_INTR_OPEN_DATA_PATH   0
#define CP_INTR_CLOSE_DATA_PATH  1
#define CP_INTR_RESET            2

/* Largest frame either side sends (ESP_TRANSPORT_SDIO_MAX_BUF_SIZE). */
#define CP_SDIO_MAX_FRAME  1536

/* Switches the co-processor supply on (board_cp_power_on()). */
esp_err_t cp_power_on(void);

/* Pulses the C6's EN line, as the esp-hosted hosts do. */
void cp_reset(void);

/* Sets up the SDMMC host and slot 1 with the given bus width (1 or 4).
 * Calling it again with another width re-initialises the slot. */
esp_err_t cp_sdio_host_init(int width);

/* Initialises the C6 as an SDIO card at the given clock and enables
 * function 1, as the esp-hosted hosts do. Retries for up to timeout_ms
 * (the C6 needs a moment after reset). Resets the byte counters. */
esp_err_t cp_sdio_card_init(int freq_khz, uint32_t timeout_ms);

/* Sends a host->slave interrupt (CP_INTR_*). */
esp_err_t cp_sdio_send_intr(int intr_bit);

/* Sends one frame (at most CP_SDIO_MAX_FRAME bytes), waiting up to
 * timeout_ms for the slave to have a receive buffer. */
esp_err_t cp_sdio_send(const uint8_t *data, size_t len, uint32_t timeout_ms);

/* Reads whatever the slave has queued, up to cap bytes, waiting up to
 * timeout_ms for data. *got is 0 on timeout (returns ESP_ERR_TIMEOUT).
 * The data may hold several frames back to back. */
esp_err_t cp_sdio_recv(uint8_t *buf, size_t cap, size_t *got, uint32_t timeout_ms);

/* DMA-capable buffer for cp_sdio_send() / cp_sdio_recv(). */
uint8_t *cp_sdio_alloc(size_t size);

#ifdef __cplusplus
}
#endif
