/* Online demo: bridge to the web page (see demo_host.h). */

#include "demo_host.h"

#include <emscripten.h>
#include <emscripten/eventloop.h>

EM_JS(void, demo_host_set_screen, (int w, int h, int scale, bool upside_down, bool epd), {
    Module.demoSetScreen(w, h, scale, upside_down, epd);
});

EM_JS(void, demo_host_present, (const uint16_t *fb, int fb_w, int x, int y, int w, int h), {
    Module.demoPresent(fb, fb_w, x, y, w, h);
});

EM_JS(void, demo_host_epd_refresh, (const uint8_t *px, int w, int h, bool full,
                                    int delay_ms, int dur_ms), {
    Module.demoEpdRefresh(px, w, h, full, delay_ms, dur_ms);
});

EM_JS(void, demo_host_set_backlight, (int percent), {
    Module.demoSetBacklight(percent);
});

EM_JS(void, demo_host_restart, (void), {
    Module.demoRestart();
});

EM_JS(void, demo_host_sleep, (void), {
    Module.demoSleep();
});

EM_JS(void, demo_host_storage_dirty, (void), {
    Module.demoStorageDirty();
});

void demo_halt(void)
{
    emscripten_cancel_main_loop();
    /* Leave the current call stack (an LVGL timer or a key handler)
     * for good, like a reset does on the device. */
    emscripten_unwind_to_js_event_loop();
}
