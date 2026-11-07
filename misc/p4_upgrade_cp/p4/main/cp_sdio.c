/*
 * Minimal SDIO host for the co-processor (wiring from board.h).
 * See cp_sdio.h for the register protocol.
 */
#include "cp_sdio.h"

#include <string.h>
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "sdmmc_cmd.h"
#include "board.h"

static const char *TAG = "cp_sdio";

/* Slave register offsets (only the low 10 bits are decoded). */
#define REG_TOKEN_RDATA   0x044
#define REG_INT_RAW       0x050
#define REG_PKT_LEN       0x060
#define REG_CONF_W7       0x08C
#define REG_INT_CLR       0x0D4

#define CMD53_END_ADDR    0x1F800
#define RX_BYTE_MAX       0x100000
#define RX_BYTE_MASK      0xFFFFF
#define TX_BUFFER_MAX     0x1000
#define TX_BUFFER_MASK    0xFFF
#define BLOCK_SIZE        512
#define INT_NEW_PACKET    (1u << 23)

#define FUNC1_EN_MASK     (1u << 1)

/* esp-hosted's settle time after the reset pulse. */
#define RESET_SETTLE_MS   1500

static sdmmc_card_t s_card;
static bool s_host_up;
static bool s_slot_up;
static int s_width;
static uint32_t s_rx_got;      /* bytes read so far, mod RX_BYTE_MAX */
static uint32_t s_tx_sent;     /* buffers used so far, mod TX_BUFFER_MAX */
static uint8_t *s_reg_buf;     /* DMA buffer for register access */

uint8_t *cp_sdio_alloc(size_t size)
{
    size = (size + 63) & ~(size_t)63;
    return heap_caps_aligned_calloc(64, 1, size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
}

esp_err_t cp_power_on(void)
{
    return board_cp_power_on();
}

void cp_reset(void)
{
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << BOARD_CP_PIN_RESET,
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);
    gpio_set_level(BOARD_CP_PIN_RESET, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(BOARD_CP_PIN_RESET, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(BOARD_CP_PIN_RESET, 1);
    vTaskDelay(pdMS_TO_TICKS(RESET_SETTLE_MS));
}

esp_err_t cp_sdio_host_init(int width)
{
    esp_err_t err;

    if (!s_reg_buf) {
        s_reg_buf = cp_sdio_alloc(64);
        if (!s_reg_buf) return ESP_ERR_NO_MEM;
    }
    if (!s_host_up) {
        err = sdmmc_host_init();
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
            ESP_LOGE(TAG, "sdmmc_host_init: %s", esp_err_to_name(err));
            return err;
        }
        s_host_up = true;
    }
    if (s_slot_up && s_width == width) return ESP_OK;
    if (s_slot_up) {
        sdmmc_host_deinit_slot(BOARD_CP_SDMMC_SLOT);
        s_slot_up = false;
    }

    sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
    slot.width = width;
    slot.clk = BOARD_CP_PIN_CLK;
    slot.cmd = BOARD_CP_PIN_CMD;
    slot.d0 = BOARD_CP_PIN_D0;
    if (width == 4) {
        slot.d1 = BOARD_CP_PIN_D1;
        slot.d2 = BOARD_CP_PIN_D2;
        slot.d3 = BOARD_CP_PIN_D3;
    } else {
        slot.d1 = GPIO_NUM_NC;
        slot.d2 = GPIO_NUM_NC;
        slot.d3 = GPIO_NUM_NC;
    }
    slot.d4 = slot.d5 = slot.d6 = slot.d7 = GPIO_NUM_NC;
    err = sdmmc_host_init_slot(BOARD_CP_SDMMC_SLOT, &slot);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "sdmmc_host_init_slot(%d-bit): %s", width, esp_err_to_name(err));
        return err;
    }
    s_slot_up = true;
    s_width = width;
    return ESP_OK;
}

static esp_err_t set_block_size(uint32_t fn, uint16_t bs)
{
    uint32_t base = (fn == 0) ? 0x10 : (0x100 * fn + 0x10);
    esp_err_t err = sdmmc_io_write_byte(&s_card, 0, base, bs & 0xFF, NULL);
    if (err == ESP_OK) err = sdmmc_io_write_byte(&s_card, 0, base + 1, bs >> 8, NULL);
    return err;
}

/* Enables function 1 and its interrupt, sets 512-byte blocks: the
 * sequence of the esp-hosted hosts (eh_host_port_sdio_card_fn_init). */
static esp_err_t card_fn_init(void)
{
    uint8_t ioe = 0, ior = 0, ie = 0;
    esp_err_t err = sdmmc_io_read_byte(&s_card, 0, SD_IO_CCCR_FN_ENABLE, &ioe);
    if (err != ESP_OK) return err;
    ioe |= FUNC1_EN_MASK;
    err = sdmmc_io_write_byte(&s_card, 0, SD_IO_CCCR_FN_ENABLE, ioe, &ioe);
    if (err != ESP_OK) return err;
    for (int i = 0; i < 100; i++) {
        err = sdmmc_io_read_byte(&s_card, 0, SD_IO_CCCR_FN_READY, &ior);
        if (err != ESP_OK) return err;
        if (ior & FUNC1_EN_MASK) break;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    if (!(ior & FUNC1_EN_MASK)) return ESP_ERR_TIMEOUT;
    err = sdmmc_io_read_byte(&s_card, 0, SD_IO_CCCR_INT_ENABLE, &ie);
    if (err != ESP_OK) return err;
    ie |= 1u | FUNC1_EN_MASK;
    err = sdmmc_io_write_byte(&s_card, 0, SD_IO_CCCR_INT_ENABLE, ie, NULL);
    if (err != ESP_OK) return err;
    err = set_block_size(0, BLOCK_SIZE);
    if (err == ESP_OK) err = set_block_size(1, BLOCK_SIZE);
    return err;
}

esp_err_t cp_sdio_card_init(int freq_khz, uint32_t timeout_ms)
{
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.slot = BOARD_CP_SDMMC_SLOT;
    host.max_freq_khz = freq_khz;
    /* Aligned bounce buffer for CMD53 byte mode (buffers behind cache). */
    host.flags |= SDMMC_HOST_FLAG_ALLOC_ALIGNED_BUF;
    host.flags &= ~SDMMC_HOST_FLAG_DDR;
    if (s_width == 1) {
        host.flags &= ~(SDMMC_HOST_FLAG_4BIT | SDMMC_HOST_FLAG_8BIT);
    }

    int64_t deadline = esp_timer_get_time() + (int64_t)timeout_ms * 1000;
    esp_err_t err;
    do {
        memset(&s_card, 0, sizeof(s_card));
        err = sdmmc_card_init(&host, &s_card);
        if (err == ESP_OK) break;
        vTaskDelay(pdMS_TO_TICKS(100));
    } while (esp_timer_get_time() < deadline);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "card init (%d-bit, %d kHz) failed: %s", s_width, freq_khz,
                 esp_err_to_name(err));
        return err;
    }
    err = card_fn_init();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "function 1 init failed: %s", esp_err_to_name(err));
        return err;
    }
    s_rx_got = 0;
    s_tx_sent = 0;
    ESP_LOGI(TAG, BOARD_CP_SHORT " card up: %d-bit, %d kHz", s_width, s_card.real_freq_khz);
    return ESP_OK;
}

static esp_err_t read_reg32(uint32_t reg, uint32_t *val)
{
    esp_err_t err = sdmmc_io_read_bytes(&s_card, 1, reg & 0x3FF, s_reg_buf, 4);
    if (err == ESP_OK) memcpy(val, s_reg_buf, 4);
    return err;
}

static esp_err_t write_reg32(uint32_t reg, uint32_t val)
{
    memcpy(s_reg_buf, &val, 4);
    return sdmmc_io_write_bytes(&s_card, 1, reg & 0x3FF, s_reg_buf, 4);
}

esp_err_t cp_sdio_send_intr(int intr_bit)
{
    return sdmmc_io_write_byte(&s_card, 1, REG_CONF_W7 & 0x3FF,
                               (uint8_t)(1u << intr_bit), NULL);
}

static esp_err_t tx_buffers_free(uint32_t *n)
{
    uint32_t token;
    esp_err_t err = read_reg32(REG_TOKEN_RDATA, &token);
    if (err != ESP_OK) return err;
    uint32_t latest = (token >> 16) & TX_BUFFER_MASK;
    *n = (latest + TX_BUFFER_MAX - s_tx_sent) % TX_BUFFER_MAX;
    return ESP_OK;
}

esp_err_t cp_sdio_send(const uint8_t *data, size_t len, uint32_t timeout_ms)
{
    if (!len || len > CP_SDIO_MAX_FRAME) return ESP_ERR_INVALID_ARG;

    int64_t deadline = esp_timer_get_time() + (int64_t)timeout_ms * 1000;
    uint32_t free_bufs = 0;
    for (;;) {
        esp_err_t err = tx_buffers_free(&free_bufs);
        if (err != ESP_OK) return err;
        if (free_bufs >= 1) break;
        if (esp_timer_get_time() >= deadline) return ESP_ERR_TIMEOUT;
        vTaskDelay(pdMS_TO_TICKS(2));
    }

    /* One frame always fits one slave buffer (1536 bytes). Whole blocks
     * first, then the rest in byte mode rounded up to 4 bytes; the
     * slave takes the length from the address (essl_sdio does this). */
    const uint8_t *p = data;
    size_t remain = len;
    while (remain) {
        size_t n;
        esp_err_t err;
        if (remain >= BLOCK_SIZE) {
            n = (remain / BLOCK_SIZE) * BLOCK_SIZE;
            err = sdmmc_io_write_blocks(&s_card, 1, CMD53_END_ADDR - remain, (void *)p, n);
        } else {
            n = remain;
            err = sdmmc_io_write_bytes(&s_card, 1, CMD53_END_ADDR - remain, (void *)p,
                                       (n + 3) & ~(size_t)3);
        }
        if (err != ESP_OK) return err;
        p += n;
        remain -= n;
    }
    s_tx_sent = (s_tx_sent + 1) % TX_BUFFER_MAX;
    return ESP_OK;
}

esp_err_t cp_sdio_recv(uint8_t *buf, size_t cap, size_t *got, uint32_t timeout_ms)
{
    *got = 0;
    int64_t deadline = esp_timer_get_time() + (int64_t)timeout_ms * 1000;
    uint32_t avail = 0;
    for (;;) {
        uint32_t pkt_len;
        esp_err_t err = read_reg32(REG_PKT_LEN, &pkt_len);
        if (err != ESP_OK) return err;
        avail = ((pkt_len & RX_BYTE_MASK) + RX_BYTE_MAX - s_rx_got) % RX_BYTE_MAX;
        if (avail) break;
        if (esp_timer_get_time() >= deadline) return ESP_ERR_TIMEOUT;
        vTaskDelay(pdMS_TO_TICKS(2));
    }

    /* Acknowledge the new-packet interrupt; the byte count above is
     * what decides how much to read. */
    uint32_t raw = 0;
    if (read_reg32(REG_INT_RAW, &raw) == ESP_OK && (raw & INT_NEW_PACKET)) {
        write_reg32(REG_INT_CLR, INT_NEW_PACKET);
    }

    size_t want = avail;
    if (want > cap) want = cap & ~(size_t)3;
    uint8_t *p = buf;
    size_t remain = want;
    while (remain) {
        size_t n;
        esp_err_t err;
        if (remain >= BLOCK_SIZE) {
            n = (remain / BLOCK_SIZE) * BLOCK_SIZE;
            err = sdmmc_io_read_blocks(&s_card, 1, CMD53_END_ADDR - remain, p, n);
        } else {
            n = remain;
            err = sdmmc_io_read_bytes(&s_card, 1, CMD53_END_ADDR - remain, p,
                                      (n + 3) & ~(size_t)3);
        }
        if (err != ESP_OK) return err;
        p += n;
        remain -= n;
        s_rx_got = (s_rx_got + n) % RX_BYTE_MAX;
    }
    *got = want;
    return ESP_OK;
}
