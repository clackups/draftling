/*
 * Host test of the Devanagari font (deva_font.c with HarfBuzz): shapes
 * sample lines, draws every glyph through the font's own
 * get_glyph_dsc() / get_glyph_bitmap() into an image (out/deva.pgm, to
 * look at), and checks that every shaped glyph has a bitmap, that the
 * cluster offsets never decrease and that the shaping cache returns
 * what shaping did. Run ./run.sh.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lvgl.h"
#include "deva_font.h"

#define W 1600
#define H 400
static uint8_t img[H][W];

/* Hindi, Marathi, Nepali, Sanskrit: conjuncts, reph, pre-base i,
 * ra + u ligatures, rakar, nukta. */
static const char *const LINES[] = {
    "\xE0\xA4\xB9\xE0\xA4\xBF\xE0\xA4\xA8\xE0\xA5\x8D\xE0\xA4\xA6\xE0\xA5\x80 \xE0\xA4\xAD\xE0\xA4\xBE\xE0\xA4\xB0\xE0\xA4\xA4",
    "\xE0\xA4\xB8\xE0\xA5\x8D\xE0\xA4\xA4\xE0\xA5\x8D\xE0\xA4\xB0\xE0\xA5\x80 \xE0\xA4\x95\xE0\xA5\x8D\xE0\xA4\xB7\xE0\xA4\xA4\xE0\xA5\x8D\xE0\xA4\xB0\xE0\xA4\xBF\xE0\xA4\xAF \xE0\xA4\xA6\xE0\xA5\x8D\xE0\xA4\xB5\xE0\xA4\xBF\xE0\xA4\xA4\xE0\xA5\x80\xE0\xA4\xAF",
    "\xE0\xA4\x95\xE0\xA4\xB0\xE0\xA5\x8D\xE0\xA4\xAE \xE0\xA4\xB0\xE0\xA5\x81\xE0\xA4\xAA\xE0\xA4\xAF\xE0\xA5\x87 \xE0\xA4\xB9\xE0\xA5\x83\xE0\xA4\xA6\xE0\xA4\xAF \xE0\xA4\x9F\xE0\xA5\x8D\xE0\xA4\xB0\xE0\xA5\x87\xE0\xA4\xA8",
    "\xE0\xA4\xAE\xE0\xA4\xB9\xE0\xA4\xBE\xE0\xA4\xB0\xE0\xA4\xBE\xE0\xA4\xB7\xE0\xA5\x8D\xE0\xA4\x9F\xE0\xA5\x8D\xE0\xA4\xB0 \xE0\xA4\xA8\xE0\xA5\x87\xE0\xA4\xAA\xE0\xA4\xBE\xE0\xA4\xB2 \xE0\xA4\xA1\xE0\xA4\xBC",
    "\xE0\xA4\xA7\xE0\xA4\xB0\xE0\xA5\x8D\xE0\xA4\xAE\xE0\xA4\x95\xE0\xA5\x8D\xE0\xA4\xB7\xE0\xA5\x87\xE0\xA4\xA4\xE0\xA5\x8D\xE0\xA4\xB0\xE0\xA5\x87 \xE0\xA4\x95\xE0\xA5\x81\xE0\xA4\xB0\xE0\xA5\x81\xE0\xA4\x95\xE0\xA5\x8D\xE0\xA4\xB7\xE0\xA5\x87\xE0\xA4\xA4\xE0\xA5\x8D\xE0\xA4\xB0\xE0\xA5\x87",
};

static int fails;

static void check(int ok, const char *what, int line)
{
    if (!ok) {
        fails++;
        printf("FAIL line %d: %s\n", line, what);
    }
}

int main(void)
{
    lv_font_t *f = deva_font_create(22, 9);   /* the 18 px slot, tall line */
    if (!f) {
        printf("FAIL: no font for the 18 px slot\n");
        return 1;
    }
    memset(img, 255, sizeof(img));
    int row = 0;
    for (int l = 0; l < (int)(sizeof(LINES) / sizeof(LINES[0])); l++) {
        const char *s = LINES[l];
        size_t len = strlen(s);
        static deva_glyph_t g[512], again[512];
        size_t n = deva_shape(s, len, g, 512);
        check(n > 0, "shaped to nothing", l);
        size_t n2 = deva_shape(s, len, again, 512);
        check(n2 == n && memcmp(g, again, n * sizeof(g[0])) == 0, "cache differs", l);
        int x = 4, base = row + (int)f->line_height - (int)f->base_line;
        for (size_t i = 0; i < n; i++) {
            if (i > 0) check(g[i].cluster >= g[i - 1].cluster, "cluster went back", l);
            /* Spaces are not part of a Devanagari run in the editor. */
            if (s[g[i].cluster] == ' ') { x += 6; continue; }
            lv_font_glyph_dsc_t d;
            memset(&d, 0, sizeof(d));
            bool ok = f->get_glyph_dsc(f, &d, g[i].code, i + 1 < n ? g[i + 1].code : 0);
            check(ok, "glyph without descriptor", l);
            if (!ok) continue;
            d.resolved_font = f;
            static uint8_t buf[256 * 256];
            lv_draw_buf_t db = { { d.box_w ? d.box_w : 1u }, buf };
            if (d.box_w && d.box_h) {
                check(f->get_glyph_bitmap(&d, &db) != NULL, "glyph without bitmap", l);
                for (int yy = 0; yy < d.box_h; yy++)
                    for (int xx = 0; xx < d.box_w; xx++)
                        if (buf[yy * db.header.stride + xx]) {
                            int X = x + d.ofs_x + xx, Y = base - d.box_h - d.ofs_y + yy;
                            if (X >= 0 && X < W && Y >= 0 && Y < H) img[Y][X] = 0;
                        }
            }
            x += d.adv_w;
        }
        printf("ok   line %d: %zu glyphs, %d px\n", l, n, x);
        row += (int)f->line_height;
    }
    FILE *o = fopen(getenv("OUT_PGM") ? getenv("OUT_PGM") : "deva.pgm", "wb");
    if (o) {
        fprintf(o, "P5 %d %d 255\n", W, row + 2);
        for (int y = 0; y < row + 2; y++) fwrite(img[y], 1, W, o);
        fclose(o);
    }
    printf("%d failures\n", fails);
    return fails != 0;
}
