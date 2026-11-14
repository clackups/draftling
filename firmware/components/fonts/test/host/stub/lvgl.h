/* Minimal LVGL stand-in for the host test of deva_font.c: only the
 * font types and functions it uses. */
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#define LV_UNUSED(x) (void)(x)
typedef enum { LV_FONT_GLYPH_FORMAT_NONE=0, LV_FONT_GLYPH_FORMAT_A1=1 } lv_font_glyph_format_t;
enum { LV_FONT_SUBPX_NONE = 0 };
struct _lv_font_t;
typedef struct { uint32_t stride; } lv_draw_buf_header_t;
typedef struct { lv_draw_buf_header_t header; uint8_t *data; } lv_draw_buf_t;
typedef struct {
    const struct _lv_font_t *resolved_font;
    uint16_t adv_w, box_w, box_h; int16_t ofs_x, ofs_y; uint8_t stride;
    lv_font_glyph_format_t format; uint8_t is_placeholder;
    union { uint32_t index; const void *src; } gid;
} lv_font_glyph_dsc_t;
typedef struct _lv_font_t {
    bool (*get_glyph_dsc)(const struct _lv_font_t *, lv_font_glyph_dsc_t *, uint32_t, uint32_t);
    const void *(*get_glyph_bitmap)(lv_font_glyph_dsc_t *, lv_draw_buf_t *);
    int32_t line_height, base_line; uint8_t subpx; int8_t underline_position, underline_thickness;
    const void *dsc; const struct _lv_font_t *fallback; void *user_data;
} lv_font_t;
static inline void lv_draw_buf_flush_cache(lv_draw_buf_t *b, void *a) { (void)b; (void)a; }
