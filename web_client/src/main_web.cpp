/*
 * Web client: entry point. Runs on a Web Worker (Emscripten pthread,
 * PROXY_TO_PTHREAD) once the page (web/client.js) has made sure of
 * persistent storage and a single running copy. Mounts the browser's
 * origin-private file system, then starts the subsystems in the order
 * of linux/src/main_linux.cpp (itself following app_main() in
 * firmware/main/main.cpp) and runs the Linux program's LVGL loop.
 */

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sys/stat.h>
#include <unistd.h>

#include <emscripten.h>
#include <emscripten/wasmfs.h>

#include "app_config.h"
#include "ble_keyboard.h"
#include "display.h"
#include "editor.h"
#include "editor_ui.h"
#include "esp_log.h"
#include "git_sync.h"
#include "linux_host.h"
#include "lvgl.h"
#include "lvgl_port.h"
#include "nvs_flash.h"
#include "sd_card.h"
#include "standby.h"
#include "web_host.h"
#include "wifi_manager.h"

static const char *TAG = "Draftling";

#ifndef DRAFTLING_VERSION
#define DRAFTLING_VERSION "dev"
#endif

/* The browser's origin-private file system (OPFS), mounted here. */
#define OPFS_ROOT "/opfs"

/* The SD card layer's mount point: the documents folder is the current
 * directory (see linux_host.h). */
#define MOUNT_POINT "."

char linux_data_dir[4096] = OPFS_ROOT "/documents";
char linux_config_dir[4096] = OPFS_ROOT "/settings";
/* The folder is the site's own: "Format SD card" may empty it. */
bool linux_data_dir_owned = true;
int linux_window_w, linux_window_h;

/* Tell the page something went wrong for good (shown over the canvas). */
static void fatal(const char *msg)
{
    ESP_LOGE(TAG, "%s", msg);
    MAIN_THREAD_EM_ASM({
        if (Module.webFatal) Module.webFatal(UTF8ToString($0));
    }, msg);
    for (;;) sleep(3600);
}

/* Never ends: the page goes on showing the last picture until it is
 * reloaded or closed. */
static void park(void)
{
    for (;;) sleep(3600);
}

/* Restart (esp_restart(): a setting that applies after a restart,
 * Factory reset, Format SD card): reload the page. */
extern "C" void linux_restart(void)
{
    ESP_LOGI(TAG, "Restarting");
    draftling_lvgl_port_lock(-1);
    MAIN_THREAD_ASYNC_EM_ASM({
        if (Module.webRestart) Module.webRestart();
        else location.reload();
    });
    park();
    __builtin_unreachable();
}

/* F1 -> "Quit Draftling" (standby_quit()) or a quit request of the
 * LVGL port: save, then the page closes the app (Module.webQuit). */
extern "C" void linux_quit(int status)
{
    (void)status;
    draftling_lvgl_port_lock(-1);
    static bool quitting;
    if (!quitting) {
        quitting = true;
        if (linux_pre_sleep_cb) linux_pre_sleep_cb();
        MAIN_THREAD_ASYNC_EM_ASM({
            if (Module.webQuit) Module.webQuit();
        });
    }
    park();
    __builtin_unreachable();
}

/* Same as pre_sleep_autosave() in main.cpp. */
static void pre_sleep_save_doc_cb(editor_doc_t *doc, void *ctx)
{
    (void)ctx;
    if (editor_doc_is_modified(doc) && editor_doc_get_path(doc)) {
        esp_err_t err = editor_doc_save(doc);
        if (err != ESP_OK) ESP_LOGE(TAG, "Auto-save failed: %s", esp_err_to_name(err));
    }
    editor_doc_save_meta(doc);
}

static void pre_sleep_web(void)
{
    if (draftling_lvgl_port_lock(2000)) {
        editor_doc_foreach(pre_sleep_save_doc_cb, nullptr);
        draftling_lvgl_port_unlock();
    }
}

static bool mkdir_ok(const char *path)
{
    return mkdir(path, 0700) == 0 || errno == EEXIST;
}

int main(void)
{
    int lvl = MAIN_THREAD_EM_ASM_INT({ return Module.webLogLevel || 0; });
    if (lvl >= LINUX_LOG_ERROR && lvl <= LINUX_LOG_DEBUG) linux_log_level = lvl;

    backend_t opfs = wasmfs_create_opfs_backend();
    if (!opfs || wasmfs_create_directory(OPFS_ROOT, 0777, opfs) != 0)
        fatal("The browser's private file system (OPFS) is not available.");
    if (!mkdir_ok(linux_data_dir) || !mkdir_ok(linux_config_dir))
        fatal("Cannot create Draftling's folders in the browser's private file system.");
    if (chdir(linux_data_dir) != 0)
        fatal("Cannot open Draftling's documents folder.");

    if (!linux_port_open_io("web")) fatal("No display.");

    nvs_flash_init();
    display_margins_init();
    display_orientation_init();
    display_flip_init();
    display_scale_init();

    int w, h;
    linux_port_initial_size(&w, &h);
    draftling_lvgl_port_init(w, h, 0, DISPLAY_SCALE);

    wifi_manager_init();

    editor_ui_set_board_name(BOARD_NAME);
    editor_ui_init();

    if (sd_card_init(-1, -1, -1, MOUNT_POINT) != ESP_OK) {
        editor_ui_set_status("ERROR: browser storage not usable");
    }

    ble_keyboard_init();
    git_sync_init();
    standby_init();
    standby_set_pre_sleep_cb(pre_sleep_web);

    MAIN_THREAD_ASYNC_EM_ASM({
        if (Module.webReady) Module.webReady();
    });
    ESP_LOGI(TAG, "Draftling %s ready", DRAFTLING_VERSION);
    linux_port_run();
}
