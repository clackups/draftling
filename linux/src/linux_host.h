/*
 * Linux: what the parts of the port share -- the command-line options,
 * the data and settings folders, the I/O backends (Wayland, X11,
 * framebuffer) and the hooks they call into the LVGL port.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

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

/* Pixels handed to a backend: 32-bit 0x00RRGGBB, rows of stride
 * pixels. */
typedef struct {
    const char *name;   /* "wayland", "x11", "fbdev" */
    /* Connect; on success *w / *h is the initial screen / window size.
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
    /* Mouse wheel and pointer cursor shape are window-only. */
    bool is_window;
} linux_io_backend_t;

#if defined(LINUX_IO_WAYLAND)
extern const linux_io_backend_t linux_io_wayland;
#endif
#if defined(LINUX_IO_X11)
extern const linux_io_backend_t linux_io_x11;
#endif
#if defined(LINUX_IO_FBDEV)
extern const linux_io_backend_t linux_io_fbdev;
/* Framebuffer device (--fbdev). */
extern const char *linux_fbdev_path;
#endif

/* Initial window size (--geometry), 0 = the Kconfig default. */
extern int linux_window_w, linux_window_h;
/* Window title. */
#define LINUX_WINDOW_TITLE "Draftling"

/* ---- hooks the backends call (lvgl_port_linux.c / keyboard_linux.c) ----- */

/* A key changed state; code is a Linux input event code (KEY_*), the
 * same on Wayland, X11 (keycode - 8) and evdev. */
void linux_io_key(int evdev_code, bool pressed);
/* Every key is up (focus lost, VT switched away). */
void linux_io_release_all_keys(void);
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

/* ---- LVGL port (lvgl_port_linux.c) --------------------------------------- */

/* Open a backend: the one named (NULL: Wayland, X11, framebuffer, the
 * first that works). Returns false with a message when none works. */
bool linux_port_open_io(const char *name);
const char *linux_port_io_name(void);
/* The screen / window size the backend opened with. */
void linux_port_initial_size(int *w, int *h);
/* The LVGL loop; never returns. */
__attribute__((noreturn)) void linux_port_run(void);
/* Leave the backend (restores the console on the framebuffer). */
void linux_port_close_io(void);

#ifdef __cplusplus
}
#endif
