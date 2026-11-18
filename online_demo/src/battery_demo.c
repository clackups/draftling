/*
 * Online demo: the FNK0104S reads its cell through an ADC divider,
 * which gives a voltage but no charge state. The demo cell sits at a
 * fixed, healthy level.
 */

#include "battery.h"

int battery_init(int gpio_num, int enable_gpio, int divider)
{
    (void)gpio_num; (void)enable_gpio; (void)divider;
    return 0;
}
int battery_read_mv(void) { return 3990; }
int battery_read_percent(void) { return 75; }
int battery_read_charging(void) { return -1; }
