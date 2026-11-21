/*
 * Linux: entry point. Parses the command line, sets up the data and
 * settings folders, opens an I/O backend (Wayland, X11 or the
 * framebuffer), then starts the subsystems in the order of app_main()
 * in firmware/main/main.cpp and runs the LVGL loop.
 */

#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <dirent.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#include "app_config.h"
#include "battery.h"
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
#include "wifi_manager.h"

static const char *TAG = "Draftling";

#ifndef DRAFTLING_VERSION
#define DRAFTLING_VERSION "dev"
#endif
#ifndef DRAFTLING_LANG_PACK
#define DRAFTLING_LANG_PACK "default"
#endif

/* The SD card layer's mount point: the data folder is the current
 * directory (see linux_host.h). */
#define MOUNT_POINT "."

char linux_data_dir[4096];
char linux_config_dir[4096];
bool linux_data_dir_owned;
int linux_window_w, linux_window_h;
#if defined(LINUX_IO_FBDEV)
const char *linux_fbdev_path = "/dev/fb0";
#endif

extern "C" standby_pre_sleep_cb_t linux_pre_sleep_cb;

static char **s_argv;
static char s_exe[PATH_MAX];

/* ---- folders ------------------------------------------------------------- */

static bool mkdir_p(const char *path)
{
    char tmp[sizeof(linux_data_dir)];
    snprintf(tmp, sizeof(tmp), "%s", path);
    for (char *p = tmp + 1; *p; p++) {
        if (*p != '/') continue;
        *p = '\0';
        if (mkdir(tmp, 0700) != 0 && errno != EEXIST) return false;
        *p = '/';
    }
    return mkdir(tmp, 0700) == 0 || errno == EEXIST;
}

static bool dir_is_empty(const char *path)
{
    DIR *d = opendir(path);
    if (!d) return false;
    struct dirent *e;
    bool empty = true;
    while ((e = readdir(d)) != nullptr) {
        if (strcmp(e->d_name, ".") && strcmp(e->d_name, "..")) {
            empty = false;
            break;
        }
    }
    closedir(d);
    return empty;
}

/* $XDG_<var> or ~/<fallback>, plus "/draftling". */
static void xdg_dir(char *out, size_t n, const char *var, const char *fallback)
{
    const char *x = getenv(var);
    if (x && x[0] == '/') {
        snprintf(out, n, "%s/draftling", x);
        return;
    }
    const char *home = getenv("HOME");
    snprintf(out, n, "%s/%s/draftling", home && home[0] ? home : ".", fallback);
}

static bool absolute(const char *in, char *out, size_t n)
{
    if (in[0] == '/') {
        snprintf(out, n, "%s", in);
        return true;
    }
    char cwd[PATH_MAX];
    if (!getcwd(cwd, sizeof(cwd))) return false;
    snprintf(out, n, "%s/%s", cwd, in);
    return true;
}

static void setup_data_dir(void)
{
    bool existed = access(linux_data_dir, F_OK) == 0;
    if (!mkdir_p(linux_data_dir)) {
        fprintf(stderr, "draftling: cannot create %s: %s\n", linux_data_dir, strerror(errno));
        exit(1);
    }
    char marker[sizeof(linux_data_dir) + 32];
    snprintf(marker, sizeof(marker), "%s/" LINUX_DATA_MARKER, linux_data_dir);
    if (!existed || dir_is_empty(linux_data_dir)) {
        FILE *f = fopen(marker, "w");
        if (f) {
            fputs("This folder holds Draftling's documents. Draftling's\n"
                  "\"Format SD card\" command may empty it.\n", f);
            fclose(f);
        }
    }
    linux_data_dir_owned = access(marker, F_OK) == 0;
    if (chdir(linux_data_dir) != 0) {
        fprintf(stderr, "draftling: cannot enter %s: %s\n", linux_data_dir, strerror(errno));
        exit(1);
    }
}

/* One Draftling per data folder: two would overwrite each other's
 * documents and Git history. The lock lives in the settings folder, so
 * nothing extra appears among the documents. */
static void lock_data_dir(void)
{
    char path[sizeof(linux_config_dir) + 32];
    snprintf(path, sizeof(path), "%s/lock", linux_config_dir);
    int fd = open(path, O_RDWR | O_CREAT | O_CLOEXEC, 0600);
    if (fd < 0) return;
    if (flock(fd, LOCK_EX | LOCK_NB) != 0) {
        fprintf(stderr, "draftling: another Draftling is already running with %s\n",
                linux_config_dir);
        exit(1);
    }
    /* Kept open (and locked) for the life of the process; an exec for
     * esp_restart() keeps it too, as the descriptor is not inherited
     * (O_CLOEXEC) and the new program locks again. */
}

/* ---- leaving -------------------------------------------------------------- */

extern "C" void linux_quit(int status)
{
    static bool quitting;
    if (!quitting) {
        quitting = true;
        if (linux_pre_sleep_cb) linux_pre_sleep_cb();
    }
    linux_port_close_io();
    exit(status);
}

extern "C" void linux_restart(void)
{
    ESP_LOGI(TAG, "Restarting");
    linux_port_close_io();
    fflush(nullptr);
    execv(s_exe[0] ? s_exe : s_argv[0], s_argv);
    execvp(s_argv[0], s_argv);
    fprintf(stderr, "draftling: cannot restart: %s\n", strerror(errno));
    exit(1);
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

static void pre_sleep_linux(void)
{
    if (draftling_lvgl_port_lock(2000)) {
        editor_doc_foreach(pre_sleep_save_doc_cb, nullptr);
        draftling_lvgl_port_unlock();
    }
    sync();
}

/* ---- command line ---------------------------------------------------------- */

static const char *io_list(void)
{
    return ""
#if defined(LINUX_IO_WAYLAND)
        " wayland"
#endif
#if defined(LINUX_IO_X11)
        " x11"
#endif
#if defined(LINUX_IO_FBDEV)
        " fbdev"
#endif
        ;
}

static void usage(FILE *f)
{
    fprintf(f,
        "Usage: draftling [OPTION]...\n"
        "Distraction-free Markdown, Fountain and plain-text editor.\n"
        "\n"
        "  -d, --data-dir DIR   documents folder (default: $XDG_DATA_HOME/draftling,\n"
        "                       i.e. ~/.local/share/draftling)\n"
        "  -c, --config-dir DIR settings folder (default: $XDG_CONFIG_HOME/draftling,\n"
        "                       i.e. ~/.config/draftling)\n"
        "  -o, --io NAME        display and input to use; this build has:%s\n"
        "                       (default: the first of them that works)\n"
#if defined(LINUX_IO_FBDEV)
        "  -f, --fbdev DEVICE   framebuffer device for --io fbdev (default /dev/fb0)\n"
#endif
        "  -g, --geometry WxH   initial window size (Wayland, X11)\n"
        "  -v, --verbose        more messages on standard error (twice: debug)\n"
        "  -q, --quiet          errors only\n"
        "  -V, --version        show the version and exit\n"
        "  -h, --help           show this help and exit\n",
        io_list());
}

int main(int argc, char **argv)
{
    s_argv = argv;
#if defined(__linux__)
    ssize_t el = readlink("/proc/self/exe", s_exe, sizeof(s_exe) - 1);
    s_exe[el > 0 ? el : 0] = '\0';
#endif

    static const struct option opts[] = {
        { "data-dir",   required_argument, nullptr, 'd' },
        { "config-dir", required_argument, nullptr, 'c' },
        { "io",         required_argument, nullptr, 'o' },
        { "fbdev",      required_argument, nullptr, 'f' },
        { "geometry",   required_argument, nullptr, 'g' },
        { "verbose",    no_argument,       nullptr, 'v' },
        { "quiet",      no_argument,       nullptr, 'q' },
        { "version",    no_argument,       nullptr, 'V' },
        { "help",       no_argument,       nullptr, 'h' },
        { nullptr, 0, nullptr, 0 },
    };
    const char *data_arg = nullptr, *config_arg = nullptr, *io = nullptr;
    int c;
    while ((c = getopt_long(argc, argv, "d:c:o:f:g:vqVh", opts, nullptr)) != -1) {
        switch (c) {
        case 'd': data_arg = optarg; break;
        case 'c': config_arg = optarg; break;
        case 'o': io = optarg; break;
        case 'f':
#if defined(LINUX_IO_FBDEV)
            linux_fbdev_path = optarg;
#else
            fprintf(stderr, "draftling: built without framebuffer support\n");
            return 2;
#endif
            break;
        case 'g':
            if (sscanf(optarg, "%dx%d", &linux_window_w, &linux_window_h) != 2 ||
                linux_window_w < 100 || linux_window_h < 100 ||
                linux_window_w > 16384 || linux_window_h > 16384) {
                fprintf(stderr, "draftling: bad --geometry '%s' (expected e.g. 1024x640)\n", optarg);
                return 2;
            }
            break;
        case 'v': linux_log_level = linux_log_level < LINUX_LOG_INFO ? LINUX_LOG_INFO
                                                                     : LINUX_LOG_DEBUG; break;
        case 'q': linux_log_level = LINUX_LOG_ERROR; break;
        case 'V':
            printf("draftling %s (language pack: %s; I/O:%s)\n",
                   DRAFTLING_VERSION, DRAFTLING_LANG_PACK, io_list());
            return 0;
        case 'h': usage(stdout); return 0;
        default: usage(stderr); return 2;
        }
    }
    if (optind < argc) {
        usage(stderr);
        return 2;
    }

    if (data_arg) {
        if (!absolute(data_arg, linux_data_dir, sizeof(linux_data_dir))) return 1;
    } else {
        xdg_dir(linux_data_dir, sizeof(linux_data_dir), "XDG_DATA_HOME", ".local/share");
    }
    if (config_arg) {
        if (!absolute(config_arg, linux_config_dir, sizeof(linux_config_dir))) return 1;
    } else {
        xdg_dir(linux_config_dir, sizeof(linux_config_dir), "XDG_CONFIG_HOME", ".config");
    }
    if (!mkdir_p(linux_config_dir)) {
        fprintf(stderr, "draftling: cannot create %s: %s\n", linux_config_dir, strerror(errno));
        return 1;
    }
    lock_data_dir();
    setup_data_dir();

    /* The display first: without one there is nothing to do. */
    if (!linux_port_open_io(io)) return 1;

    nvs_flash_init();
    display_margins_init();
    display_orientation_init();
    display_flip_init();
    display_scale_init();

    int w, h;
    linux_port_initial_size(&w, &h);
    draftling_lvgl_port_init(w, h, 0, DISPLAY_SCALE);

    battery_init(BATT_ADC_PIN, BATT_EN_PIN, BATT_DIVIDER);

    /* Before the editor (unlike main.cpp): the network is usually up
     * already, and the editor draws the WiFi icon from
     * wifi_manager_is_connected() when it builds its screens. */
    wifi_manager_init();

    editor_ui_set_board_name(BOARD_NAME);
    editor_ui_init();

    if (sd_card_init(-1, -1, -1, MOUNT_POINT) != ESP_OK) {
        editor_ui_set_status("ERROR: data folder not usable");
    }
    ESP_LOGI(TAG, "Documents in %s, settings in %s", linux_data_dir, linux_config_dir);

    ble_keyboard_init();
    git_sync_init();
    standby_init();
    standby_set_pre_sleep_cb(pre_sleep_linux);

    ESP_LOGI(TAG, "Draftling %s ready (%s)", DRAFTLING_VERSION, linux_port_io_name());
    linux_port_run();
}
