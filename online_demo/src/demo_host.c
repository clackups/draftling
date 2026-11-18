/* Online demo: bridge to the web page (see demo_host.h). */

#include "demo_host.h"

#include <emscripten.h>
#include <emscripten/eventloop.h>

EM_JS(void, demo_host_set_screen, (int w, int h, int scale, bool upside_down), {
    Module.demoSetScreen(w, h, scale, upside_down);
});

EM_JS(void, demo_host_present, (const uint16_t *fb, int fb_w, int x, int y, int w, int h), {
    Module.demoPresent(fb, fb_w, x, y, w, h);
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
