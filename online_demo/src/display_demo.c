/*
 * Online demo: the display backend API. The simulated panel is the
 * Freenove FNK0104S's ST7796 480x320 color LCD; LVGL's frames reach
 * the page through lvgl_port_demo.c, so these calls only carry the
 * backlight level to the page.
 */

#include "demo_host.h"
#include "display.h"

void display_init(int a, int b, int c, int d, int e, int f, int w, int h)
{
    (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; (void)w; (void)h;
}
void display_spi_tft_config(const display_spi_tft_config_t *cfg) { (void)cfg; }
void display_nv3041a_config(const display_nv3041a_config_t *cfg) { (void)cfg; }
void display_ssd1677_spi_config(const display_ssd1677_spi_config_t *cfg) { (void)cfg; }
void display_axs15231b_init(const display_axs15231b_config_t *cfg) { (void)cfg; }
void display_clear(uint8_t color) { (void)color; }
void display_set_pixel(uint16_t x, uint16_t y, uint8_t color) { (void)x; (void)y; (void)color; }
void display_flush(void) { }
void display_full_refresh(void) { }
void display_request_full_refresh(void) { }
bool display_push_rgb565(int x, int y, int w, int h, const void *map)
{
    (void)x; (void)y; (void)w; (void)h; (void)map;
    return false;
}
void display_set_partial_clip(int x, int y, int w, int h) { (void)x; (void)y; (void)w; (void)h; }
void display_sleep(void) { }
void display_wake(void) { }
void display_deep_sleep_prepare(void) { demo_host_set_backlight(0); }
void display_set_backlight(int percent) { demo_host_set_backlight(percent); }
void display_set_invert(bool on) { (void)on; }
void display_set_shared_i2c_bus(void *bus) { (void)bus; }
const char *display_controller_name(void) { return "ST7796 (simulated)"; }
