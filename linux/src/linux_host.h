/*
 * Linux: what the parts of the port share -- the command-line options,
 * the data and settings folders, the I/O backends (a Wayland or X11
 * window) and the hooks they call into the LVGL port.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "sdkconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- options and folders (main_linux.cpp) ------------------------------- */

/* The documents (the "SD card"). The process runs with this as its
 * current directory and the SD card layer is mounted at ".", so
 * the firmware's fixed-size path buffers never see a long absolute
 * path. */
extern char linux_data_dir[4096];
/* Settings (the "NVS" file) live here. */
extern char linux_config_dir[4096];
/* True when the data folder carries the marker Draftling puts into a
 * folder it created (or found empty): only then may "Format SD card"
 * empty it. */
extern bool linux_data_dir_owned;
/* Marker file name in the data folder. */
#define LINUX_DATA_MARKER ".draftling-data"

/* Restore the console and start the program again with the same
 * arguments (esp_restart()). */
__attribute__((noreturn)) void linux_restart(void);
/* Run the pre-sleep hook (save every document) and leave the program
 * (standby_enter_sleep(), "Sleep now", closing the window). */
__attribute__((noreturn)) void linux_quit(int status);

/* ---- I/O backends --------------------------------------------------------- */

struct pollfd;

/* Pixels handed to a backend: 32-bit 0x00RRGGBB, rows of stride
 * pixels. */
typedef struct {
    const char *name;   /* "wayland", "x11" */
    /* Connect; on success *w / *h is the initial window size.
     * Returns false (quietly, unless verbose) when this backend is not
     * available here, so the next one can be tried. */
    bool (*open)(int *w, int *h);
    /* A file descriptor to poll for input, or -1. */
    int (*fd)(void);
    /* Handle pending input / window events; calls the linux_io_* hooks
     * below. */
    void (*dispatch)(void);
    /* Show the rectangle (x, y, w, h) of the frame (the whole window
     * size, fb_w x fb_h). The backend may show more. */
    void (*present)(const uint32_t *fb, int fb_w, int fb_h, int x, int y, int w, int h);
    /* Called before waiting: flush requests (Wayland, X11). */
    void (*flush)(void);
    void (*close)(void);
    /* The server sends a held key's auto-repeat as further presses
     * (X11); otherwise a second press means the release was lost. */
    bool repeats_keys;
    /* Desktop clipboard (clipboard_linux.c). clip_set: take it over
     * with this UTF-8 text (the backend keeps a copy); NULL lets go of
     * it. clip_request: ask for its text; false when Draftling owns it
     * or nothing offers text, otherwise linux_io_clipboard_text()
     * follows from dispatch() (NULL: it failed). Either may be NULL. */
    void (*clip_set)(const char *text, size_t len);
    bool (*clip_request)(void);
    /* More file descriptors to wait on (clipboard transfers): fills at
     * most max entries, returns how many. May be NULL. */
    int (*poll_fds)(struct pollfd *fds, int max);
    /* Wait up to timeout_ms for input, instead of polling fd() and
     * poll_fds() (a backend without file descriptors: the web client's
     * canvas). May be NULL. */
    void (*wait)(int timeout_ms);
} linux_io_backend_t;

#if defined(LINUX_IO_WAYLAND)
extern const linux_io_backend_t linux_io_wayland;
#endif
#if defined(LINUX_IO_X11)
extern const linux_io_backend_t linux_io_x11;
#endif
#if defined(LINUX_IO_WEB)
/* The web client's canvas (web_client/src/io_web.c). */
extern const linux_io_backend_t linux_io_web;
#endif

/* True while the backend --io names is being opened: a backend that
 * finds no display then says so as a warning, not only with -v (when
 * trying each in turn, a missing display is the normal case). */
extern bool linux_io_explicit;

/* Set by the port from the backend in use (repeats_keys). */
extern bool linux_io_repeats_keys;

/* Initial window size (--geometry), 0 = the Kconfig default. */
extern int linux_window_w, linux_window_h;
/* Window title. */
#define LINUX_WINDOW_TITLE "Draftling"
/* Wayland app id: the desktop file's name, so the desktop finds its
 * icon and name (draftling.desktop; the Flatpak renames it to its app
 * id). */
#if defined(CONFIG_DRAFTLING_LINUX_FLATPAK)
#define LINUX_APP_ID "art.draftling.Draftling"
#else
#define LINUX_APP_ID "draftling"
#endif

/* ---- hooks the backends call (lvgl_port_linux.c / keyboard_linux.c) ----- */

/* A key changed state; code is a Linux input event code (KEY_*), the
 * same on Wayland and X11 (keycode - 8). */
void linux_io_key(int evdev_code, bool pressed);
/* Every key is up (focus lost, VT switched away). */
void linux_io_release_all_keys(void);
/* The modifiers the display server says are held now (LINUX_MOD_*).
 * Clears the ones the keyboard layer thinks are held but are not --
 * their release went elsewhere (a desktop shortcut, a grab) -- and
 * never sets any. */
#define LINUX_MOD_SHIFT 0x01
#define LINUX_MOD_CTRL  0x02
#define LINUX_MOD_ALT   0x04   /* Mod1 */
#define LINUX_MOD_ALTGR 0x08   /* Mod5 / ISO_Level3: Right Alt on AltGr keymaps */
#define LINUX_MOD_SUPER 0x10   /* Mod4 */
void linux_io_sync_modifiers(unsigned held);
/* Pointer position in window pixels and the primary button. */
void linux_io_pointer(int x, int y, bool pressed);
/* Mouse wheel: steps > 0 scroll down. */
void linux_io_wheel(int steps);
/* The window has a new size. */
void linux_io_resize(int w, int h);
/* The window should be redrawn in full (exposed, or a new buffer). */
void linux_io_damage_all(void);
/* The user closed the window. */
void linux_io_close(void);
/* The clipboard text clip_request() asked for, in any encoding the
 * backend got (NULL: none). */
void linux_io_clipboard_text(const char *data, size_t len);
/* UTF-8 to Latin-1 ('?' for what it lacks), for the X11 STRING type;
 * malloc()ed, NUL-terminated. */
char *linux_clip_to_latin1(const char *text, size_t len, size_t *out_len);

/* ---- standby (standby_linux.c) ------------------------------------------ */

/* User input arrived: restart the inactivity timeout and, when the
 * screen is off (standby), turn it on. Returns true when it was off:
 * the input then only wakes and is not passed on. */
bool linux_standby_wake(void);

/* ---- backlight (display_linux.c) ----------------------------------------- */

/* --backlight: a device under /sys/class/backlight, "none" (dim the
 * window), or NULL (the best device, if it can be set). */
extern const char *linux_backlight_name;
/* Pick the backlight and check it can be set; before editor_ui_init(),
 * which applies the saved level. */
void linux_backlight_init(void);

/* ---- LVGL port (lvgl_port_linux.c) --------------------------------------- */

/* Turn the window black (standby) or show the picture again. */
void linux_port_set_blank(bool blank);
/* Without a screen backlight: show the window's pixels at this
 * brightness, in percent (100 = as drawn). */
void linux_port_set_dim(int percent);

/* Open a backend: the one named (NULL: Wayland, then X11, the first
 * that works). Returns false with a message when none works. */
bool linux_port_open_io(const char *name);
const char *linux_port_io_name(void);
/* The backend in use, or NULL. */
const linux_io_backend_t *linux_port_io(void);
/* The window size the backend opened with. */
void linux_port_initial_size(int *w, int *h);
/* The LVGL loop; never returns. */
__attribute__((noreturn)) void linux_port_run(void);
/* Leave the backend (closes the window). */
void linux_port_close_io(void);

#ifdef __cplusplus
}
#endif
