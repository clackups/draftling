/*
 * Online demo: entry point. Mirrors app_main() in firmware/main/main.cpp
 * for the simulated board (the one app_config.h selects from the
 * variant's generated sdkconfig.h), minus the hardware bring-up, then runs the
 * LVGL loop from the browser's animation frames.
 *
 * On the first visit the sample documents embedded at /seed are
 * copied onto the simulated SD card.
 */

#include <cstdio>
#include <cstring>

#include <dirent.h>
#include <sys/stat.h>

#include <emscripten.h>

#include "app_config.h"
#include "battery.h"
#include "driver/spi_common.h"  /* SPI2_HOST for boards with the SD card on SPI */
#include "ble_keyboard.h"
#include "demo_host.h"
#include "display.h"
#include "editor.h"
#include "editor_ui.h"
#include "esp_log.h"
#include "git_sync.h"
#include "lvgl.h"
#include "lvgl_port.h"
#include "nvs_flash.h"
#include "sd_card.h"
#include "standby.h"
#include "usb_msc.h"

static const char *TAG = "Draftling";

#define SEED_DIR    "/seed"
#define SEED_MARKER "/persist/seeded"

extern "C" void demo_lvgl_port_present(void);

static void copy_tree(const char *from, const char *to)
{
    mkdir(to, 0755);
    DIR *d = opendir(from);
    if (!d) return;
    struct dirent *e;
    char src[512], dst[512];
    while ((e = readdir(d)) != nullptr) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        snprintf(src, sizeof(src), "%s/%s", from, e->d_name);
        snprintf(dst, sizeof(dst), "%s/%s", to, e->d_name);
        struct stat st;
        if (stat(src, &st) != 0) continue;
        if (S_ISDIR(st.st_mode)) {
            copy_tree(src, dst);
        } else {
            FILE *in = fopen(src, "rb");
            FILE *out = fopen(dst, "wb");
            if (in && out) {
                char buf[4096];
                size_t n;
                while ((n = fread(buf, 1, sizeof(buf), in)) > 0) fwrite(buf, 1, n, out);
            }
            if (in) fclose(in);
            if (out) fclose(out);
        }
    }
    closedir(d);
}

/* First visit (or after "Reset demo"): put the sample project on the card. */
static void seed_sd_card(void)
{
    struct stat st;
    if (stat(SEED_MARKER, &st) == 0) return;
    ESP_LOGI(TAG, "First start: copying the sample documents to the SD card");
    copy_tree(SEED_DIR, SD_MOUNT_POINT);
    FILE *f = fopen(SEED_MARKER, "w");
    if (f) fclose(f);
    demo_host_storage_dirty();
}

/* Same as pre_sleep_autosave() / pre_sleep_freenove_deinit() in main.cpp. */
static void pre_sleep_save_doc_cb(editor_doc_t *doc, void *ctx)
{
    (void)ctx;
    if (editor_doc_is_modified(doc) && editor_doc_get_path(doc)) {
        esp_err_t err = editor_doc_save(doc);
        if (err != ESP_OK) ESP_LOGE(TAG, "Auto-save failed: %s", esp_err_to_name(err));
    }
    editor_doc_save_meta(doc);
}

static void pre_sleep_demo(void)
{
    usb_msc_set_mode(USB_MSC_MODE_OFF);
    editor_doc_foreach(pre_sleep_save_doc_cb, nullptr);
    display_deep_sleep_prepare();
}

static void main_loop(void)
{
    lv_timer_handler();
    demo_lvgl_port_present();
    demo_display_poll();   /* e-paper: a refresh owed while it was busy */
}

int main(void)
{
    nvs_flash_init();
    display_margins_init();
    display_orientation_init();
    display_flip_init();
    display_scale_init();

    seed_sd_card();

    draftling_lvgl_port_init(DISPLAY_LOGICAL_WIDTH, DISPLAY_LOGICAL_HEIGHT,
                             DISPLAY_ROTATE_EFFECTIVE, DISPLAY_SCALE);
    display_set_backlight(50);   /* editor_ui_init() applies the saved level */

#if defined(CONFIG_DRAFTLING_BATTERY_BQ27220)
    battery_init_bq27220(nullptr);
#else
    battery_init(BATT_ADC_PIN, BATT_EN_PIN, BATT_DIVIDER);
#endif

    editor_ui_set_board_name(BOARD_NAME " (online demo)");
    editor_ui_init();

#if defined(CONFIG_DRAFTLING_SD_SDMMC)
    esp_err_t sd_ret = sd_card_init(SD_CLK_PIN, SD_CMD_PIN, SD_D0_PIN, SD_MOUNT_POINT);
#else
    esp_err_t sd_ret = sd_card_init_spi(SD_SPI_HOST_NUM, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN,
                                        SD_SPI_SCK_PIN, SD_SPI_CS_PIN, -1, SD_MOUNT_POINT);
#endif
    if (sd_ret != ESP_OK) {
        editor_ui_set_status("ERROR: SD card not ready");
    }

    editor_ui_set_ble_prompt_text(
        "Searching for BLE keyboard...\nPlease turn on your keyboard");
    ble_keyboard_init();

    git_sync_init();
    standby_init();
    standby_set_pre_sleep_cb(pre_sleep_demo);

    ESP_LOGI(TAG, "Draftling online demo ready");
    emscripten_set_main_loop(main_loop, 0, false);
    return 0;
}
