#include "io_expander_m5ioe1.h"

#if defined(CONFIG_DRAFTLING_HAS_M5IOE1)

#include <driver/i2c_master.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *TAG = "M5IOE1";

#define M5IOE1_I2C_ADDR     0x4F

#define REG_UID_L           0x00
#define REG_GPIO_MODE_L     0x03
#define REG_GPIO_OUT_L      0x05
#define REG_GPIO_PU_L       0x09
#define REG_GPIO_PD_L       0x0B
#define REG_GPIO_DRV_L      0x13
#define REG_I2C_CFG         0x23

static i2c_master_dev_handle_t s_dev = NULL;

static esp_err_t read16(uint8_t reg, uint16_t *out)
{
    uint8_t rd[2] = { 0, 0 };
    esp_err_t err = i2c_master_transmit_receive(s_dev, &reg, 1, rd, 2, 100);
    if (err == ESP_OK) *out = (uint16_t)(rd[0] | (rd[1] << 8));
    return err;
}

static esp_err_t write16(uint8_t reg, uint16_t val)
{
    uint8_t wr[3] = { reg, (uint8_t)(val & 0xFF), (uint8_t)(val >> 8) };
    return i2c_master_transmit(s_dev, wr, sizeof(wr), 100);
}

/* Set (on = true) or clear the bit for one pin in a 16-bit register
 * pair, skipping the write when it is already in that state. */
static esp_err_t update16(uint8_t reg, uint16_t bit, bool on)
{
    uint16_t cur = 0;
    esp_err_t err = read16(reg, &cur);
    if (err != ESP_OK) return err;
    uint16_t next = on ? (uint16_t)(cur | bit) : (uint16_t)(cur & ~bit);
    return next == cur ? ESP_OK : write16(reg, next);
}

extern "C" esp_err_t m5ioe1_init(void *i2c_bus)
{
    if (s_dev) return ESP_OK;
    if (!i2c_bus) return ESP_ERR_INVALID_ARG;
    i2c_master_bus_handle_t bus = (i2c_master_bus_handle_t)i2c_bus;

    /* A START condition wakes the chip if it was left in I2C idle
     * sleep by earlier firmware (the same wake signal M5Stack's
     * library sends); retry for a moment while it comes up. */
    esp_err_t err = ESP_FAIL;
    for (int i = 0; i < 10 && err != ESP_OK; i++) {
        err = i2c_master_probe(bus, M5IOE1_I2C_ADDR, 20);
        if (err != ESP_OK) vTaskDelay(pdMS_TO_TICKS(20));
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "not responding at 0x%02X", M5IOE1_I2C_ADDR);
        return err;
    }

    i2c_device_config_t dev_cfg = {};
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address  = M5IOE1_I2C_ADDR;
    dev_cfg.scl_speed_hz    = 100000;
    err = i2c_master_bus_add_device(bus, &dev_cfg, &s_dev);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "bus_add_device failed: %s", esp_err_to_name(err));
        s_dev = NULL;
        return err;
    }

    /* Keep the chip awake: with an idle-sleep timeout set it would
     * swallow the first transaction after every quiet period. */
    uint8_t cfg[2] = { REG_I2C_CFG, 0x00 };
    err = i2c_master_transmit(s_dev, cfg, sizeof(cfg), 100);
    if (err != ESP_OK) ESP_LOGW(TAG, "I2C sleep disable failed: %s", esp_err_to_name(err));

    uint16_t uid = 0;
    read16(REG_UID_L, &uid);
    ESP_LOGI(TAG, "initialized at 0x%02X (uid 0x%04X)", M5IOE1_I2C_ADDR, uid);
    return ESP_OK;
}

extern "C" esp_err_t m5ioe1_set_pin(int pin, bool level)
{
    if (!s_dev) return ESP_ERR_INVALID_STATE;
    if (pin < 1 || pin > 14) return ESP_ERR_INVALID_ARG;
    uint16_t bit = (uint16_t)(1u << (pin - 1));

    /* Level first, so the pin does not glitch to the old output
     * level when it switches from input to output. */
    esp_err_t err = update16(REG_GPIO_OUT_L, bit, level);
    if (err == ESP_OK) err = update16(REG_GPIO_DRV_L, bit, false);
    if (err == ESP_OK) err = update16(REG_GPIO_PU_L, bit, false);
    if (err == ESP_OK) err = update16(REG_GPIO_PD_L, bit, false);
    if (err == ESP_OK) err = update16(REG_GPIO_MODE_L, bit, true);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "PYG%d <- %d failed: %s", pin, level ? 1 : 0, esp_err_to_name(err));
    }
    return err;
}

extern "C" esp_err_t m5ioe1_release_pin(int pin)
{
    if (!s_dev) return ESP_ERR_INVALID_STATE;
    if (pin < 1 || pin > 14) return ESP_ERR_INVALID_ARG;
    return update16(REG_GPIO_MODE_L, (uint16_t)(1u << (pin - 1)), false);
}

#else /* !CONFIG_DRAFTLING_HAS_M5IOE1 */

extern "C" esp_err_t m5ioe1_init(void *i2c_bus)
{
    (void)i2c_bus;
    return ESP_OK;
}

extern "C" esp_err_t m5ioe1_set_pin(int pin, bool level)
{
    (void)pin;
    (void)level;
    return ESP_OK;
}

extern "C" esp_err_t m5ioe1_release_pin(int pin)
{
    (void)pin;
    return ESP_OK;
}

#endif /* CONFIG_DRAFTLING_HAS_M5IOE1 */
