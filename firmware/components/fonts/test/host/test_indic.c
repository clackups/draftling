/*
 * Host test of the shaped Indic scripts (indic_font.c with HarfBuzz):
 * shapes Devanagari, Bengali and Telugu sample lines, draws every glyph through
 * the font's own get_glyph_dsc() / get_glyph_bitmap() into an image
 * (out/indic.pgm, to look at), and checks that every shaped glyph has a
 * bitmap, that the cluster offsets never decrease, that the shaping
 * cache returns what shaping did, and the character classes. Run
 * ./run.sh.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lvgl.h"
#include "indic_font.h"

#define W 1600
#define H 1000
static uint8_t img[H][W];

typedef struct {
    indic_script_t script;
    const char *text;
} line_t;

/* Hindi, Marathi, Nepali, Sanskrit: conjuncts, reph, pre-base i,
 * ra + u ligatures, rakar, nukta. Bengali and Assamese: pre-base i / e,
 * split o / au, reph, ra- and ya-phala, k-ssa, jnya, khanda ta, nukta
 * letters, the danda. Telugu: vattu (below-base) conjuncts, three-
 * consonant stacks, the i / ii / e / o vowel signs joined to the
 * consonant, ra-vattu, vocalic r, the danda. */
static const line_t LINES[] = {
    { INDIC_DEVA, "\xE0\xA4\xB9\xE0\xA4\xBF\xE0\xA4\xA8\xE0\xA5\x8D\xE0\xA4\xA6\xE0\xA5\x80 \xE0\xA4\xAD\xE0\xA4\xBE\xE0\xA4\xB0\xE0\xA4\xA4" },
    { INDIC_DEVA, "\xE0\xA4\xB8\xE0\xA5\x8D\xE0\xA4\xA4\xE0\xA5\x8D\xE0\xA4\xB0\xE0\xA5\x80 \xE0\xA4\x95\xE0\xA5\x8D\xE0\xA4\xB7\xE0\xA4\xA4\xE0\xA5\x8D\xE0\xA4\xB0\xE0\xA4\xBF\xE0\xA4\xAF \xE0\xA4\xA6\xE0\xA5\x8D\xE0\xA4\xB5\xE0\xA4\xBF\xE0\xA4\xA4\xE0\xA5\x80\xE0\xA4\xAF" },
    { INDIC_DEVA, "\xE0\xA4\x95\xE0\xA4\xB0\xE0\xA5\x8D\xE0\xA4\xAE \xE0\xA4\xB0\xE0\xA5\x81\xE0\xA4\xAA\xE0\xA4\xAF\xE0\xA5\x87 \xE0\xA4\xB9\xE0\xA5\x83\xE0\xA4\xA6\xE0\xA4\xAF \xE0\xA4\x9F\xE0\xA5\x8D\xE0\xA4\xB0\xE0\xA5\x87\xE0\xA4\xA8" },
    { INDIC_DEVA, "\xE0\xA4\xAE\xE0\xA4\xB9\xE0\xA4\xBE\xE0\xA4\xB0\xE0\xA4\xBE\xE0\xA4\xB7\xE0\xA5\x8D\xE0\xA4\x9F\xE0\xA5\x8D\xE0\xA4\xB0 \xE0\xA4\xA8\xE0\xA5\x87\xE0\xA4\xAA\xE0\xA4\xBE\xE0\xA4\xB2 \xE0\xA4\xA1\xE0\xA4\xBC" },
    { INDIC_DEVA, "\xE0\xA4\xA7\xE0\xA4\xB0\xE0\xA5\x8D\xE0\xA4\xAE\xE0\xA4\x95\xE0\xA5\x8D\xE0\xA4\xB7\xE0\xA5\x87\xE0\xA4\xA4\xE0\xA5\x8D\xE0\xA4\xB0\xE0\xA5\x87 \xE0\xA4\x95\xE0\xA5\x81\xE0\xA4\xB0\xE0\xA5\x81\xE0\xA4\x95\xE0\xA5\x8D\xE0\xA4\xB7\xE0\xA5\x87\xE0\xA4\xA4\xE0\xA5\x8D\xE0\xA4\xB0\xE0\xA5\x87" },
    { INDIC_BENG, "\xE0\xA6\xAC\xE0\xA6\xBE\xE0\xA6\x82\xE0\xA6\xB2\xE0\xA6\xBE \xE0\xA6\xAD\xE0\xA6\xBE\xE0\xA6\xB7\xE0\xA6\xBE \xE0\xA6\x95\xE0\xA6\xBF \xE0\xA6\x95\xE0\xA7\x80" },
    { INDIC_BENG, "\xE0\xA6\x95\xE0\xA7\x8D\xE0\xA6\xB7\xE0\xA6\xAE\xE0\xA6\xBE \xE0\xA6\xB8\xE0\xA7\x8D\xE0\xA6\xA4\xE0\xA7\x8D\xE0\xA6\xB0\xE0\xA7\x80 \xE0\xA6\xB6\xE0\xA7\x8D\xE0\xA6\xB0\xE0\xA7\x80 \xE0\xA6\x9C\xE0\xA7\x8D\xE0\xA6\x9E\xE0\xA6\xBE\xE0\xA6\xA8" },
    { INDIC_BENG, "\xE0\xA6\x95\xE0\xA6\xB0\xE0\xA7\x8D\xE0\xA6\xAE \xE0\xA6\xAA\xE0\xA7\x8D\xE0\xA6\xB0\xE0\xA6\x95\xE0\xA7\x83\xE0\xA6\xA4\xE0\xA6\xBF \xE0\xA6\xAC\xE0\xA7\x8D\xE0\xA6\xAF\xE0\xA6\xBE\xE0\xA6\x82\xE0\xA6\x95 \xE0\xA6\x89\xE0\xA7\x8E\xE0\xA6\xB8\xE0\xA6\xAC" },
    { INDIC_BENG, "\xE0\xA6\xB0\xE0\xA6\xAC\xE0\xA7\x80\xE0\xA6\xA8\xE0\xA7\x8D\xE0\xA6\xA6\xE0\xA7\x8D\xE0\xA6\xB0\xE0\xA6\xA8\xE0\xA6\xBE\xE0\xA6\xA5 \xE0\xA6\xA0\xE0\xA6\xBE\xE0\xA6\x95\xE0\xA7\x81\xE0\xA6\xB0\xE0\xA5\xA4 \xE0\xA6\x95\xE0\xA7\x8C\xE0\xA6\xB6\xE0\xA6\xB2" },
    { INDIC_BENG, "\xE0\xA6\x85\xE0\xA6\xB8\xE0\xA6\xAE\xE0\xA7\x80\xE0\xA6\xAF\xE0\xA6\xBC\xE0\xA6\xBE \xE0\xA7\xB0\xE0\xA6\xBE\xE0\xA6\x9C\xE0\xA7\x8D\xE0\xA6\xAF \xE0\xA6\xA1\xE0\xA6\xBC\xE0\xA6\xA2\xE0\xA6\xBC\xE0\xA6\xAF\xE0\xA6\xBC \xE0\xA6\xB9\xE0\xA7\x8D\xE0\xA6\xAE" },    { INDIC_TELU, "\xE0\xB0\xA4\xE0\xB1\x86\xE0\xB0\xB2\xE0\xB1\x81\xE0\xB0\x97\xE0\xB1\x81 \xE0\xB0\xAD\xE0\xB0\xBE\xE0\xB0\xB7 \xE0\xB0\x95\xE0\xB0\xBF \xE0\xB0\x95\xE0\xB1\x80" },
    { INDIC_TELU, "\xE0\xB0\xB6\xE0\xB1\x8D\xE0\xB0\xB0\xE0\xB1\x80 \xE0\xB0\x95\xE0\xB1\x8D\xE0\xB0\xB7\xE0\xB1\x87\xE0\xB0\xA4\xE0\xB1\x8D\xE0\xB0\xB0\xE0\xB0\x82 \xE0\xB0\xB8\xE0\xB1\x8D\xE0\xB0\xA4\xE0\xB1\x8D\xE0\xB0\xB0\xE0\xB1\x80 \xE0\xB0\x9C\xE0\xB1\x8D\xE0\xB0\x9E\xE0\xB0\xBE\xE0\xB0\xA8\xE0\xB0\x82" },
    { INDIC_TELU, "\xE0\xB0\x86\xE0\xB0\x82\xE0\xB0\xA7\xE0\xB1\x8D\xE0\xB0\xB0 \xE0\xB0\xAA\xE0\xB1\x8D\xE0\xB0\xB0\xE0\xB0\xA6\xE0\xB1\x87\xE0\xB0\xB6\xE0\xB1\x8D \xE0\xB0\x95\xE0\xB1\x83\xE0\xB0\xB7\xE0\xB1\x8D\xE0\xB0\xA3 \xE0\xB0\x97\xE0\xB1\x8D\xE0\xB0\xB0\xE0\xB0\x82\xE0\xB0\xA5\xE0\xB0\x82" },
    { INDIC_TELU, "\xE0\xB0\xB9\xE0\xB1\x88\xE0\xB0\xA6\xE0\xB0\xB0\xE0\xB0\xBE\xE0\xB0\xAC\xE0\xB0\xBE\xE0\xB0\xA6\xE0\xB1\x8D \xE0\xB0\x89\xE0\xB0\xA6\xE0\xB1\x8D\xE0\xB0\xAF\xE0\xB1\x8B\xE0\xB0\x97\xE0\xB0\x82 \xE0\xB0\xB8\xE0\xB1\x8C\xE0\xB0\x82\xE0\xB0\xA6\xE0\xB0\xB0\xE0\xB1\x8D\xE0\xB0\xAF\xE0\xB0\x82\xE0\xA5\xA4 \xE0\xB0\x95\xE0\xB1\x8A\xE0\xB0\xA4\xE0\xB1\x8D\xE0\xB0\xA4" },
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
    /* Character classes. */
    check(indic_script(0x0915) == INDIC_DEVA, "ka is not Devanagari", -1);
    check(indic_script(0x0995) == INDIC_BENG, "ka is not Bengali", -1);
    check(indic_script(0x0C15) == INDIC_TELU, "ka is not Telugu", -1);
    check(indic_script(0x0964) == INDIC_ANY && indic_script(0x200D) == INDIC_ANY,
          "danda / joiner are not shared", -1);
    check(indic_script('a') == INDIC_NONE, "Latin is Indic", -1);
    check(indic_mid_cluster(0x0995, 0x09BF) && indic_mid_cluster(0x09CD, 0x09B7) &&
          indic_mid_cluster(0x0915, 0x094D) && !indic_mid_cluster(0x09BF, 0x0995) &&
          !indic_mid_cluster(' ', 0x0995) && indic_mid_cluster(0x0C15, 0x0C46) &&
          indic_mid_cluster(0x0C4D, 0x0C37) && !indic_mid_cluster(0x0C41, 0x0C15),
          "mid-cluster", -1);
    check(indic_cp_cells(0x09C1) == 0 && indic_cp_cells(0x0995) > 0 &&
          indic_cp_cells(0x0C3F) == 0 && indic_cp_cells(0x0C15) > 0 &&
          indic_cp_cells('a') == -1, "cells", -1);

    lv_font_t *f = indic_font_create(22, 9);   /* the 18 px slot, tall line */
    if (!f) {
        printf("FAIL: no font for the 18 px slot\n");
        return 1;
    }
    memset(img, 255, sizeof(img));
    int row = 0;
    for (int l = 0; l < (int)(sizeof(LINES) / sizeof(LINES[0])); l++) {
        const char *s = LINES[l].text;
        size_t len = strlen(s);
        static indic_glyph_t g[512], again[512];
        size_t n = indic_shape(LINES[l].script, s, len, g, 512);
        check(n > 0, "shaped to nothing", l);
        size_t n2 = indic_shape(LINES[l].script, s, len, again, 512);
        check(n2 == n && memcmp(g, again, n * sizeof(g[0])) == 0, "cache differs", l);
        int x = 4, base = row + (int)f->line_height - (int)f->base_line;
        for (size_t i = 0; i < n; i++) {
            if (i > 0) check(g[i].cluster >= g[i - 1].cluster, "cluster went back", l);
            /* Spaces are not part of a shaped run in the editor. */
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
    FILE *o = fopen(getenv("OUT_PGM") ? getenv("OUT_PGM") : "indic.pgm", "wb");
    if (o) {
        fprintf(o, "P5 %d %d 255\n", W, row + 2);
        for (int y = 0; y < row + 2; y++) fwrite(img[y], 1, W, o);
        fclose(o);
    }
    printf("%d failures\n", fails);
    return fails != 0;
}
