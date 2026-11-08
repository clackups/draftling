/* epd_fb.h -- the 1-bpp framebuffer and dirty-rectangle bookkeeping
 * shared by the e-paper backends that keep their own framebuffer
 * (display_ws_epd397.cpp, display_m5_papermono.cpp,
 * display_reterminal_sticky.cpp, display_xteink_epd.cpp,
 * display_seeed_e1001.cpp, display_h752.cpp).
 *
 * One bit per panel pixel, MSB first, 1 = white, 0 = black; rows are
 * width / 8 bytes. The dirty rectangle is the inclusive bounding box
 * of everything drawn since epd_fb_clear_dirty() (x0 < 0 = nothing),
 * and area the sum of the drawn rectangles' areas, which some backends
 * use to promote a large update to a full refresh. */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    uint8_t *buf;
    int      width, height;          /* panel pixels */
    int      x0, y0, x1, y1;         /* dirty box, inclusive; x0 < 0 = clean */
    long     area;                   /* summed area of the drawn rectangles */
} epd_fb_t;

#define EPD_FB_INIT(w, h) { NULL, (w), (h), -1, -1, -1, -1, 0 }

/* Green is the highest-resolution RGB565 channel and a good enough
 * threshold for Draftling's black / white LVGL output. */
static inline bool epd_rgb565_is_black(uint16_t v)
{
    return ((v >> 5) & 0x3F) < 32;
}

static inline void epd_fb_clear_dirty(epd_fb_t *f)
{
    f->x0 = f->y0 = f->x1 = f->y1 = -1;
    f->area = 0;
}

static inline void epd_fb_mark_dirty(epd_fb_t *f, int x, int y, int w, int h)
{
    if (w <= 0 || h <= 0) return;
    int x0 = x > 0 ? x : 0;
    int y0 = y > 0 ? y : 0;
    int x1 = x + w < f->width  ? x + w : f->width;
    int y1 = y + h < f->height ? y + h : f->height;
    if (x1 <= x0 || y1 <= y0) return;
    f->area += (long)(x1 - x0) * (long)(y1 - y0);
    if (f->x0 < 0) {
        f->x0 = x0; f->y0 = y0;
        f->x1 = x1 - 1; f->y1 = y1 - 1;
        return;
    }
    if (x0 < f->x0) f->x0 = x0;
    if (y0 < f->y0) f->y0 = y0;
    if (x1 - 1 > f->x1) f->x1 = x1 - 1;
    if (y1 - 1 > f->y1) f->y1 = y1 - 1;
}

static inline void epd_fb_set_pixel(epd_fb_t *f, int x, int y, bool black)
{
    if ((unsigned)x >= (unsigned)f->width || (unsigned)y >= (unsigned)f->height) return;
    size_t idx = (size_t)y * (size_t)(f->width / 8) + (size_t)(x >> 3);
    uint8_t mask = (uint8_t)(0x80U >> (x & 7));
    if (black) f->buf[idx] &= (uint8_t)~mask;
    else       f->buf[idx] |= mask;
}

static inline void epd_fb_fill_rect(epd_fb_t *f, int x, int y, int w, int h, bool black)
{
    if (!f->buf || w <= 0 || h <= 0) return;
    int x0 = x > 0 ? x : 0;
    int y0 = y > 0 ? y : 0;
    int x1 = x + w < f->width  ? x + w : f->width;
    int y1 = y + h < f->height ? y + h : f->height;
    if (x1 <= x0 || y1 <= y0) return;
    size_t row = (size_t)(f->width / 8);
    if (x0 == 0 && x1 == f->width) {
        memset(f->buf + (size_t)y0 * row, black ? 0x00 : 0xFF, (size_t)(y1 - y0) * row);
        return;
    }
    for (int py = y0; py < y1; ++py)
        for (int px = x0; px < x1; ++px) epd_fb_set_pixel(f, px, py, black);
}

/* Draw a tightly packed w x h RGB565 tile at panel position (x, y) and
 * mark it dirty. */
static inline void epd_fb_push_rgb565(epd_fb_t *f, int x, int y, int w, int h,
                                      const void *color_map)
{
    const uint16_t *src = (const uint16_t *)color_map;
    for (int sy = 0; sy < h; ++sy)
        for (int sx = 0; sx < w; ++sx)
            epd_fb_set_pixel(f, x + sx, y + sy,
                             epd_rgb565_is_black(src[(size_t)sy * w + sx]));
    epd_fb_mark_dirty(f, x, y, w, h);
}
