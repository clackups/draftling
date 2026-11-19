/*
 * Online demo: the battery. The FNK0104S reads its cell through an ADC
 * divider, which gives a voltage but no charge state; the reTerminal
 * Sticky has a BQ27220 fuel gauge, which also tells charging from
 * discharging. The demo cell sits at a fixed, healthy level and is
 * not being charged.
 */

#include "battery.h"
#include "sdkconfig.h"

static int s_charging = -1;

int battery_init(int gpio_num, int enable_gpio, int divider)
{
    (void)gpio_num; (void)enable_gpio; (void)divider;
    s_charging = -1;
    return 0;
}

int battery_init_bq27220(void *i2c_master_bus)
{
    (void)i2c_master_bus;
    s_charging = 0;
    return 0;
}

int battery_read_mv(void) { return 3990; }
int battery_read_percent(void) { return 75; }
int battery_read_charging(void) { return s_charging; }
