#include "io_expander_pca9557.h"

#if defined(CONFIG_DRAFTLING_HAS_PCA9557)

#include <driver/i2c_master.h>
#include <esp_log.h>

static const char *TAG = "PCA9557";

#define REG_OUTPUT  0x01
#define REG_CONFIG  0x03

static i2c_master_dev_handle_t s_dev = NULL;

static esp_err_t read_reg(uint8_t reg, uint8_t *out)
{
    return i2c_master_transmit_receive(s_dev, &reg, 1, out, 1, 100);
}

static esp_err_t write_reg(uint8_t reg, uint8_t val)
{
    uint8_t wr[2] = { reg, val };
    return i2c_master_transmit(s_dev, wr, sizeof(wr), 100);
}

/* Set (on = true) or clear one bit of a register. */
static esp_err_t update_bit(uint8_t reg, int pin, bool on)
{
    uint8_t cur = 0;
    esp_err_t err = read_reg(reg, &cur);
    if (err != ESP_OK) return err;
    uint8_t bit  = (uint8_t)(1u << pin);
    uint8_t next = on ? (uint8_t)(cur | bit) : (uint8_t)(cur & ~bit);
    return next == cur ? ESP_OK : write_reg(reg, next);
}

extern "C" esp_err_t pca9557_init(void *i2c_bus, uint8_t addr)
{
    if (s_dev) return ESP_OK;
    if (!i2c_bus) return ESP_ERR_INVALID_ARG;
    i2c_master_bus_handle_t bus = (i2c_master_bus_handle_t)i2c_bus;

    if (i2c_master_probe(bus, addr, 50) != ESP_OK) {
        ESP_LOGI(TAG, "not found at 0x%02X", addr);
        return ESP_ERR_NOT_FOUND;
    }

    i2c_device_config_t dev_cfg = {};
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address  = addr;
    dev_cfg.scl_speed_hz    = 400000;
    esp_err_t err = i2c_master_bus_add_device(bus, &dev_cfg, &s_dev);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "bus_add_device failed: %s", esp_err_to_name(err));
        s_dev = NULL;
        return err;
    }
    ESP_LOGI(TAG, "found at 0x%02X", addr);
    return ESP_OK;
}

extern "C" bool pca9557_is_present(void)
{
    return s_dev != NULL;
}

extern "C" esp_err_t pca9557_set_pin(int pin, bool level)
{
    if (!s_dev) return ESP_ERR_INVALID_STATE;
    if (pin < 0 || pin > 7) return ESP_ERR_INVALID_ARG;
    /* Level first, so the pin does not glitch to the old output level
     * when it switches from input to output. */
    esp_err_t err = update_bit(REG_OUTPUT, pin, level);
    if (err == ESP_OK) err = update_bit(REG_CONFIG, pin, false);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "IO%d <- %d failed: %s", pin, level ? 1 : 0, esp_err_to_name(err));
    }
    return err;
}

extern "C" esp_err_t pca9557_release_pin(int pin)
{
    if (!s_dev) return ESP_ERR_INVALID_STATE;
    if (pin < 0 || pin > 7) return ESP_ERR_INVALID_ARG;
    return update_bit(REG_CONFIG, pin, true);
}

#else /* !CONFIG_DRAFTLING_HAS_PCA9557 */

extern "C" esp_err_t pca9557_init(void *i2c_bus, uint8_t addr)
{
    (void)i2c_bus;
    (void)addr;
    return ESP_OK;
}

extern "C" bool pca9557_is_present(void)
{
    return false;
}

extern "C" esp_err_t pca9557_set_pin(int pin, bool level)
{
    (void)pin;
    (void)level;
    return ESP_OK;
}

extern "C" esp_err_t pca9557_release_pin(int pin)
{
    (void)pin;
    return ESP_OK;
}

#endif /* CONFIG_DRAFTLING_HAS_PCA9557 */
