/*
 * Battery monitor.
 *
 * Two backends are currently supported:
 *
 *   - ADC + resistive divider (battery_init):
 *       * Waveshare ESP32-S3-RLCD-4.2: GPIO4 (ADC1_CH3) with a 3:1
 *         resistive divider (200 K + 100 K), no enable pin.
 *       * M5Stack PaperS3: GPIO3 (ADC1_CH2) with a 2:1 resistive
 *         divider, no enable pin (matches the M5Unified Power_Class
 *         configuration for board_M5PaperS3: BAT_ADC = ADC1_GPIO3,
 *         adc_ratio = 2.0).
 *       The ESP-IDF ADC oneshot driver with curve-fitting calibration
 *       converts the raw ADC reading to millivolts at the pin, and the
 *       divider ratio recovers the actual cell voltage. An optional
 *       active-high enable GPIO is supported for boards that gate the
 *       divider through a transistor.
 *
 *   - TI BQ27220 fuel gauge over I2C (battery_init_bq27220):
 *       * LilyGO T5 E-Paper S3 Pro / Pro Lite: BQ27220YZFR at 0x55 on
 *         the same I2C bus that carries the GT911 touch controller
 *         and the EPDIY-owned TPS65185 / PCA9535 expander.
 *       The caller passes in an already-created
 *       i2c_master_bus_handle_t (the bus is shared with epdiy + GT911,
 *       so the battery component must not own it). Voltage comes from
 *       the 16-bit Voltage register at 0x08 (mV); percentage is then
 *       derived from that voltage via the same LiPo discharge LUT
 *       used by the ADC backend, because the BQ27220's StateOfCharge
 *       register at 0x2C ships with default Data Memory on the T5
 *       Pro and reports a meaningless ~50 % regardless of actual
 *       cell voltage, even after several discharge/charge cycles.
 */

#include "battery.h"

#include <climits>
#include <cstdint>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <driver/gpio.h>
#include <driver/i2c_master.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>

static const char *TAG = "Battery";

/* ---- backend selection ---- */
enum batt_backend {
    BATT_BACKEND_NONE = 0,
    BATT_BACKEND_ADC,
    BATT_BACKEND_BQ27220,
    BATT_BACKEND_INA226,
    BATT_BACKEND_CW2017,
};
static enum batt_backend s_backend = BATT_BACKEND_NONE;

/* ---- ADC backend state ---- */
static adc_oneshot_unit_handle_t s_adc_handle = NULL;
static adc_cali_handle_t         s_cali_handle = NULL;
static adc_channel_t             s_channel = ADC_CHANNEL_3;
static int                       s_enable_gpio = -1;
static int                       s_divider = 3;

/* ---- BQ27220 backend state ---- */
#define BQ27220_I2C_ADDR        0x55
#define BQ27220_REG_VOLTAGE     0x08    /* u16 LE, mV */
#define BQ27220_REG_FLAGS       0x06    /* u16 LE, bit 0 = DSG */
#define BQ27220_FLAGS_DSG       0x0001  /* 0 = charging or full */
#define BQ27220_REG_CURRENT     0x0C    /* s16 LE, mA -- signed two's complement,
                                         * + = into cell, - = out of cell */
#define BQ27220_REG_SOC         0x2C    /* u16 LE, 0-100 % */
static i2c_master_dev_handle_t   s_bq_dev = NULL;

/* ---- CW2017 backend state ----
 * CellWise CW2017 fuel gauge, used on the Xteink X4 Pro (I2C address
 * 0x63, no charger IC on the same bus). Register map, reset sequence
 * and the 80-byte BATINFO battery profile were recovered by the
 * FreeInk SDK (github.com/Free-Ink/freeink-sdk, MIT licensed) from
 * the OEM firmware's Cw2017PowerHal class. Unlike BQ27220, the CW2017
 * reports SOC (register 0x04) as a direct integer percentage once a
 * matching battery profile is resident -- it reports 0 % until then,
 * so battery_init_cw2017() must verify/upload the profile before any
 * reading is meaningful. */
#define CW2017_REG_VERSION   0x00  /* running state in (ver & 0xFD) == 0x0D */
#define CW2017_REG_VCELL_H   0x02  /* 14-bit VCELL, big-endian over 0x02/0x03 */
#define CW2017_REG_SOC       0x04  /* integer percent, 0..100 */
#define CW2017_REG_MODE      0x08  /* soft-reset / sleep control */
#define CW2017_REG_CONFIG    0x0B  /* bit7 = profile-loaded / update-enable */
#define CW2017_REG_BATINFO   0x10  /* 80-byte profile spans 0x10..0x5F */
#define CW2017_BATINFO_LEN   80

/* The exact BATINFO profile the OEM uploads for the X4 Pro's cell.
 * Battery-model-specific; do not reuse for a different pack. */
static const uint8_t CW2017_BATINFO[CW2017_BATINFO_LEN] = {
    0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xaa, 0xbf, 0xb5, 0xb4, 0xa4, 0x9c, 0xeb, 0xe2,
    0xdf, 0xe5, 0xca, 0xa0, 0x8a, 0x62, 0x53, 0x48, 0x40, 0x3a, 0x32, 0xb1, 0xae, 0xda, 0xb5, 0xff,
    0xff, 0xff, 0xe8, 0xdb, 0xd9, 0xd6, 0xd4, 0xd2, 0xd0, 0xcb, 0xc3, 0xbc, 0x9e, 0x87, 0x7b, 0x71,
    0x72, 0x7c, 0x8c, 0xa3, 0xb7, 0xc8, 0xa5, 0x4f, 0x00, 0x00, 0xab, 0x02, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x23,
};
#define CW2017_I2C_ADDR_DEFAULT 0x63
static i2c_master_dev_handle_t   s_cw2017_dev = NULL;

/* ---- INA226 backend state ----
 * Only the bus-voltage and shunt-voltage registers are used.
 * INA226_REG_BUSVOLTAGE is a 16-bit unsigned value with an LSB of
 * 1.25 mV (datasheet section 7.6.3.3); reading 0x0CCC -> 4.095 V at
 * the BUS pin. The Tab5 routes the raw battery pack rail (NP-F550,
 * 2S Li-ion, ~6.0-8.4 V) to the BUS input, so the per-cell voltage
 * is bus_mv / cells.
 *
 * INA226_REG_SHUNTVOLTAGE is a signed 16-bit register with a 2.5 uV
 * LSB. Its sign indicates the direction of current flow across the
 * shunt and is therefore enough to detect "charging vs discharging"
 * without writing the calibration register (which would require the
 * exact shunt-resistor value from the schematic to convert the raw
 * value to amperes). On the Tab5 the IP2326 charger sits between
 * the USB-C input and the cell and the INA226 shunt is on the cell-
 * side rail. Empirically the shunt's IN+/IN- inputs are wired such
 * that current flowing *out* of the cell (i.e. the system running
 * on battery) produces a positive shunt-voltage reading, and current
 * flowing *into* the cell while charging produces a negative one --
 * so on the Tab5 a negative shunt voltage means "charging". A small
 * noise threshold filters out the few-microvolt offset reading the
 * INA226 returns when no current is flowing. */
#define INA226_REG_SHUNTVOLTAGE  0x01
#define INA226_REG_BUSVOLTAGE    0x02
#define INA226_BUSVOLTAGE_LSB_UV 1250    /* microvolts per LSB */
/* Noise threshold for charge detection, in raw LSB units of the
 * shunt-voltage register (2.5 uV/LSB). 8 LSB == 20 uV across the
 * shunt; with the typical 10 mohm Tab5 shunt that is ~2 mA, well
 * below idle MCU draw and well above the INA226 zero-offset. */
#define INA226_SHUNT_CHARGE_THRESHOLD 8
/* Per-cell voltage threshold below which the INA226 backend treats
 * the pack as "no battery attached". A real Li-ion cell is fully
 * discharged at ~3.0 V and the protection circuit cuts off well
 * before that, so anything below ~2.8 V/cell on the BUS rail means
 * the pack is missing (the rail then floats / leaks to a noisy few
 * volts -- on a 2S Tab5 we observed ~4-5 V total, which the
 * Li-ion discharge curve would otherwise misread as ~14-18 %). */
#define INA226_BATTERY_MIN_MV_PER_CELL 2800
static i2c_master_dev_handle_t   s_ina226_dev = NULL;
static int                       s_ina226_cells = 1;

/* Read the INA226 BUS voltage and return the per-cell value in mV,
 * or -1 if the device is not initialized or the I2C read fails. */
static int ina226_read_cell_mv(void)
{
    if (!s_ina226_dev) return -1;
    uint8_t wr[1] = { INA226_REG_BUSVOLTAGE };
    uint8_t rd[2] = { 0, 0 };
    if (i2c_master_transmit_receive(s_ina226_dev, wr, 1, rd, 2,
                                    100 /* ms */) != ESP_OK) {
        return -1;
    }
    /* Big-endian on the wire. Convert raw LSBs to mV per cell. */
    uint16_t raw = ((uint16_t)rd[0] << 8) | (uint16_t)rd[1];
    uint32_t bus_uv = (uint32_t)raw * INA226_BUSVOLTAGE_LSB_UV;
    int bus_mv = (int)(bus_uv / 1000U);
    int cells = (s_ina226_cells > 0) ? s_ina226_cells : 1;
    return bus_mv / cells;
}

/* True when the INA226 BUS rail looks like no pack is attached. */
static bool ina226_battery_absent(int cell_mv)
{
    return cell_mv < INA226_BATTERY_MIN_MV_PER_CELL;
}

/* Exponential moving average state. Used by the ADC backend always,
 * and by the BQ27220 backend to dampen short-term IR-drop sag in the
 * gauge's raw Voltage register so the displayed percentage does not
 * cliff-step from 100 % down by ~10 percentage points the moment the
 * USB cable is unplugged and the e-paper / radio load pulls the cell
 * below its post-charge resting voltage. */
#define SMOOTH_SAMPLES 8
static uint32_t s_smooth_acc = 0;   /* accumulated mV * SMOOTH_SAMPLES */
static uint32_t s_bq27220_smooth_acc = 0;
static bool     s_bq27220_smooth_primed = false;

/* Approximate max voltage (mV) at the ADC pin with 12 dB attenuation */
#define ADC_VREF_MV 3100

/* ---- voltage-to-percent look-up (ADC backend) ----
 * Based on community LiPo discharge measurements for the board.
 * The table defines key-points; mv_to_percent() linearly interpolates
 * between adjacent entries for a smooth percentage value. */
struct batt_step { int mv; int pct; };
static const batt_step BATT_TABLE[] = {
    { 4100, 100 },
    { 3950,  75 },
    { 3800,  50 },
    { 3600,  25 },
    {    0,   0 },
};

static const int BATT_TABLE_N = sizeof(BATT_TABLE) / sizeof(BATT_TABLE[0]);

/* ---- Charging-time IR correction ----
 * The voltage-to-percent LUT above is an *open-circuit* / rest curve.
 * While the BQ25896 is actively pushing ~1 A into the cell, terminal
 * voltage runs ~150-250 mV above OCV due to the pack's internal
 * resistance, and in the CV taper it is pinned near Vreg (~4.2 V)
 * regardless of true SoC. Feeding that into mv_to_percent() makes a
 * cell that was sitting at ~3.95 V / ~89 % jump straight to 100 %
 * the moment USB is plugged in.
 *
 * 180 mOhm is a midpoint for a typical 1S 18650 / pouch pack in this
 * board's form factor. The correction is clamped (CORR_MAX_MV) so a
 * bogus / runaway current reading cannot drag the displayed percentage
 * arbitrarily low. */
#define BATT_INTERNAL_R_MOHM    180
#define BATT_IR_CORR_MAX_MV     300

static int mv_to_percent(int mv)
{
    /* Above the highest entry -> cap at 100 % */
    if (mv >= BATT_TABLE[0].mv)
        return BATT_TABLE[0].pct;

    /* Walk the table and linearly interpolate between adjacent entries */
    for (int i = 0; i < BATT_TABLE_N - 1; ++i) {
        int mv_lo  = BATT_TABLE[i + 1].mv;
        if (mv >= mv_lo) {
            int mv_hi  = BATT_TABLE[i].mv;
            int pct_hi = BATT_TABLE[i].pct;
            int pct_lo = BATT_TABLE[i + 1].pct;
            return pct_lo + (pct_hi - pct_lo) * (mv - mv_lo) / (mv_hi - mv_lo);
        }
    }
    return 0;
}

/* Voltage divider ratio default (overridden in battery_init) */
#define DIVIDER_RATIO_DEFAULT  3

/* Map GPIO number to ADC1 channel.  Only a handful of pins
 * are wired to ADC1 on the ESP32-S3; we support the ones
 * commonly used by the supported boards. */
static adc_channel_t gpio_to_channel(int gpio)
{
    switch (gpio) {
    case 1:  return ADC_CHANNEL_0;
    case 2:  return ADC_CHANNEL_1;
    case 3:  return ADC_CHANNEL_2;
    case 4:  return ADC_CHANNEL_3;
    case 5:  return ADC_CHANNEL_4;
    case 6:  return ADC_CHANNEL_5;
    case 7:  return ADC_CHANNEL_6;
    case 8:  return ADC_CHANNEL_7;
    case 9:  return ADC_CHANNEL_8;
    case 10: return ADC_CHANNEL_9;
    default:
        ESP_LOGW(TAG, "Unsupported battery GPIO%d, falling back to CH3 (GPIO4)", gpio);
        return ADC_CHANNEL_3;
    }
}

/* Pulse the enable GPIO on (if any), let the divider settle, return whether
 * we actually toggled it (so the caller can turn it off afterwards). */
static bool enable_divider(void)
{
    if (s_enable_gpio < 0) return false;
    gpio_set_level((gpio_num_t)s_enable_gpio, 1);
    /* Allow the divider a few ms to settle once powered. */
    vTaskDelay(pdMS_TO_TICKS(5));
    return true;
}

static void disable_divider(bool was_enabled)
{
    if (was_enabled && s_enable_gpio >= 0) {
        gpio_set_level((gpio_num_t)s_enable_gpio, 0);
    }
}

/* ---- public API ---- */

extern "C" int battery_init(int gpio_num, int enable_gpio, int divider)
{
    if (s_backend != BATT_BACKEND_NONE) return 0;

    /* Boards with no on-board battery monitor pass gpio_num < 0 (e.g. the
     * bare Waveshare E-Paper Driver HAT). Skip ADC setup entirely;
     * battery_read_mv()/battery_read_percent() will then return 0 / -1
     * and the editor UI will hide the icon. */
    if (gpio_num < 0) {
        ESP_LOGI(TAG, "Battery monitor disabled (no ADC pin configured)");
        return 0;
    }

    s_channel     = gpio_to_channel(gpio_num);
    s_enable_gpio = enable_gpio;
    s_divider     = (divider > 0) ? divider : DIVIDER_RATIO_DEFAULT;

    /* Configure the optional enable GPIO as an output and leave it driven
     * HIGH so the very first reading after init is valid. */
    if (s_enable_gpio >= 0) {
        gpio_config_t en_cfg = {};
        en_cfg.intr_type    = GPIO_INTR_DISABLE;
        en_cfg.mode         = GPIO_MODE_OUTPUT;
        en_cfg.pin_bit_mask = (1ULL << s_enable_gpio);
        en_cfg.pull_up_en   = GPIO_PULLUP_DISABLE;
        en_cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
        if (gpio_config(&en_cfg) != ESP_OK) {
            ESP_LOGW(TAG, "Battery enable GPIO%d config failed", s_enable_gpio);
        }
        gpio_set_level((gpio_num_t)s_enable_gpio, 1);
    }

    /* Create oneshot ADC unit */
    adc_oneshot_unit_init_cfg_t unit_cfg = {};
    unit_cfg.unit_id  = ADC_UNIT_1;
    unit_cfg.ulp_mode = ADC_ULP_MODE_DISABLE;

    esp_err_t err = adc_oneshot_new_unit(&unit_cfg, &s_adc_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ADC unit init failed: %s", esp_err_to_name(err));
        return -1;
    }

    /* Configure channel */
    adc_oneshot_chan_cfg_t chan_cfg = {};
    chan_cfg.atten    = ADC_ATTEN_DB_12;
    chan_cfg.bitwidth = ADC_BITWIDTH_12;

    err = adc_oneshot_config_channel(s_adc_handle, s_channel, &chan_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ADC channel config failed: %s", esp_err_to_name(err));
        return -1;
    }

    /* Try to create calibration scheme (curve fitting on ESP32-S3) */
    adc_cali_curve_fitting_config_t cali_cfg = {};
    cali_cfg.unit_id  = ADC_UNIT_1;
    cali_cfg.chan     = s_channel;
    cali_cfg.atten   = ADC_ATTEN_DB_12;
    cali_cfg.bitwidth = ADC_BITWIDTH_12;

    err = adc_cali_create_scheme_curve_fitting(&cali_cfg, &s_cali_handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "ADC calibration not available, using raw conversion");
        s_cali_handle = NULL;
    }

    /* Seed the smoother with the first real reading */
    bool en = enable_divider();
    int raw = 0;
    adc_oneshot_read(s_adc_handle, s_channel, &raw);
    disable_divider(en);

    int pin_mv = 0;
    if (s_cali_handle) {
        adc_cali_raw_to_voltage(s_cali_handle, raw, &pin_mv);
    } else {
        pin_mv = (raw * ADC_VREF_MV) / 4095;
    }
    int bat_mv = pin_mv * s_divider;
    s_smooth_acc = (uint32_t)bat_mv * SMOOTH_SAMPLES;

    s_backend = BATT_BACKEND_ADC;
    ESP_LOGI(TAG, "Battery monitor initialized on GPIO%d (ch%d, div=%d, en=%d), "
             "initial %d mV", gpio_num, (int)s_channel, s_divider,
             s_enable_gpio, bat_mv);
    return 0;
}

/* ---- BQ27220 backend ---- */

static int bq27220_read_u16(uint8_t reg, uint16_t *out)
{
    if (!s_bq_dev) return -1;
    uint8_t wr[1] = { reg };
    uint8_t rd[2] = { 0, 0 };
    esp_err_t err = i2c_master_transmit_receive(s_bq_dev, wr, 1, rd, 2,
                                                100 /* ms */);
    if (err != ESP_OK) return -1;
    *out = (uint16_t)rd[0] | ((uint16_t)rd[1] << 8);
    return 0;
}

extern "C" int battery_init_bq27220(void *i2c_master_bus)
{
    if (s_backend != BATT_BACKEND_NONE) return 0;

    if (i2c_master_bus == NULL) {
        ESP_LOGW(TAG, "BQ27220 init: NULL I2C bus handle");
        return -1;
    }

    i2c_master_bus_handle_t bus = (i2c_master_bus_handle_t)i2c_master_bus;

    /* Probe the device first so an absent / dead fuel gauge does not
     * leave a half-attached handle behind. */
    if (i2c_master_probe(bus, BQ27220_I2C_ADDR, 100 /* ms */) != ESP_OK) {
        ESP_LOGW(TAG, "BQ27220 not responding at 0x%02X", BQ27220_I2C_ADDR);
        return -1;
    }

    i2c_device_config_t dev_cfg = {};
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address  = BQ27220_I2C_ADDR;
    dev_cfg.scl_speed_hz    = 400000;
    esp_err_t err = i2c_master_bus_add_device(bus, &dev_cfg, &s_bq_dev);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "BQ27220 bus_add_device failed: %s",
                 esp_err_to_name(err));
        s_bq_dev = NULL;
        return -1;
    }

    s_backend = BATT_BACKEND_BQ27220;

    uint16_t mv = 0, soc = 0;
    bq27220_read_u16(BQ27220_REG_VOLTAGE, &mv);
    bq27220_read_u16(BQ27220_REG_SOC, &soc);
    ESP_LOGI(TAG, "BQ27220 fuel gauge initialized at 0x%02X "
             "(initial %u mV, %u%%)", BQ27220_I2C_ADDR,
             (unsigned)mv, (unsigned)soc);
    return 0;
}

/* ---- CW2017 backend ---- */

static int cw2017_read_u8(uint8_t reg, uint8_t *out)
{
    if (!s_cw2017_dev) return -1;
    uint8_t wr[1] = { reg };
    uint8_t rd[1] = { 0 };
    esp_err_t err = i2c_master_transmit_receive(s_cw2017_dev, wr, 1, rd, 1,
                                                100 /* ms */);
    if (err != ESP_OK) return -1;
    *out = rd[0];
    return 0;
}

static int cw2017_write_u8(uint8_t reg, uint8_t val)
{
    if (!s_cw2017_dev) return -1;
    uint8_t wr[2] = { reg, val };
    return (i2c_master_transmit(s_cw2017_dev, wr, 2, 100 /* ms */) == ESP_OK) ? 0 : -1;
}

/* Soft-reset sequence: MODE 0xF0 -> 0x30 -> 0x00, 20 ms apart (ported
 * from the OEM firmware's FUN_4215042c via the FreeInk SDK). */
static void cw2017_reset(void)
{
    cw2017_write_u8(CW2017_REG_MODE, 0xF0);
    vTaskDelay(pdMS_TO_TICKS(20));
    cw2017_write_u8(CW2017_REG_MODE, 0x30);
    vTaskDelay(pdMS_TO_TICKS(20));
    cw2017_write_u8(CW2017_REG_MODE, 0x00);
    vTaskDelay(pdMS_TO_TICKS(20));
}

/* One-shot: make sure a valid BATINFO profile is resident. Wakes /
 * resets the gauge if it is not running, then uploads and enables
 * the profile only when the resident bytes do not already match (the
 * gauge normally keeps it resident across warm boots, so this is
 * usually a verify-only readback). Bounded polling so a cold gauge
 * cannot stall boot. */
static void cw2017_ensure_profile(void)
{
    uint8_t ver = 0;
    if (cw2017_read_u8(CW2017_REG_VERSION, &ver) != 0) return; /* gauge absent */
    if ((ver & 0xFD) != 0x0D) cw2017_reset();

    uint8_t cfg = 0;
    if (cw2017_read_u8(CW2017_REG_CONFIG, &cfg) == 0 && (cfg & 0x80)) {
        bool match = true;
        for (uint8_t i = 0; i < CW2017_BATINFO_LEN; i++) {
            uint8_t b = 0;
            if (cw2017_read_u8((uint8_t)(CW2017_REG_BATINFO + i), &b) != 0 ||
                b != CW2017_BATINFO[i]) {
                match = false;
                break;
            }
        }
        if (match) return; /* correct profile already loaded */
    }

    for (uint8_t i = 0; i < CW2017_BATINFO_LEN; i++) {
        cw2017_write_u8((uint8_t)(CW2017_REG_BATINFO + i), CW2017_BATINFO[i]);
    }
    cw2017_write_u8(CW2017_REG_CONFIG, 0x80); /* update-enable (bit7) */
    vTaskDelay(pdMS_TO_TICKS(20));
    cw2017_reset();
    for (int i = 0; i < 50; i++) { /* ~1 s cap for SOC to become valid */
        uint8_t soc = 0;
        if (cw2017_read_u8(CW2017_REG_SOC, &soc) == 0 && soc <= 100) break;
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

extern "C" int battery_init_cw2017(void *i2c_master_bus)
{
    if (s_backend != BATT_BACKEND_NONE) return 0;

    if (i2c_master_bus == NULL) {
        ESP_LOGW(TAG, "CW2017 init: NULL I2C bus handle");
        return -1;
    }

    i2c_master_bus_handle_t bus = (i2c_master_bus_handle_t)i2c_master_bus;

    if (i2c_master_probe(bus, CW2017_I2C_ADDR_DEFAULT, 100 /* ms */) != ESP_OK) {
        ESP_LOGW(TAG, "CW2017 not responding at 0x%02X", CW2017_I2C_ADDR_DEFAULT);
        return -1;
    }

    i2c_device_config_t dev_cfg = {};
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address  = CW2017_I2C_ADDR_DEFAULT;
    dev_cfg.scl_speed_hz    = 400000;
    esp_err_t err = i2c_master_bus_add_device(bus, &dev_cfg, &s_cw2017_dev);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "CW2017 bus_add_device failed: %s", esp_err_to_name(err));
        s_cw2017_dev = NULL;
        return -1;
    }

    cw2017_ensure_profile();
    s_backend = BATT_BACKEND_CW2017;

    uint8_t soc = 0;
    cw2017_read_u8(CW2017_REG_SOC, &soc);
    ESP_LOGI(TAG, "CW2017 fuel gauge initialized at 0x%02X (initial %u%%)",
             CW2017_I2C_ADDR_DEFAULT, (unsigned)soc);
    return 0;
}

/* ---- INA226 backend ---- */

extern "C" int battery_init_ina226(void *i2c_master_bus, int i2c_addr,
                                   int cells)
{
    if (s_backend != BATT_BACKEND_NONE) return 0;

    if (i2c_master_bus == NULL) {
        ESP_LOGW(TAG, "INA226 init: NULL I2C bus handle");
        return -1;
    }
    if (i2c_addr < 0 || i2c_addr > 0x7F) {
        ESP_LOGW(TAG, "INA226 init: invalid I2C address 0x%02X", i2c_addr);
        return -1;
    }
    if (cells < 1 || cells > 4) {
        ESP_LOGW(TAG, "INA226 init: implausible cell count %d, "
                 "clamping to 1", cells);
        cells = 1;
    }

    i2c_master_bus_handle_t bus = (i2c_master_bus_handle_t)i2c_master_bus;

    if (i2c_master_probe(bus, (uint8_t)i2c_addr, 100 /* ms */) != ESP_OK) {
        ESP_LOGW(TAG, "INA226 not responding at 0x%02X", i2c_addr);
        return -1;
    }

    i2c_device_config_t dev_cfg = {};
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address  = (uint16_t)i2c_addr;
    dev_cfg.scl_speed_hz    = 400000;
    esp_err_t err = i2c_master_bus_add_device(bus, &dev_cfg, &s_ina226_dev);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "INA226 bus_add_device failed: %s",
                 esp_err_to_name(err));
        s_ina226_dev = NULL;
        return -1;
    }

    /* The INA226 powers up in continuous shunt+bus mode with 1.1 ms
     * conversion times by default, which is fine for a voltage-only
     * read. We deliberately do NOT touch the calibration register
     * (current/power LSBs depend on the shunt resistor wiring, which
     * is board-specific and irrelevant for percentage-from-voltage).
     */

    s_ina226_cells = cells;
    s_backend = BATT_BACKEND_INA226;

    int cell_mv = ina226_read_cell_mv();
    if (cell_mv < 0) {
        ESP_LOGW(TAG, "INA226 initialized at 0x%02X but BUS read failed",
                 i2c_addr);
    } else if (ina226_battery_absent(cell_mv)) {
        ESP_LOGI(TAG, "INA226 power monitor initialized at 0x%02X "
                 "(%dS pack, %d mV/cell -- no battery attached)",
                 i2c_addr, cells, cell_mv);
    } else {
        ESP_LOGI(TAG, "INA226 power monitor initialized at 0x%02X "
                 "(%dS pack, initial %d mV/cell, %d%%)", i2c_addr, cells,
                 cell_mv, mv_to_percent(cell_mv));
    }
    return 0;
}

/* ---- BQ25896 charger ----
 *
 * The BQ25896 is a single-cell I2C-controlled buck charger at
 * 7-bit address 0x6B. We only touch four registers, leaving JEITA
 * thermistor, charge timer, BATFET and OTG configuration at their
 * power-on defaults. See the public header for the rationale.
 *
 * Register map (subset):
 *   REG00 - Input Source Control:
 *     [7]   EN_HIZ          (0 = not in HIZ)
 *     [6]   EN_ILIM         (0 = ignore ILIM pin, use IINLIM register;
 *                            1 = take min(IINLIM, I_LIM_pin))
 *     [5:0] IINLIM          input current limit, 100 mA + N * 50 mA
 *   REG02 - ADC, ICO and D+/D- detection:
 *     [7]   CONV_START      (write 1 to trigger a one-shot ADC; self-
 *                            clears when conversion completes)
 *     [6]   CONV_RATE       (0 = one-shot (POR default), 1 = continuous
 *                            ADC at 1 Hz feeding BATV/SYSV/ICHGR/TSPCT)
 *     [4]   ICO_EN          (0 = disable Input Current Optimizer;
 *                            1 = let the chip dynamically pick
 *                                IDPM_LIM <= IINLIM based on VBUS sag)
 *     [3]   HVDCP_EN        (0 = no HVDCP / QC handshake)
 *     [2]   MAXC_EN         (0 = no MaxCharge handshake)
 *     [1]   FORCE_DPDM      (write 1 to trigger a one-shot D+/D- detect)
 *     [0]   AUTO_DPDM_EN    (0 = do not re-run D+/D- on plug-in)
 *   REG04 - Fast Charge Current Control:
 *     [6:0] ICHG            fast-charge current, N * 64 mA
 *   REG07 - Charge Termination / Timer / Watchdog:
 *     [5:4] WATCHDOG        00 = disabled, 01 = 40 s (POR default)
 *   REG0D - VINDPM Threshold:
 *     [7]   FORCE_VINDPM    0 = relative VINDPM tracks VBUS sample
 *                                 (= VBUS_sampled - VINDPM_OS);
 *                            1 = use the absolute VINDPM bits below
 *     [6:0] VINDPM          absolute threshold, 2600 mV + N * 100 mV
 *                            (range 3.9 V .. 15.3 V)
 */
#define BQ25896_I2C_ADDR        0x6B
#define BQ25896_REG_ISC         0x00
#define BQ25896_REG_ADC         0x02
#define BQ25896_REG_ICHG        0x04
#define BQ25896_REG_TIMER       0x07
#define BQ25896_REG_VBUS_STAT   0x0B   /* REG0B: VBUS_STAT / CHRG_STAT / PG / VSYS */
#define BQ25896_REG_FAULT       0x0C   /* REG0C: WATCHDOG / BOOST / CHRG / BAT / NTC fault */
#define BQ25896_REG_VINDPM      0x0D
#define BQ25896_REG_BATV        0x0E   /* REG0E: THERM_STAT + BATV ADC (2.304 V + N * 20 mV) */
#define BQ25896_REG_SYSV        0x0F   /* REG0F: SYSV ADC (2.304 V + N * 20 mV) */
#define BQ25896_REG_TSPCT       0x10   /* REG10: TS thermistor percentage of REGN */
#define BQ25896_REG_VBUSV       0x11   /* REG11: VBUS_GD + VBUSV ADC (2.6 V + N * 100 mV) */
#define BQ25896_REG_ICHGR       0x12   /* REG12: cell charge current ADC (N * 50 mA) */
#define BQ25896_REG_IDPM_LIM    0x13   /* REG13: VDPM_STAT / IDPM_STAT / IDPM_LIM */

#define BQ25896_IINLIM_BASE_MA  100
#define BQ25896_IINLIM_STEP_MA  50
#define BQ25896_ICHG_STEP_MA    64
#define BQ25896_ICHGR_STEP_MA   50      /* ICHGR ADC step (REG12) */
#define BQ25896_VINDPM_BASE_MV  2600
#define BQ25896_VINDPM_STEP_MV  100
#define BQ25896_BATV_BASE_MV    2304    /* REG0E BATV ADC base */
#define BQ25896_BATV_STEP_MV    20
#define BQ25896_SYSV_BASE_MV    2304    /* REG0F SYSV ADC base */
#define BQ25896_SYSV_STEP_MV    20
#define BQ25896_VBUSV_BASE_MV   2600    /* REG11 VBUSV ADC base */
#define BQ25896_VBUSV_STEP_MV   100

/* Status / fault bit masks. */
#define BQ25896_REG00_EN_HIZ           0x80
#define BQ25896_REG0B_VBUS_STAT_SHIFT  5
#define BQ25896_REG0B_CHRG_STAT_SHIFT  3
#define BQ25896_REG0B_PG_STAT          0x04
#define BQ25896_REG0B_VSYS_STAT        0x01
#define BQ25896_REG0E_THERM_STAT       0x80
#define BQ25896_REG11_VBUS_GD          0x80
#define BQ25896_REG13_VDPM_STAT        0x80
#define BQ25896_REG13_IDPM_STAT        0x40

/* Defaults that comfortably charge a typical 1500-2500 mAh single-cell
 * LiPo. The external ILIM resistor on the board still caps the input
 * current to whatever the hardware was designed for. */
#define BQ25896_DEFAULT_IINLIM_MA   2000
#define BQ25896_DEFAULT_ICHG_MA     1024

/* Absolute VINDPM floor we pin the chip to. The BQ25896's minimum
 * encodable threshold is 3.9 V; that is well below any healthy
 * 5 V USB source even with a noticeable cable drop, so the chip
 * effectively never enters VINDPM regulation on a wall adapter. */
#define BQ25896_DEFAULT_VINDPM_MV   3900

static i2c_master_dev_handle_t   s_bq25896_dev = NULL;

static int bq25896_read_u8(uint8_t reg, uint8_t *out)
{
    if (!s_bq25896_dev) return -1;
    uint8_t wr[1] = { reg };
    if (i2c_master_transmit_receive(s_bq25896_dev, wr, 1, out, 1,
                                    100 /* ms */) != ESP_OK) {
        return -1;
    }
    return 0;
}

static int bq25896_write_u8(uint8_t reg, uint8_t val)
{
    if (!s_bq25896_dev) return -1;
    uint8_t wr[2] = { reg, val };
    if (i2c_master_transmit(s_bq25896_dev, wr, 2, 100 /* ms */) != ESP_OK) {
        return -1;
    }
    return 0;
}

/* Read-modify-write a single register, leaving bits outside `mask`
 * untouched. Returns 0 on success. */
static int bq25896_rmw_u8(uint8_t reg, uint8_t mask, uint8_t val)
{
    uint8_t cur = 0;
    if (bq25896_read_u8(reg, &cur) != 0) return -1;
    uint8_t next = (uint8_t)((cur & ~mask) | (val & mask));
    if (next == cur) return 0;
    return bq25896_write_u8(reg, next);
}

extern "C" int battery_init_bq25896(void *i2c_master_bus)
{
    if (s_bq25896_dev != NULL) return 0;
    if (i2c_master_bus == NULL) {
        ESP_LOGW(TAG, "BQ25896 init: NULL I2C bus handle");
        return -1;
    }

    i2c_master_bus_handle_t bus = (i2c_master_bus_handle_t)i2c_master_bus;

    if (i2c_master_probe(bus, BQ25896_I2C_ADDR, 100 /* ms */) != ESP_OK) {
        ESP_LOGW(TAG, "BQ25896 not responding at 0x%02X", BQ25896_I2C_ADDR);
        return -1;
    }

    i2c_device_config_t dev_cfg = {};
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address  = BQ25896_I2C_ADDR;
    dev_cfg.scl_speed_hz    = 400000;
    esp_err_t err = i2c_master_bus_add_device(bus, &dev_cfg, &s_bq25896_dev);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "BQ25896 bus_add_device failed: %s",
                 esp_err_to_name(err));
        s_bq25896_dev = NULL;
        return -1;
    }

    /* (1) Disable the I2C watchdog FIRST so the subsequent writes
     * cannot be silently reverted to defaults 40 s later. REG07
     * power-on default is 0x9D (EN_TERM=1, WATCHDOG=01 = 40 s,
     * EN_TIMER=1, CHG_TIMER=01 = 12 h, JEITA_ISET=1). Clearing
     * bits 5:4 disables the watchdog while leaving termination,
     * the safety timer and JEITA scaling untouched. */
    if (bq25896_rmw_u8(BQ25896_REG_TIMER, 0x30 /* WATCHDOG */, 0x00) != 0) {
        ESP_LOGW(TAG, "BQ25896: failed to disable I2C watchdog");
    }

    /* (2) Disable the source-detection state machine and enable the
     * continuous ADC. REG02 power-on default is 0x3D (CONV_START=0,
     * CONV_RATE=0, BOOST_FREQ=1, ICO_EN=1, HVDCP_EN=1, MAXC_EN=1,
     * FORCE_DPDM=0, AUTO_DPDM_EN=1). We clear:
     *
     *   - ICO_EN (bit 4): the Input Current Optimizer dynamically
     *     reduces the *actual* input current limit (IDPM_LIM, REG13)
     *     below IINLIM whenever it detects VBUS sag. On a ~500 mA
     *     wall brick + a thin USB-C cable into the LilyGO T5 it
     *     converges to ~150 mA, so charging crawls at 0.11-0.15 A
     *     even though we asked for 2 A. With ICO_EN=0 the IINLIM
     *     register is the only ceiling and the chip stops pulling
     *     itself back.
     *   - HVDCP_EN (bit 3) / MAXC_EN (bit 2): no QC / MaxCharge
     *     handshake; we are a plain USB device. Leaving these on
     *     can cause the chip to renegotiate and drop IINLIM on the
     *     fly.
     *   - AUTO_DPDM_EN (bit 0): do not re-run D+/D- on plug-in,
     *     which would otherwise snap IINLIM back to 500 mA (USB SDP)
     *     whenever the cable is replugged or the host re-enumerates.
     *     (Note: the bit lives at position 0, not 1 -- bit 1 is
     *     FORCE_DPDM, a write-1-to-trigger pulse, default 0.)
     *
     * And we set:
     *
     *   - CONV_RATE (bit 6) = 1: run the BATV / SYSV / ICHGR / TSPCT
     *     ADC continuously at 1 Hz. Without this the chip stays in
     *     one-shot mode and battery_bq25896_dump_status() reads stale
     *     register zeros (BATV=2304 mV / SYSV=2304 mV / ICHGR=0 mA
     *     are the raw-zero ADC base values, not real measurements).
     *
     * Mask 0x5D = CONV_RATE | ICO_EN | HVDCP_EN | MAXC_EN | AUTO_DPDM_EN. */
    if (bq25896_rmw_u8(BQ25896_REG_ADC, 0x5D, 0x40) != 0) {
        ESP_LOGW(TAG, "BQ25896: failed to configure REG02 (ADC / ICO / HVDCP / AUTO_DPDM)");
    }

    /* (3) Raise IINLIM. Clear EN_ILIM (bit 6) so the IINLIM register
     * is the sole input-current ceiling. With EN_ILIM=1 the chip
     * uses min(IINLIM, I_LIM_pin), and on boards such as the LilyGO
     * T5 E-Paper S3 Pro the external ILIM resistor is sized for only
     * ~500 mA, silently clipping the 2 A we ask for here and making
     * charging crawl. EN_HIZ stays 0 (not in HIZ). */
    uint8_t iinlim_n = (uint8_t)((BQ25896_DEFAULT_IINLIM_MA -
                                  BQ25896_IINLIM_BASE_MA) /
                                 BQ25896_IINLIM_STEP_MA);
    if (iinlim_n > 0x3F) iinlim_n = 0x3F;
    uint8_t reg00 = (uint8_t)(iinlim_n & 0x3F); /* EN_HIZ=0, EN_ILIM=0 */
    if (bq25896_write_u8(BQ25896_REG_ISC, reg00) != 0) {
        ESP_LOGW(TAG, "BQ25896: failed to set IINLIM");
    }

    /* (4) Raise fast-charge current. REG04 bit 7 (EN_PUMPX) stays
     * at its default 0. */
    uint8_t ichg_n = (uint8_t)(BQ25896_DEFAULT_ICHG_MA /
                               BQ25896_ICHG_STEP_MA);
    if (ichg_n > 0x7F) ichg_n = 0x7F;
    if (bq25896_write_u8(BQ25896_REG_ICHG, (uint8_t)(ichg_n & 0x7F)) != 0) {
        ESP_LOGW(TAG, "BQ25896: failed to set ICHG");
    }

    /* (5) Pin VINDPM to a low absolute threshold. REG0D power-on
     * default is FORCE_VINDPM=0 (relative mode) with VINDPM_OS
     * (REG01[4:0]) = 600 mV. In relative mode the chip latches
     * VINDPM = (VBUS sampled at plug-in / FORCE_DPDM completion) -
     * VINDPM_OS, and then throttles input current the moment VBUS
     * sags below that latched threshold. On a 5 V wall brick with a
     * thin USB-C cable into the LilyGO T5, sampling at 5.0 V latches
     * VINDPM to ~4.4 V; the instant we try to pull 2 A and VBUS
     * droops to ~4.3 V the chip clamps input current to ~100 mA to
     * hold VBUS up, and the cell crawls in at ~0.11 A even though
     * we cleared ICO, EN_ILIM and AUTO_DPDM. Forcing VINDPM to the
     * chip minimum of 3.9 V means VBUS would have to truly collapse
     * before VINDPM regulation engages, so a normal 5 V supply is
     * never throttled. */
    uint8_t vindpm_n = (uint8_t)((BQ25896_DEFAULT_VINDPM_MV -
                                  BQ25896_VINDPM_BASE_MV) /
                                 BQ25896_VINDPM_STEP_MV);
    if (vindpm_n > 0x7F) vindpm_n = 0x7F;
    uint8_t reg0d = (uint8_t)(0x80 | (vindpm_n & 0x7F)); /* FORCE_VINDPM=1 */
    if (bq25896_write_u8(BQ25896_REG_VINDPM, reg0d) != 0) {
        ESP_LOGW(TAG, "BQ25896: failed to set VINDPM");
    }

    /* Read REG00/REG04 back so the log reflects what the chip actually
     * latched (e.g. if a bus glitch or pending watchdog reset clobbered
     * a write, the numbers below will not match the requested ones).
     * Also surface the post-ICO actual input limit (REG13 IDPM_LIM)
     * and the VBUS / charge status (REG0B) so a "still charging slowly"
     * regression is immediately obvious from the boot log. */
    int iinlim_actual_ma = BQ25896_DEFAULT_IINLIM_MA;
    int ichg_actual_ma   = BQ25896_DEFAULT_ICHG_MA;
    int idpm_lim_ma      = -1;
    int vindpm_actual_mv = BQ25896_DEFAULT_VINDPM_MV;
    bool vindpm_forced   = true;
    uint8_t vbus_stat    = 0xFF;
    uint8_t rb = 0;
    if (bq25896_read_u8(BQ25896_REG_ISC, &rb) == 0) {
        iinlim_actual_ma = BQ25896_IINLIM_BASE_MA +
                           (int)(rb & 0x3F) * BQ25896_IINLIM_STEP_MA;
    }
    if (bq25896_read_u8(BQ25896_REG_ICHG, &rb) == 0) {
        ichg_actual_ma = (int)(rb & 0x7F) * BQ25896_ICHG_STEP_MA;
    }
    if (bq25896_read_u8(BQ25896_REG_IDPM_LIM, &rb) == 0) {
        /* IDPM_LIM uses the same 100 mA + N * 50 mA encoding as IINLIM. */
        idpm_lim_ma = BQ25896_IINLIM_BASE_MA +
                      (int)(rb & 0x3F) * BQ25896_IINLIM_STEP_MA;
    }
    if (bq25896_read_u8(BQ25896_REG_VINDPM, &rb) == 0) {
        vindpm_forced    = (rb & 0x80) != 0;
        vindpm_actual_mv = BQ25896_VINDPM_BASE_MV +
                           (int)(rb & 0x7F) * BQ25896_VINDPM_STEP_MV;
    }
    (void)bq25896_read_u8(BQ25896_REG_VBUS_STAT, &vbus_stat);

    ESP_LOGI(TAG, "BQ25896 charger initialized at 0x%02X "
             "(IINLIM=%d mA, ICHG=%d mA, IDPM_LIM=%d mA, "
             "VINDPM=%d mV %s, REG0B=0x%02X, ICO=off, EN_ILIM=0, "
             "watchdog disabled)",
             BQ25896_I2C_ADDR, iinlim_actual_ma, ichg_actual_ma,
             idpm_lim_ma, vindpm_actual_mv,
             vindpm_forced ? "(forced)" : "(relative)", vbus_stat);
    return 0;
}

extern "C" int battery_read_mv(void)
{
    if (s_backend == BATT_BACKEND_BQ27220) {
        uint16_t mv = 0;
        if (bq27220_read_u16(BQ27220_REG_VOLTAGE, &mv) != 0) return 0;
        /* Smooth with an 8-sample EMA. Without smoothing the displayed
         * percentage tracks raw cell-voltage transients (e.g. unplug
         * relaxation, e-paper refresh bursts) and can drop ~10 % in a
         * few minutes after USB removal even though the pack itself
         * is still near full. The first sample primes the accumulator
         * so we don't return ~mv/8 on cold start. */
        if (!s_bq27220_smooth_primed) {
            s_bq27220_smooth_acc = (uint32_t)mv * SMOOTH_SAMPLES;
            s_bq27220_smooth_primed = true;
        } else {
            s_bq27220_smooth_acc = s_bq27220_smooth_acc
                                 - (s_bq27220_smooth_acc / SMOOTH_SAMPLES)
                                 + (uint32_t)mv;
        }
        return (int)(s_bq27220_smooth_acc / SMOOTH_SAMPLES);
    }
    if (s_backend == BATT_BACKEND_INA226) {
        int cell_mv = ina226_read_cell_mv();
        if (cell_mv < 0) return 0;
        /* Report 0 mV when the pack is missing so callers that only
         * look at voltage cannot infer a bogus "low battery" reading
         * from the floating BUS rail. */
        if (ina226_battery_absent(cell_mv)) return 0;
        return cell_mv;
    }
    if (s_backend == BATT_BACKEND_CW2017) {
        /* 14-bit VCELL, big-endian over two registers. OEM formula:
         * mV = raw14 * 5 / 16, rounded (~0.3125 mV/LSB). */
        uint8_t hi = 0, lo = 0;
        if (cw2017_read_u8(CW2017_REG_VCELL_H, &hi) != 0) return 0;
        if (cw2017_read_u8((uint8_t)(CW2017_REG_VCELL_H + 1), &lo) != 0) return 0;
        uint16_t raw14 = (uint16_t)((hi & 0x3F) << 8) | lo;
        return (int)((raw14 * 5 + 8) >> 4);
    }
    if (s_backend != BATT_BACKEND_ADC) return 0;

    bool en = enable_divider();
    int raw = 0;
    esp_err_t err = adc_oneshot_read(s_adc_handle, s_channel, &raw);
    disable_divider(en);
    if (err != ESP_OK) return 0;

    int pin_mv = 0;
    if (s_cali_handle) {
        adc_cali_raw_to_voltage(s_cali_handle, raw, &pin_mv);
    } else {
        pin_mv = (raw * ADC_VREF_MV) / 4095;
    }

    int bat_mv = pin_mv * s_divider;

    /* Update exponential moving average */
    s_smooth_acc = s_smooth_acc - (s_smooth_acc / SMOOTH_SAMPLES) + (uint32_t)bat_mv;
    return (int)(s_smooth_acc / SMOOTH_SAMPLES);
}

/* Forward decl: battery_read_current_ma() is defined further down. */
extern "C" int battery_read_current_ma(void);

/* Return the BQ25896 CHRG_STAT field (0..3) or -1 if the charger is
 * absent / unreadable. 0=Not Charging, 1=Pre-charge, 2=Fast Charge,
 * 3=Charge Termination Done. */
static int bq25896_chrg_stat(void)
{
    if (!s_bq25896_dev) return -1;
    uint8_t reg0b = 0;
    if (bq25896_read_u8(BQ25896_REG_VBUS_STAT, &reg0b) != 0) return -1;
    return (int)((reg0b >> BQ25896_REG0B_CHRG_STAT_SHIFT) & 0x03);
}

/* Translate a BQ27220 terminal-voltage reading to a displayed
 * percentage, applying a charge-state-aware correction when the
 * BQ25896 reports the cell is actively being charged. */
static int bq27220_mv_to_displayed_percent(int mv)
{
    if (mv <= 0) return -1;

    int chrg = bq25896_chrg_stat();

    /* Termination Done -> cell is at Vreg with current tapered off;
     * display 100 % regardless of the LUT's nearest entry. */
    if (chrg == 3) return 100;

    /* Pre-charge: cell is so deeply discharged the charger is using
     * the low-current pre-charge phase. Whatever the LUT says about
     * 3.0-3.2 V is fine, but cap displayed % so we never show e.g.
     * 30 % while still in pre-charge. */
    if (chrg == 1) {
        int p = mv_to_percent(mv);
        return (p > 10) ? 10 : p;
    }

    /* Fast Charge: subtract I*R_int from the measured terminal
     * voltage to estimate OCV before the LUT. battery_read_current_ma()
     * returns + when current flows INTO the cell. */
    if (chrg == 2) {
        int corrected = mv;
        int ima = battery_read_current_ma();
        if (ima != INT32_MIN && ima > 0) {
            int drop = (ima * BATT_INTERNAL_R_MOHM) / 1000;
            if (drop > BATT_IR_CORR_MAX_MV) drop = BATT_IR_CORR_MAX_MV;
            corrected = mv - drop;
        }
        return mv_to_percent(corrected);
    }

    /* Not Charging (or charger absent / I2C failure): treat as OCV. */
    return mv_to_percent(mv);
}

extern "C" int battery_read_percent(void)
{
    if (s_backend == BATT_BACKEND_BQ27220) {
        /* The BQ27220's StateOfCharge register is only meaningful after
         * its Data Memory (Design Capacity, chemistry ID, terminate
         * voltage, taper current, ...) has been programmed for the
         * pack and the Impedance-Track algorithm has completed a full
         * learning cycle. On the LilyGO T5 E-Paper S3 Pro the factory
         * leaves the gauge with default DM values, and a few full
         * discharge/charge cycles do NOT make it learn -- SoC stays
         * pinned around 50 % regardless of actual cell voltage (e.g.
         * 53 % reported at 4124 mV BATV / CHRG_STAT=Termination Done).
         * Trust the gauge only as a voltmeter and derive percentage
         * from the same LiPo discharge LUT used by the ADC backend.
         * battery_read_mv() applies an EMA over recent samples so the
         * displayed percentage does not lurch on each USB unplug.
         *
         * While the BQ25896 is actively charging the cell, terminal
         * voltage runs ~150-250 mV above OCV (and is pinned at Vreg
         * during CV taper), so feeding it straight into the OCV LUT
         * makes a ~89 % cell display 100 % the instant USB is plugged
         * in. bq27220_mv_to_displayed_percent() consults the charger's
         * CHRG_STAT and applies an I*R_int correction during Fast
         * Charge to keep the percentage close to true SoC. */
        int mv = battery_read_mv();
        return bq27220_mv_to_displayed_percent(mv);
    }
    if (s_backend == BATT_BACKEND_INA226) {
        int cell_mv = ina226_read_cell_mv();
        if (cell_mv < 0) return -1;
        /* No battery attached -> let the status bar hide the value
         * instead of rendering ~14-18 % from the floating rail. */
        if (ina226_battery_absent(cell_mv)) return -1;
        return mv_to_percent(cell_mv);
    }
    if (s_backend == BATT_BACKEND_CW2017) {
        /* The CW2017 reports SOC as a direct integer percentage once
         * its BATINFO profile is resident -- no discharge LUT needed,
         * unlike BQ27220. */
        uint8_t soc = 0;
        if (cw2017_read_u8(CW2017_REG_SOC, &soc) != 0) return -1;
        return (soc > 100) ? 100 : (int)soc;
    }
    if (s_backend != BATT_BACKEND_ADC) return -1;

    int mv = battery_read_mv();
    return mv_to_percent(mv);
}

extern "C" int battery_read_charging(void)
{
    /* When the BQ25896 charger is initialized it is the authoritative
     * source for charge state: it reads VBUS power-good and the
     * CHRG_STAT field directly from the chip that controls the FET.
     * The BQ27220 Flags DSG bit is derived from current threshold
     * comparisons in the gauge's algorithm and -- like its SoC
     * register -- is unreliable when the Data Memory is left at
     * factory defaults (the gauge can report DSG=0 while running on
     * battery), which produced a sticky "+" charging glyph on the
     * LilyGO T5 E-Paper S3 Pro after USB removal. */
    if (s_bq25896_dev) {
        uint8_t reg0b = 0;
        if (bq25896_read_u8(BQ25896_REG_VBUS_STAT, &reg0b) == 0) {
            /* CHRG_STAT: 0=Not Charging, 1=Pre-charge, 2=Fast Charge,
             * 3=Charge Termination Done. Only 1 and 2 mean current is
             * actually flowing into the cell. */
            unsigned chrg = (reg0b >> BQ25896_REG0B_CHRG_STAT_SHIFT) & 0x03;
            return (chrg == 1 || chrg == 2) ? 1 : 0;
        }
        /* I2C read failed -- fall through to backend-specific paths
         * so we don't lose all charge-state visibility. */
    }
    if (s_backend == BATT_BACKEND_BQ27220) {
        /* Flags register, bit 0 (DSG): 0 -> charging or full, 1 -> discharging.
         * BQ27220 datasheet, table 12-7. */
        uint16_t flags = 0;
        if (bq27220_read_u16(BQ27220_REG_FLAGS, &flags) != 0) return -1;
        return (flags & BQ27220_FLAGS_DSG) ? 0 : 1;
    }
    if (s_backend == BATT_BACKEND_INA226) {
        if (!s_ina226_dev) return -1;
        /* If no battery is attached, charge state is meaningless. */
        int cell_mv = ina226_read_cell_mv();
        if (cell_mv < 0 || ina226_battery_absent(cell_mv)) return -1;
        uint8_t wr[1] = { INA226_REG_SHUNTVOLTAGE };
        uint8_t rd[2] = { 0, 0 };
        if (i2c_master_transmit_receive(s_ina226_dev, wr, 1, rd, 2,
                                        100 /* ms */) != ESP_OK) {
            return -1;
        }
        /* Big-endian on the wire; the register is signed two's complement. */
        int16_t raw = (int16_t)(((uint16_t)rd[0] << 8) | (uint16_t)rd[1]);
        if (raw <= -INA226_SHUNT_CHARGE_THRESHOLD) return 1;
        return 0;
    }
    /* ADC backend, CW2017 (no charger IC on its bus, no current
     * register) and "no backend" cannot tell. */
    return -1;
}

/* ---- BQ27220 signed current readout ----
 *
 * BQ27220 register 0x0C (Current) is a 16-bit signed two's complement
 * value in mA, LSB = 1 mA. Sign convention per the TRM (section 4.1):
 * positive = current flowing INTO the cell (charging), negative =
 * current flowing OUT of the cell (system load). */
extern "C" int battery_read_current_ma(void)
{
    if (s_backend != BATT_BACKEND_BQ27220) return INT32_MIN;
    uint16_t raw = 0;
    if (bq27220_read_u16(BQ27220_REG_CURRENT, &raw) != 0) return INT32_MIN;
    return (int)(int16_t)raw;
}

/* ---- BQ25896 helpers exposed for instrumentation / sleep prep ---- */

extern "C" int battery_bq25896_vbus_present(void)
{
    if (!s_bq25896_dev) return -1;
    uint8_t v = 0;
    if (bq25896_read_u8(BQ25896_REG_VBUSV, &v) != 0) return -1;
    return (v & BQ25896_REG11_VBUS_GD) ? 1 : 0;
}

extern "C" void battery_bq25896_prepare_sleep(void)
{
    /* Only HIZ the charger when we are running on battery. With
     * VBUS present the rail is what we want the chip to be regulating
     * (e.g. user left the device plugged in while it sleeps to charge
     * overnight) and HIZ would disconnect it from VBUS. The cold-boot
     * re-init in battery_init_bq25896() always clears EN_HIZ back
     * to 0, so a stale HIZ from a previous sleep cycle cannot
     * persist across a wake.
     *
     * Returning early on "VBUS present" or "read failed" is the
     * conservative choice: a missed I2C transaction leaves the chip
     * in its current state (already configured at boot to draw
     * ~50 uA when VBUS is genuinely absent) instead of forcing a
     * change we cannot verify. */
    int present = battery_bq25896_vbus_present();
    if (present != 0) return;  /* present or unknown -> leave alone */

    /* Set EN_HIZ (bit 7) in REG00 while preserving the IINLIM bits
     * we wrote at init. EN_ILIM stays 0. */
    uint8_t reg00 = 0;
    if (bq25896_read_u8(BQ25896_REG_ISC, &reg00) != 0) return;
    uint8_t target = reg00 | BQ25896_REG00_EN_HIZ;
    if (target == reg00) return;  /* already HIZ */
    if (bq25896_write_u8(BQ25896_REG_ISC, target) != 0) {
        ESP_LOGW(TAG, "BQ25896: pre-sleep EN_HIZ write failed");
        return;
    }
    ESP_LOGI(TAG, "BQ25896: VBUS absent -> EN_HIZ set for battery-only sleep");
}

extern "C" void battery_bq25896_reassert_config(void)
{
    /* Disable the I2C watchdog (bits 5:4 = 00 in REG07). Re-issued
     * periodically because a brown-out / glitch reset on the chip
     * power rail would otherwise revert REG07 to its POR 0x9D
     * (watchdog = 40 s) and silently undo our writes 40 s later. */
    (void)bq25896_rmw_u8(BQ25896_REG_TIMER, 0x30, 0x00);

    /* Force ICO_EN / HVDCP_EN / MAXC_EN / AUTO_DPDM_EN all = 0 and
     * re-assert CONV_RATE = 1 (REG02 mask 0x5D, value 0x40). Same
     * rationale as init: the Input Current Optimizer can otherwise
     * drag IDPM_LIM down to ~150 mA over time on a slightly sagging
     * supply, AUTO_DPDM would snap IINLIM back to USB-SDP 500 mA on
     * every cable reseat, and a chip POR would revert CONV_RATE to 0
     * and leave the ADC stuck in one-shot mode (so the next status
     * dump would log raw-zero BATV/SYSV/ICHGR readings). */
    (void)bq25896_rmw_u8(BQ25896_REG_ADC, 0x5D, 0x40);
}

extern "C" void battery_bq25896_dump_status(void)
{
    if (!s_bq25896_dev) return;

    uint8_t reg00 = 0, reg0b = 0, reg0c = 0, reg0e = 0, reg0f = 0;
    uint8_t reg10 = 0, reg11 = 0, reg12 = 0, reg13 = 0;
    (void)bq25896_read_u8(BQ25896_REG_ISC,      &reg00);
    (void)bq25896_read_u8(BQ25896_REG_VBUS_STAT,&reg0b);
    (void)bq25896_read_u8(BQ25896_REG_FAULT,    &reg0c);
    (void)bq25896_read_u8(BQ25896_REG_BATV,     &reg0e);
    (void)bq25896_read_u8(BQ25896_REG_SYSV,     &reg0f);
    (void)bq25896_read_u8(BQ25896_REG_TSPCT,    &reg10);
    (void)bq25896_read_u8(BQ25896_REG_VBUSV,    &reg11);
    (void)bq25896_read_u8(BQ25896_REG_ICHGR,    &reg12);
    (void)bq25896_read_u8(BQ25896_REG_IDPM_LIM, &reg13);

    int iinlim_ma  = BQ25896_IINLIM_BASE_MA +
                     (int)(reg00 & 0x3F) * BQ25896_IINLIM_STEP_MA;
    int batv_mv    = BQ25896_BATV_BASE_MV +
                     (int)(reg0e & 0x7F) * BQ25896_BATV_STEP_MV;
    int sysv_mv    = BQ25896_SYSV_BASE_MV +
                     (int)(reg0f & 0x7F) * BQ25896_SYSV_STEP_MV;
    int vbusv_mv   = BQ25896_VBUSV_BASE_MV +
                     (int)(reg11 & 0x7F) * BQ25896_VBUSV_STEP_MV;
    int ichgr_ma   = (int)(reg12 & 0x7F) * BQ25896_ICHGR_STEP_MA;
    int idpm_lim_ma = BQ25896_IINLIM_BASE_MA +
                      (int)(reg13 & 0x3F) * BQ25896_IINLIM_STEP_MA;

    unsigned vbus_stat = (reg0b >> BQ25896_REG0B_VBUS_STAT_SHIFT) & 0x07;
    unsigned chrg_stat = (reg0b >> BQ25896_REG0B_CHRG_STAT_SHIFT) & 0x03;

    ESP_LOGI(TAG,
        "BQ25896 status: EN_HIZ=%d IINLIM=%dmA VBUSV=%dmV (GD=%d) "
        "BATV=%dmV SYSV=%dmV ICHGR=%dmA IDPM_LIM=%dmA "
        "VBUS_STAT=%u CHRG_STAT=%u PG=%d VSYS=%d "
        "THERM=%d VDPM=%d IDPM=%d TSPCT=%u%% "
        "FAULT[wd=%d boost=%d chrg=%u bat=%d ntc=%u]",
        (reg00 & BQ25896_REG00_EN_HIZ) ? 1 : 0, iinlim_ma, vbusv_mv,
        (reg11 & BQ25896_REG11_VBUS_GD) ? 1 : 0,
        batv_mv, sysv_mv, ichgr_ma, idpm_lim_ma,
        vbus_stat, chrg_stat,
        (reg0b & BQ25896_REG0B_PG_STAT) ? 1 : 0,
        (reg0b & BQ25896_REG0B_VSYS_STAT) ? 1 : 0,
        (reg0e & BQ25896_REG0E_THERM_STAT) ? 1 : 0,
        (reg13 & BQ25896_REG13_VDPM_STAT) ? 1 : 0,
        (reg13 & BQ25896_REG13_IDPM_STAT) ? 1 : 0,
        (unsigned)(reg10 & 0x7F),
        (reg0c >> 7) & 1,
        (reg0c >> 6) & 1,
        (unsigned)((reg0c >> 4) & 0x03),
        (reg0c >> 3) & 1,
        (unsigned)(reg0c & 0x07));
}
