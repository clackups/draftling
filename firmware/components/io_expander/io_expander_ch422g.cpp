#include "sdkconfig.h"
#include "io_expander_ch422g.h"

#if !defined(CONFIG_DRAFTLING_HAS_CH422G)
/* Stub for boards without a CH422G expander. */

extern "C" esp_err_t ch422g_init(void * /*i2c_bus*/)      { return ESP_OK; }
extern "C" void      ch422g_set_pin(int /*pin*/, bool /*level*/) { /* no-op */ }

#else /* CONFIG_DRAFTLING_HAS_CH422G */

#include <esp_log.h>
#include <driver/i2c_master.h>

static const char *TAG = "CH422G";

/*
 * Each CH422G "register" is a distinct I2C slave address (7-bit,
 * already shifted from the datasheet's 8-bit values: 0x48>>1, etc).
 * Only the two registers Draftling needs are implemented -- WR_OC
 * (open-drain-only EXIO8..11) and RD_IO (digital input read) are
 * unused because every pin Draftling drives on this chip is a
 * plain push-pull output in EXIO0..7. */
#define CH422G_ADDR_WR_SET  0x24   /* system/mode register */
#define CH422G_ADDR_WR_IO   0x38   /* EXIO0..7 output level */

#define CH422G_WR_SET_IO_OE (1U << 0)  /* 1 = all EXIO0..7 are outputs */

static i2c_master_dev_handle_t s_dev_wr_set = NULL;
static i2c_master_dev_handle_t s_dev_wr_io  = NULL;
/* Shadow of the EXIO0..7 output byte. The CH422G has no
 * read-modify-write, so every ch422g_set_pin() call re-sends this
 * full byte. POR default is 0xFF (all high). */
static uint8_t s_output_shadow = 0xFF;

extern "C" esp_err_t ch422g_init(void *i2c_bus)
{
    if (s_dev_wr_io) return ESP_OK;   /* already initialized */
    if (!i2c_bus) {
        ESP_LOGE(TAG, "ch422g_init: NULL I2C bus handle");
        return ESP_ERR_INVALID_ARG;
    }
    i2c_master_bus_handle_t bus = (i2c_master_bus_handle_t)i2c_bus;

    i2c_device_config_t set_cfg = {};
    set_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    set_cfg.device_address  = CH422G_ADDR_WR_SET;
    set_cfg.scl_speed_hz    = 400000;
    esp_err_t err = i2c_master_bus_add_device(bus, &set_cfg, &s_dev_wr_set);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "add WR_SET device failed: %s", esp_err_to_name(err));
        return err;
    }

    i2c_device_config_t io_cfg = {};
    io_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    io_cfg.device_address  = CH422G_ADDR_WR_IO;
    io_cfg.scl_speed_hz    = 400000;
    err = i2c_master_bus_add_device(bus, &io_cfg, &s_dev_wr_io);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "add WR_IO device failed: %s", esp_err_to_name(err));
        i2c_master_bus_rm_device(s_dev_wr_set);
        s_dev_wr_set = NULL;
        return err;
    }

    /* Switch all 8 EXIO pins to push-pull outputs. */
    uint8_t set_byte = CH422G_WR_SET_IO_OE;
    err = i2c_master_transmit(s_dev_wr_set, &set_byte, 1, 100);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "WR_SET write failed: %s", esp_err_to_name(err));
        return err;
    }

    /* Sync the output shadow (POR default, all high) to the chip. */
    err = i2c_master_transmit(s_dev_wr_io, &s_output_shadow, 1, 100);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "WR_IO write failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "CH422G initialized (EXIO0..7 outputs, WR_SET=0x%02X WR_IO=0x%02X)",
             CH422G_ADDR_WR_SET, CH422G_ADDR_WR_IO);
    return ESP_OK;
}

extern "C" void ch422g_set_pin(int pin, bool level)
{
    if (!s_dev_wr_io || pin < 0 || pin > 7) return;

    if (level) {
        s_output_shadow |= (uint8_t)(1U << pin);
    } else {
        s_output_shadow &= (uint8_t)~(1U << pin);
    }
    esp_err_t err = i2c_master_transmit(s_dev_wr_io, &s_output_shadow, 1, 100);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "EXIO%d write failed: %s", pin, esp_err_to_name(err));
    }
}

#endif /* CONFIG_DRAFTLING_HAS_CH422G */
