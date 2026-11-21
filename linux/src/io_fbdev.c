/*
 * Linux: the framebuffer console (/dev/fb0) and evdev keyboards.
 *
 * The frame is copied into the mapped framebuffer (32 or 16 bits per
 * pixel, any channel layout the driver reports). Keys come from every
 * keyboard under /dev/input/event* (devices with letter keys), read as
 * Linux key codes; new keyboards are picked up every RESCAN_MS. This
 * needs read access to /dev/input/event* (the "input" group) and write
 * access to the framebuffer (the "video" group).
 *
 * When started on a virtual console, the console is put into graphics
 * mode (no text cursor drawn over the picture) and its keyboard is
 * switched off (K_OFF), so the keys typed do not also reach the shell
 * underneath; both are restored when the program ends, also on a
 * crash signal. Switching consoles with Ctrl+Alt+Fn is not available
 * meanwhile. The screen size is fixed: there is no resize.
 */


#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include <linux/fb.h>
#include <linux/input.h>
#include <linux/kd.h>
#include <linux/vt.h>

#include "esp_log.h"
#include "linux_host.h"

static const char *TAG = "Framebuffer";

#define MAX_KBDS 16
#define RESCAN_MS 2000

static int s_fb = -1;
static uint8_t *s_map;
static size_t s_map_len;
static struct fb_var_screeninfo s_var;
static struct fb_fix_screeninfo s_fix;
static int s_w, s_h, s_bpp;

static int s_kbd_fd[MAX_KBDS];
static char s_kbd_path[MAX_KBDS][64];
static int s_n_kbd;
static uint32_t s_last_scan;

static int s_tty = -1;
static int s_old_kbmode = -1;
static bool s_kd_graphics;
static struct termios s_old_tio;
static bool s_tio_saved;

static uint32_t now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)((uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u);
}

/* ---- console ---------------------------------------------------------------- */

static void console_restore(void)
{
    if (s_tty < 0) return;
    if (s_old_kbmode >= 0) ioctl(s_tty, KDSKBMODE, s_old_kbmode);
    if (s_kd_graphics) ioctl(s_tty, KDSETMODE, KD_TEXT);
    if (s_tio_saved) {
        tcflush(s_tty, TCIFLUSH);
        tcsetattr(s_tty, TCSANOW, &s_old_tio);
    }
    s_old_kbmode = -1;
    s_kd_graphics = false;
    s_tio_saved = false;
    close(s_tty);
    s_tty = -1;
}

static void on_crash(int sig)
{
    console_restore();
    signal(sig, SIG_DFL);
    raise(sig);
}

static void console_setup(void)
{
    /* Standard input, if it is a virtual console. */
    int mode;
    if (ioctl(STDIN_FILENO, KDGETMODE, &mode) != 0) {
        ESP_LOGI(TAG, "Not on a virtual console: the console is left as it is");
        return;
    }
    s_tty = dup(STDIN_FILENO);
    if (s_tty < 0) return;
    fcntl(s_tty, F_SETFD, FD_CLOEXEC);
    if (tcgetattr(s_tty, &s_old_tio) == 0) {
        s_tio_saved = true;
        struct termios t = s_old_tio;
        t.c_lflag &= (tcflag_t)~(ICANON | ECHO | ISIG);
        tcsetattr(s_tty, TCSANOW, &t);
    }
    int kbmode;
    if (ioctl(s_tty, KDGKBMODE, &kbmode) == 0 && ioctl(s_tty, KDSKBMODE, K_OFF) == 0)
        s_old_kbmode = kbmode;
    if (ioctl(s_tty, KDSETMODE, KD_GRAPHICS) == 0) s_kd_graphics = true;
    static const int sigs[] = { SIGSEGV, SIGBUS, SIGFPE, SIGILL, SIGABRT };
    for (size_t i = 0; i < sizeof(sigs) / sizeof(sigs[0]); i++) signal(sigs[i], on_crash);
    atexit(console_restore);
}

/* ---- keyboards -------------------------------------------------------------- */

#define BITS_LONG (sizeof(long) * 8)
#define TEST_BIT(b, arr) (((arr)[(b) / BITS_LONG] >> ((b) % BITS_LONG)) & 1)

static bool is_keyboard(int fd)
{
    unsigned long ev[(EV_MAX + BITS_LONG) / BITS_LONG] = { 0 };
    unsigned long keys[(KEY_MAX + BITS_LONG) / BITS_LONG] = { 0 };
    if (ioctl(fd, EVIOCGBIT(0, sizeof(ev)), ev) < 0 || !TEST_BIT(EV_KEY, ev)) return false;
    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keys)), keys) < 0) return false;
    return TEST_BIT(KEY_A, keys) && TEST_BIT(KEY_Z, keys) && TEST_BIT(KEY_ENTER, keys);
}

static bool have_path(const char *p)
{
    for (int i = 0; i < s_n_kbd; i++)
        if (strcmp(s_kbd_path[i], p) == 0) return true;
    return false;
}

static void scan_keyboards(bool verbose)
{
    s_last_scan = now_ms();
    DIR *d = opendir("/dev/input");
    if (!d) return;
    struct dirent *e;
    bool denied = false;
    while ((e = readdir(d)) != NULL && s_n_kbd < MAX_KBDS) {
        if (strncmp(e->d_name, "event", 5) != 0) continue;
        char path[64];
        snprintf(path, sizeof(path), "/dev/input/%s", e->d_name);
        if (have_path(path)) continue;
        int fd = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (fd < 0) {
            if (errno == EACCES) denied = true;
            continue;
        }
        if (!is_keyboard(fd)) {
            close(fd);
            continue;
        }
        char name[128] = "?";
        ioctl(fd, EVIOCGNAME(sizeof(name)), name);
        ESP_LOGI(TAG, "Keyboard %s (%s)", path, name);
        s_kbd_fd[s_n_kbd] = fd;
        snprintf(s_kbd_path[s_n_kbd], sizeof(s_kbd_path[0]), "%s", path);
        s_n_kbd++;
    }
    closedir(d);
    if (verbose && s_n_kbd == 0) {
        fprintf(stderr, "draftling: no keyboard found under /dev/input%s\n",
                denied ? " (permission denied: add yourself to the \"input\" group)" : "");
    }
}

static void drop_keyboard(int i)
{
    ESP_LOGI(TAG, "Keyboard %s gone", s_kbd_path[i]);
    close(s_kbd_fd[i]);
    for (int j = i; j + 1 < s_n_kbd; j++) {
        s_kbd_fd[j] = s_kbd_fd[j + 1];
        memcpy(s_kbd_path[j], s_kbd_path[j + 1], sizeof(s_kbd_path[0]));
    }
    s_n_kbd--;
    linux_io_release_all_keys();
}

/* ---- backend -------------------------------------------------------------------- */

static bool fb_open(int *w, int *h)
{
    s_fb = open(linux_fbdev_path, O_RDWR | O_CLOEXEC);
    if (s_fb < 0) {
        ESP_LOGI(TAG, "%s: %s%s", linux_fbdev_path, strerror(errno),
                 errno == EACCES ? " (add yourself to the \"video\" group)" : "");
        if (errno == EACCES)
            fprintf(stderr, "draftling: %s: permission denied (the \"video\" group)\n",
                    linux_fbdev_path);
        return false;
    }
    if (ioctl(s_fb, FBIOGET_VSCREENINFO, &s_var) != 0 ||
        ioctl(s_fb, FBIOGET_FSCREENINFO, &s_fix) != 0) {
        ESP_LOGW(TAG, "%s is not a framebuffer", linux_fbdev_path);
        close(s_fb);
        s_fb = -1;
        return false;
    }
    s_bpp = (int)s_var.bits_per_pixel;
    if ((s_bpp != 32 && s_bpp != 16) || s_fix.type != FB_TYPE_PACKED_PIXELS ||
        s_fix.visual != FB_VISUAL_TRUECOLOR) {
        fprintf(stderr, "draftling: %s: needs a 16- or 32-bit true-color framebuffer\n",
                linux_fbdev_path);
        close(s_fb);
        s_fb = -1;
        return false;
    }
    s_w = (int)s_var.xres;
    s_h = (int)s_var.yres;
    s_map_len = (size_t)s_fix.smem_len;
    s_map = mmap(NULL, s_map_len, PROT_READ | PROT_WRITE, MAP_SHARED, s_fb, 0);
    if (s_map == MAP_FAILED) {
        ESP_LOGW(TAG, "Cannot map %s: %s", linux_fbdev_path, strerror(errno));
        close(s_fb);
        s_fb = -1;
        s_map = NULL;
        return false;
    }
    scan_keyboards(true);
    console_setup();
    *w = s_w;
    *h = s_h;
    return true;
}

/* The main loop polls one descriptor: the first keyboard. Others are
 * read at the latest after the loop's longest wait (50 ms). */
static int fb_fd(void)
{
    return s_n_kbd > 0 ? s_kbd_fd[0] : -1;
}

static void fb_dispatch(void)
{
    for (int i = 0; i < s_n_kbd; i++) {
        struct input_event ev[64];
        for (;;) {
            ssize_t r = read(s_kbd_fd[i], ev, sizeof(ev));
            if (r < 0) {
                if (errno == EAGAIN || errno == EINTR) break;
                drop_keyboard(i);   /* unplugged (ENODEV) */
                i--;
                break;
            }
            if (r == 0) break;
            for (size_t k = 0; k < (size_t)r / sizeof(ev[0]); k++) {
                if (ev[k].type != EV_KEY || ev[k].value == 2) continue;   /* 2: repeat */
                linux_io_key(ev[k].code, ev[k].value == 1);
            }
        }
    }
    if ((int32_t)(now_ms() - s_last_scan) >= RESCAN_MS) scan_keyboards(false);
}

static inline uint32_t pack(uint32_t xrgb)
{
    uint32_t r = (xrgb >> 16) & 0xFF, g = (xrgb >> 8) & 0xFF, b = xrgb & 0xFF;
    r >>= 8 - s_var.red.length;
    g >>= 8 - s_var.green.length;
    b >>= 8 - s_var.blue.length;
    return (r << s_var.red.offset) | (g << s_var.green.offset) | (b << s_var.blue.offset);
}

static void fb_present(const uint32_t *fb, int fb_w, int fb_h, int x, int y, int w, int h)
{
    (void)fb_h;
    if (!s_map) return;
    size_t line = s_fix.line_length;
    uint8_t *base = s_map + (size_t)s_var.yoffset * line + (size_t)s_var.xoffset * (size_t)(s_bpp / 8);
    for (int r = 0; r < h; r++) {
        int py = y + r;
        if (py >= s_h) break;
        const uint32_t *src = fb + (size_t)py * fb_w + x;
        uint8_t *dst = base + (size_t)py * line;
        if (s_bpp == 32) {
            uint32_t *d = (uint32_t *)dst + x;
            for (int i = 0; i < w && x + i < s_w; i++) d[i] = pack(src[i]);
        } else {
            uint16_t *d = (uint16_t *)dst + x;
            for (int i = 0; i < w && x + i < s_w; i++) d[i] = (uint16_t)pack(src[i]);
        }
    }
}

static void fb_close(void)
{
    console_restore();
    for (int i = 0; i < s_n_kbd; i++) close(s_kbd_fd[i]);
    s_n_kbd = 0;
    if (s_map) {
        memset(s_map, 0, s_map_len);
        munmap(s_map, s_map_len);
        s_map = NULL;
    }
    if (s_fb >= 0) close(s_fb);
    s_fb = -1;
}

const linux_io_backend_t linux_io_fbdev = {
    .name = "fbdev",
    .open = fb_open,
    .fd = fb_fd,
    .dispatch = fb_dispatch,
    .present = fb_present,
    .flush = NULL,
    .close = fb_close,
    .is_window = false,
};
