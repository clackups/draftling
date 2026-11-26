/*
 * Linux: the desktop clipboard, between the editor and the I/O backend.
 *
 * The editor's Ctrl+C / Ctrl+X / Ctrl+V call the editor_host_clipboard_*
 * hooks (weak no-ops on a device); they go to the backend in use
 * (Wayland data device or X11 CLIPBOARD selection). Text from other
 * programs comes back through linux_io_clipboard_text(), is cleaned up
 * here and handed to the editor.
 */

#include <stdlib.h>
#include <string.h>

#include "editor_ui.h"
#include "linux_host.h"

void editor_host_clipboard_set(const char *text, size_t len)
{
    const linux_io_backend_t *io = linux_port_io();
    if (io && io->clip_set) io->clip_set(text, len);
}

bool editor_host_clipboard_request(void)
{
    const linux_io_backend_t *io = linux_port_io();
    return io && io->clip_request && io->clip_request();
}

/* Length of the valid UTF-8 sequence at p (n bytes left), 0 if there
 * is none: no overlong forms, surrogates or code points past U+10FFFF. */
static size_t utf8_seq(const unsigned char *p, size_t n)
{
    unsigned char c = p[0];
    if (c < 0x80) return 1;
    size_t len;
    unsigned char lo = 0x80, hi = 0xBF;
    if (c >= 0xC2 && c <= 0xDF) len = 2;
    else if (c >= 0xE0 && c <= 0xEF) {
        len = 3;
        if (c == 0xE0) lo = 0xA0;
        else if (c == 0xED) hi = 0x9F;
    } else if (c >= 0xF0 && c <= 0xF4) {
        len = 4;
        if (c == 0xF0) lo = 0x90;
        else if (c == 0xF4) hi = 0x8F;
    } else {
        return 0;
    }
    if (n < len || p[1] < lo || p[1] > hi) return 0;
    for (size_t i = 2; i < len; i++)
        if (p[i] < 0x80 || p[i] > 0xBF) return 0;
    return len;
}

void linux_io_clipboard_text(const char *data, size_t len)
{
    if (!data) {
        editor_ui_host_clipboard_text(NULL, 0);
        return;
    }
    /* CR LF and lone CR become LF, NUL is dropped, and a byte that is
     * not valid UTF-8 is taken as Latin-1 (what the X11 STRING type
     * is): at most two bytes out per byte in. */
    char *out = (char *)malloc(len * 2 + 1);
    if (!out) {
        editor_ui_host_clipboard_text(NULL, 0);
        return;
    }
    const unsigned char *p = (const unsigned char *)data;
    size_t o = 0;
    for (size_t i = 0; i < len;) {
        unsigned char c = p[i];
        if (c == '\r') {
            out[o++] = '\n';
            i += (i + 1 < len && p[i + 1] == '\n') ? 2 : 1;
            continue;
        }
        if (c == 0) {
            i++;
            continue;
        }
        size_t n = utf8_seq(p + i, len - i);
        if (n) {
            memcpy(out + o, p + i, n);
            o += n;
            i += n;
        } else {
            out[o++] = (char)(0xC0 | (c >> 6));
            out[o++] = (char)(0x80 | (c & 0x3F));
            i++;
        }
    }
    out[o] = '\0';
    editor_ui_host_clipboard_text(out, o);
    free(out);
}

char *linux_clip_to_latin1(const char *text, size_t len, size_t *out_len)
{
    char *out = (char *)malloc(len + 1);
    if (!out) return NULL;
    const unsigned char *p = (const unsigned char *)text;
    size_t o = 0;
    for (size_t i = 0; i < len;) {
        size_t n = utf8_seq(p + i, len - i);
        if (n == 1) {
            out[o++] = (char)p[i];
        } else if (n == 2 && p[i] <= 0xC3) {
            out[o++] = (char)(((p[i] & 0x1F) << 6) | (p[i + 1] & 0x3F));
        } else {
            out[o++] = '?';
        }
        i += n ? n : 1;
    }
    out[o] = '\0';
    *out_len = o;
    return out;
}
