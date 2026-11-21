/*
 * Linux: the display backend API. LVGL's frames reach the window
 * through lvgl_port_linux.c directly; the panel calls
 * here have nothing to do. The "controller" on the Hardware
 * information page is the I/O backend in use.
 *
 * Backlight (F1 -> Settings -> Display, Ctrl+B): the screen's own
 * backlight, /sys/class/backlight/<name>, when it can be set -- by
 * writing its brightness file (root, or a udev rule giving the user
 * write access) or else through systemd-logind's
 * Session.SetBrightness (any user with a local session, via busctl)
 * -- otherwise the window's pixels are dimmed (lvgl_port_linux.c).
 * With a backlight, the level it has at start is kept until the user
 * changes it (display_get_backlight()).
 */

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "display.h"
#include "esp_log.h"
#include "linux_host.h"

extern char **environ;

static const char *TAG = "Backlight";

#define BL_DIR "/sys/class/backlight"

const char *linux_backlight_name;

typedef enum { BL_NONE, BL_SYSFS, BL_LOGIND } bl_mode_t;

static bl_mode_t s_mode = BL_NONE;
static char s_name[128];        /* device under BL_DIR */
static long s_max;              /* max_brightness */
static int s_start_pct = -1;    /* the level at start, in percent */
static bool s_changed;          /* set to a level other than that */

static bool read_long(const char *name, const char *file, long *out)
{
    char path[512], buf[32];
    snprintf(path, sizeof(path), BL_DIR "/%s/%s", name, file);
    FILE *f = fopen(path, "r");
    if (!f) return false;
    bool ok = fgets(buf, sizeof(buf), f) != NULL;
    fclose(f);
    if (!ok) return false;
    char *end;
    errno = 0;
    *out = strtol(buf, &end, 10);
    return errno == 0 && end != buf;
}

static int type_rank(const char *name)
{
    char path[512], buf[32] = "";
    snprintf(path, sizeof(path), BL_DIR "/%s/type", name);
    FILE *f = fopen(path, "r");
    if (f) {
        if (!fgets(buf, sizeof(buf), f)) buf[0] = '\0';
        fclose(f);
    }
    /* As systemd-backlight: firmware (ACPI) over platform over raw. */
    if (strncmp(buf, "firmware", 8) == 0) return 0;
    if (strncmp(buf, "platform", 8) == 0) return 1;
    return 2;
}

/* The device to use: the best type, ties by name. */
static bool find_device(void)
{
    DIR *d = opendir(BL_DIR);
    if (!d) return false;
    struct dirent *e;
    int best = 99;
    s_name[0] = '\0';
    while ((e = readdir(d)) != NULL) {
        if (e->d_name[0] == '.') continue;
        long m;
        if (!read_long(e->d_name, "max_brightness", &m) || m <= 0) continue;
        int r = type_rank(e->d_name);
        if (r < best || (r == best && strcmp(e->d_name, s_name) < 0)) {
            best = r;
            snprintf(s_name, sizeof(s_name), "%s", e->d_name);
        }
    }
    closedir(d);
    return s_name[0] != '\0';
}

static bool write_sysfs(long v)
{
    char path[512], buf[32];
    snprintf(path, sizeof(path), BL_DIR "/%s/brightness", s_name);
    int fd = open(path, O_WRONLY | O_CLOEXEC);
    if (fd < 0) return false;
    int n = snprintf(buf, sizeof(buf), "%ld\n", v);
    bool ok = write(fd, buf, (size_t)n) == n;
    close(fd);
    return ok;
}

/* busctl call org.freedesktop.login1 .../session/auto
 *   org.freedesktop.login1.Session SetBrightness ssu backlight NAME V */
static bool write_logind(long v)
{
    char val[32];
    snprintf(val, sizeof(val), "%ld", v);
    char *argv[] = {
        (char *)"busctl", (char *)"--quiet", (char *)"call",
        (char *)"org.freedesktop.login1",
        (char *)"/org/freedesktop/login1/session/auto",
        (char *)"org.freedesktop.login1.Session", (char *)"SetBrightness",
        (char *)"ssu", (char *)"backlight", s_name, val, NULL,
    };
    posix_spawn_file_actions_t fa;
    posix_spawn_file_actions_init(&fa);
    posix_spawn_file_actions_addopen(&fa, STDIN_FILENO, "/dev/null", O_RDONLY, 0);
    posix_spawn_file_actions_addopen(&fa, STDOUT_FILENO, "/dev/null", O_WRONLY, 0);
    posix_spawn_file_actions_addopen(&fa, STDERR_FILENO, "/dev/null", O_WRONLY, 0);
    pid_t pid;
    int rc = posix_spawnp(&pid, argv[0], &fa, NULL, argv, environ);
    posix_spawn_file_actions_destroy(&fa);
    if (rc != 0) return false;
    /* A few milliseconds normally; give up after 2 s. */
    int st = 0;
    for (int i = 0; i < 200; i++) {
        pid_t r = waitpid(pid, &st, WNOHANG);
        if (r == pid) return WIFEXITED(st) && WEXITSTATUS(st) == 0;
        if (r < 0 && errno != EINTR) return false;
        struct timespec ts = { 0, 10 * 1000000L };
        nanosleep(&ts, NULL);
    }
    kill(pid, SIGKILL);
    waitpid(pid, &st, 0);
    return false;
}

static bool write_level(long v)
{
    if (s_mode == BL_SYSFS) return write_sysfs(v);
    if (s_mode == BL_LOGIND) return write_logind(v);
    return false;
}

void linux_backlight_init(void)
{
    const char *want = linux_backlight_name;
    if (want && strcmp(want, "none") == 0) {
        ESP_LOGI(TAG, "Dimming the window (--backlight none)");
        return;
    }
    if (want) {
        if (strchr(want, '/') || want[0] == '.') {
            ESP_LOGW(TAG, "Bad backlight name '%s'; dimming the window", want);
            return;
        }
        snprintf(s_name, sizeof(s_name), "%s", want);
        if (!read_long(s_name, "max_brightness", &s_max) || s_max <= 0) {
            ESP_LOGW(TAG, "No backlight %s/%s; dimming the window", BL_DIR, want);
            return;
        }
    } else if (!find_device() || !read_long(s_name, "max_brightness", &s_max)) {
        ESP_LOGI(TAG, "No screen backlight; dimming the window");
        return;
    }
    long cur;
    if (!read_long(s_name, "brightness", &cur)) cur = s_max;
    if (cur < 0) cur = 0;
    if (cur > s_max) cur = s_max;

    /* Writable? Writing the level it has changes nothing. */
    s_mode = BL_SYSFS;
    if (!write_level(cur)) {
        s_mode = BL_LOGIND;
        if (!write_level(cur)) {
            s_mode = BL_NONE;
            ESP_LOGW(TAG, "Backlight %s cannot be set (no write access, and "
                     "systemd-logind refused); dimming the window", s_name);
            return;
        }
    }
    s_start_pct = (int)((cur * 100 + s_max / 2) / s_max);
    ESP_LOGI(TAG, "Backlight %s (max %ld) at %d %%, set through %s", s_name, s_max,
             s_start_pct, s_mode == BL_SYSFS ? "sysfs" : "systemd-logind");
}

void display_init(int a, int b, int c, int d, int e, int f, int w, int h)
{
    (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; (void)w; (void)h;
}
void display_spi_tft_config(const display_spi_tft_config_t *cfg) { (void)cfg; }
void display_nv3041a_config(const display_nv3041a_config_t *cfg) { (void)cfg; }
void display_ssd1677_spi_config(const display_ssd1677_spi_config_t *cfg) { (void)cfg; }
void display_axs15231b_init(const display_axs15231b_config_t *cfg) { (void)cfg; }
/* Standby turns the "screen" -- the window -- black, and back on. */
void display_sleep(void) { linux_port_set_blank(true); }
void display_wake(void) { linux_port_set_blank(false); }
void display_set_invert(bool on) { (void)on; }
void display_set_shared_i2c_bus(void *bus) { (void)bus; }
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
void display_deep_sleep_prepare(void) { }
int display_get_backlight(void)
{
    return s_mode == BL_NONE ? -1 : s_start_pct;
}

void display_set_backlight(int percent)
{
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    if (s_mode == BL_NONE) {
        linux_port_set_dim(percent);
        return;
    }
    /* The editor applies the level at start: leave the screen as it
     * is, without a rounding step. */
    if (!s_changed && percent == s_start_pct) return;
    s_changed = true;
    long v = (s_max * percent + 50) / 100;
    if (v < 1 && percent > 0) v = 1;   /* 0 turns some screens off */
    if (!write_level(v)) ESP_LOGW(TAG, "Setting backlight %s to %ld failed", s_name, v);
}

const char *display_controller_name(void)
{
    static char name[32];
    snprintf(name, sizeof(name), "%s", linux_port_io_name());
    if (name[0] == 'x') snprintf(name, sizeof(name), "X11 window");
    else if (name[0] == 'w') snprintf(name, sizeof(name), "Wayland window");
    return name;
}
