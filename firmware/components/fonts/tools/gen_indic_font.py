#!/usr/bin/env python3
"""Generate the font data of the shaped Indic scripts of the Indic
language pack: Devanagari, Bengali and Telugu.

Both are shaped on the device by HarfBuzz (components/harfbuzz) with
the layout tables of the script's Noto Sans font; this script writes
what that needs and the glyph bitmaps the shaped text is drawn with
(<s> = deva / beng / telu):

  <s>/<s>_layout.ttf        the font subset to the script's repertoire
                            (and the glyphs its GSUB reaches), without
                            outlines or hinting: cmap, GDEF, GSUB, GPOS,
                            head, hhea, hmtx, maxp; embedded for HarfBuzz
  <s>/<s>_<set>_<NN>.bin    1 bpp glyph bitmaps per font slot, by glyph
                            id of that subset (set = lowres / hires)
  <s>/<s>_tables.h          per-glyph advances, the nominal glyph and
                            cell count per code point

Usage (needs uharfbuzz, freetype-py and fontTools; see deva/LICENSE.txt,
beng/LICENSE.txt and telu/LICENSE.txt for the font releases used):
    gen_indic_font.py deva NotoSansDevanagari-Regular.ttf fonts_dir out_dir
    gen_indic_font.py beng NotoSansBengali-Regular.ttf fonts_dir out_dir
    gen_indic_font.py telu NotoSansTelugu-Regular.ttf fonts_dir out_dir

fonts_dir holds lowres_NN.c / hires_NN.c, whose line height (plus the
tall-line extra of the Indic pack, gen_tamil_font.line_extra()),
baseline and cell width each slot copies.

Blob format (little endian):
  magic ("DDEV" / "DBEN" / "DTEL"), u8 version (1), u8 line_height, u8 base_line,
  u8 cell_w, u8 pixel size, u8 pad, u16 glyph count, then one 8-byte
  entry per glyph id: u32 bitmap offset (from the start of the bitmap
  area), u8 box_w, u8 box_h, i8 ofs_x, i8 ofs_y (LVGL glyph metrics:
  left and bottom edge of the box relative to the pen and the
  baseline); then the bitmaps, 1 bpp, rows packed MSB-first,
  ceil(w*h/8) bytes.
"""

import io
import math
import os
import struct
import sys

import freetype
import uharfbuzz as hb
from fontTools import subset
from fontTools.ttLib import TTFont

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gen_tamil_font as gt  # noqa: E402  (shared slot / tall-line helpers)

DANDAS = [0x0964, 0x0965]


class Script:
    """What differs between the scripts: the repertoire and the sample
    syllables the pixel size of each slot is fitted to."""

    def __init__(self, name, magic, block, unicodes, consonants, matras,
                 modifiers, below, virama, ra, extra):
        self.name = name              # file prefix, upper case = C prefix
        self.magic = magic
        self.block = block            # first code point of the block
        self.unicodes = unicodes
        self.consonants = consonants
        self.matras = matras
        self.modifiers = modifiers
        self.below = below
        self.virama = virama
        self.ra = ra
        self.extra = extra            # vowels, digits: alone

    def sample_texts(self):
        """Syllables the size is fitted to: every consonant with every
        vowel sign and modifier, with reph, and every two-consonant
        conjunct with the below-base vowel signs."""
        out = []
        for c in self.consonants:
            for m in self.matras:
                for mod in self.modifiers:
                    out.append(chr(c) + chr(m) + mod)
                    out.append(self.ra + self.virama + chr(c) + chr(m) + mod)
        for c1 in self.consonants:
            for c2 in self.consonants:
                for b in self.below:
                    out.append(chr(c1) + self.virama + chr(c2) + b)
        out += [chr(cp) for cp in self.extra]
        return out


# Devanagari: the Devanagari and Devanagari Extended blocks, the
# joiners, the dotted circle HarfBuzz inserts before a lone mark, and
# the rupee sign of the InScript layouts. The rare vocalic rr / l / ll
# signs (U+0944, U+0962, U+0963) are left out of the fitting sample and
# may stick out of the line a little.
DEVA = Script(
    "deva", b"DDEV", 0x0900,
    list(range(0x0900, 0x0980)) + list(range(0xA8E0, 0xA900)) +
    [0x200C, 0x200D, 0x25CC, 0x20B9],
    list(range(0x0915, 0x093A)),
    [cp for cp in range(0x093E, 0x094D) if cp != 0x0944],
    ["", "\u0902", "\u0901"],                    # none, anusvara, candrabindu
    ["", "\u0941", "\u0942", "\u0943"],
    "\u094d", "\u0930",
    list(range(0x0904, 0x0915)) + list(range(0x0966, 0x0970)))

# Bengali: the Bengali block (Bengali and Assamese letters), the
# Devanagari dandas Bengali text uses, the joiners, the dotted circle
# and the rupee sign. As for Devanagari, the rare vocalic rr / l / ll
# signs (U+09C4, U+09E2, U+09E3) are left out of the fitting sample.
BENG_CONSONANTS = ([cp for cp in range(0x0995, 0x09BA)
                    if cp not in (0x09A9, 0x09B1, 0x09B3, 0x09B4, 0x09B5)] +
                   [0x09DC, 0x09DD, 0x09DF, 0x09F0, 0x09F1])
BENG = Script(
    "beng", b"DBEN", 0x0980,
    list(range(0x0980, 0x0A00)) + DANDAS + [0x200C, 0x200D, 0x25CC, 0x20B9],
    BENG_CONSONANTS,
    [0x09BE, 0x09BF, 0x09C0, 0x09C1, 0x09C2, 0x09C3, 0x09C7, 0x09C8,
     0x09CB, 0x09CC],
    ["", "\u0982", "\u0981"],                    # none, anusvara, candrabindu
    ["", "\u09c1", "\u09c2", "\u09c3"],
    "\u09cd", "\u09b0",
    [cp for cp in range(0x0985, 0x0995) if cp not in (0x098D, 0x098E, 0x0991, 0x0992)] +
    list(range(0x09E6, 0x09F0)))

# Telugu: the Telugu block, the Devanagari dandas, the joiners, the
# dotted circle and the rupee sign. Conjuncts stack the following
# consonants below the first one (vattu), so the sample has every
# two-consonant conjunct with the vowel signs drawn above it. The rare
# letters (llla, tsa, dza, rrra, the vocalic l / rr / ll signs) are left
# out of the fitting sample.
TELU_CONSONANTS = [cp for cp in range(0x0C15, 0x0C3A) if cp not in (0x0C29, 0x0C34)]
TELU = Script(
    "telu", b"DTEL", 0x0C00,
    list(range(0x0C00, 0x0C80)) + DANDAS + [0x200C, 0x200D, 0x25CC, 0x20B9],
    TELU_CONSONANTS,
    [0x0C3E, 0x0C3F, 0x0C40, 0x0C41, 0x0C42, 0x0C43, 0x0C46, 0x0C47, 0x0C48,
     0x0C4A, 0x0C4B, 0x0C4C],
    ["", "\u0c02", "\u0c01"],                    # none, anusvara, candrabindu
    ["", "\u0c3e", "\u0c3f", "\u0c47", "\u0c4a"],
    "\u0c4d", "\u0c30",
    [cp for cp in range(0x0C05, 0x0C15) if cp not in (0x0C0D, 0x0C11)] +
    list(range(0x0C66, 0x0C70)))

SCRIPTS = {"deva": DEVA, "beng": BENG, "telu": TELU}

# Tables the device's HarfBuzz needs; outlines and hinting are only
# used here, for the bitmaps.
LAYOUT_TABLES = ("cmap", "GDEF", "GSUB", "GPOS", "head", "hhea", "hmtx", "maxp")

# Share of the sample syllables that may stick out of the line by a
# pixel or two (some deep conjunct stacks), so the common text gets a
# larger size.
OVERFLOW = 0.002


def subset_font(path, unicodes):
    """(full subset TTFont, layout-only bytes)."""
    opts = subset.Options()
    opts.layout_features = ["*"]
    opts.name_IDs = []
    opts.notdef_outline = True
    opts.glyph_names = False
    opts.hinting = True
    font = TTFont(path, recalcTimestamp=False)
    sub = subset.Subsetter(opts)
    sub.populate(unicodes=unicodes)
    sub.subset(font)
    full = io.BytesIO()
    font.save(full)
    layout = TTFont(io.BytesIO(full.getvalue()), recalcTimestamp=False)
    for tag in list(layout.keys()):
        if tag != "GlyphOrder" and tag not in LAYOUT_TABLES:
            del layout[tag]
    lay = io.BytesIO()
    layout.save(lay)
    return full.getvalue(), lay.getvalue(), font


class GlyphRenderer:
    """Renders single glyphs by id (FreeType) and shapes text
    (HarfBuzz, design units) from the subset font."""

    def __init__(self, data):
        self.ft = freetype.Face(io.BytesIO(data))
        self.hbfont = hb.Font(hb.Face(hb.Blob(data)))
        self.upem = self.ft.units_per_EM
        self.flags = freetype.FT_LOAD_TARGET_MONO
        self.px = None

    def set(self, px, autohint):
        self.px = px
        self.ft.set_pixel_sizes(0, px)
        self.flags = freetype.FT_LOAD_TARGET_MONO
        if autohint:
            self.flags |= freetype.FT_LOAD_FORCE_AUTOHINT

    def glyph(self, gid):
        """(rows, left, top) of one glyph at the current size."""
        self.ft.load_glyph(gid, self.flags | freetype.FT_LOAD_RENDER)
        g = self.ft.glyph
        bm = g.bitmap
        buf = bytes(bm.buffer)
        rows = []
        for y in range(bm.rows):
            line = buf[y * bm.pitch:(y + 1) * bm.pitch]
            bits = bin(int.from_bytes(line, "big") | (1 << (8 * len(line))))[3:] if line else ""
            rows.append([1 if c == "1" else 0 for c in bits[:bm.width]])
        rows, left, top = gt.trim(rows, g.bitmap_left, g.bitmap_top)
        return rows, left, top

    def shape(self, text):
        buf = hb.Buffer()
        buf.add_str(text)
        buf.guess_segment_properties()
        hb.shape(self.hbfont, buf, {})
        return [(i.codepoint, p.x_offset, p.y_offset)
                for i, p in zip(buf.glyph_infos, buf.glyph_positions)]


def fit_size(r, shaped, lh, autohint):
    """Largest pixel size at which all but OVERFLOW of the sample
    syllables fit lh, with the best baseline for it:
    (px, ascent, descent) with ascent + descent = lh."""
    best = None
    for px in range(max(4, lh // 2), 2 * lh):
        r.set(px, autohint)
        cache = {}
        scale = px / r.upem
        ext = []
        for run in shaped:
            a = d = 0
            for gid, dx, dy in run:
                if gid not in cache:
                    cache[gid] = r.glyph(gid)
                rows, _, top = cache[gid]
                if not rows:
                    continue
                oy = int(round(dy * scale))
                a = max(a, top + oy)
                d = max(d, len(rows) - top - oy)
            ext.append((a, d))
        allowed = int(len(ext) * OVERFLOW)
        for asc in range(lh + 1):
            bad = sum(1 for a, d in ext if a > asc or d > lh - asc)
            if bad <= allowed:
                best = (px, asc, lh - asc)
                break
    return best


def main():
    if len(sys.argv) != 5 or sys.argv[1] not in SCRIPTS:
        sys.exit(__doc__)
    sc = SCRIPTS[sys.argv[1]]
    ttf, fonts_dir, out_dir = sys.argv[2:]
    name, NAME, blk = sc.name, sc.name.upper(), sc.block
    os.makedirs(out_dir, exist_ok=True)
    full, layout, font = subset_font(ttf, sc.unicodes)
    with open(os.path.join(out_dir, f"{name}_layout.ttf"), "wb") as f:
        f.write(layout)

    order = font.getGlyphOrder()
    ngl = len(order)
    hmtx = font["hmtx"].metrics
    adv = [hmtx[n][0] for n in order]
    cmap = font.getBestCmap()

    r = GlyphRenderer(full)
    shaped = [r.shape(t) for t in sc.sample_texts()]

    cells = {}
    for family in ("lowres", "hires"):
        for slot, lh, bl, cw in gt.slot_metrics(fonts_dir, family):
            extra, below = gt.line_extra(lh)
            lh, bl = lh + extra, bl + below
            autohint = lh >= gt.AUTOHINT_LINE
            px, asc, desc = fit_size(r, shaped, lh, autohint)
            r.set(px, autohint)
            shift = max(0, desc - bl)
            scale = px / r.upem
            for cp, gname in cmap.items():
                if blk <= cp < blk + 0x80:
                    gid = order.index(gname)
                    n = math.ceil(adv[gid] * scale / cw) if adv[gid] else 0
                    cells[cp] = max(cells.get(cp, 0), n)
            entries = bytearray()
            bitmaps = bytearray()
            for gid in range(ngl):
                rows, left, top = r.glyph(gid)
                if not rows:
                    entries += struct.pack("<IBBbb", len(bitmaps), 0, 0, 0, 0)
                    continue
                h, w = len(rows), len(rows[0])
                assert w < 256 and h < 256 and -128 <= left < 128, (gid, w, h, left)
                bits = [v for row in rows for v in row]
                packed = bytearray((len(bits) + 7) // 8)
                for i, v in enumerate(bits):
                    if v:
                        packed[i >> 3] |= 0x80 >> (i & 7)
                entries += struct.pack("<IBBbb", len(bitmaps), w, h, left,
                                       top - h + shift)
                bitmaps += packed
            out = bytearray(sc.magic)
            out += struct.pack("<BBBBBBH", 1, lh, bl, cw, px, 0, ngl)
            out += entries + bitmaps
            with open(os.path.join(out_dir, f"{name}_{family}_{slot}.bin"), "wb") as f:
                f.write(out)
            print(f"{family}_{slot}: line {lh}, cell {cw}, {name} {px} px, "
                  f"asc {asc} desc {desc} shift {shift}, {len(out)} bytes")

    with open(os.path.join(out_dir, f"{name}_tables.h"), "w", encoding="ascii") as f:
        f.write("/* Generated by tools/gen_indic_font.py -- do not edit. */\n"
                "#pragma once\n#include <stdint.h>\n\n")
        f.write(f"#define {NAME}_UPEM {r.upem}\n#define {NAME}_GLYPHS {ngl}\n\n")
        f.write("/* Advance width of each glyph id, font units. */\n"
                f"static const uint16_t {NAME}_ADVANCE[{NAME}_GLYPHS] = {{\n")
        for i in range(0, ngl, 12):
            f.write("    " + ", ".join(str(a) for a in adv[i:i + 12]) + ",\n")
        f.write(f"}};\n\n/* Nominal glyph id of U+{blk:04X} + i (0 = none): the glyphs\n"
                " * drawn outside the editor, unshaped. */\n"
                f"static const uint16_t {NAME}_CMAP[128] = {{\n")
        nom = [order.index(cmap[blk + i]) if (blk + i) in cmap else 0
               for i in range(128)]
        for i in range(0, 128, 16):
            f.write("    " + ", ".join(str(c) for c in nom[i:i + 16]) + ",\n")
        f.write(f"}};\n\n/* Most monospace cells the nominal glyph of U+{blk:04X} + i takes\n"
                " * in any slot (an upper bound for width estimates). */\n"
                f"static const uint8_t {NAME}_CELLS[128] = {{\n")
        row = [cells.get(blk + i, 0) for i in range(128)]
        for i in range(0, 128, 16):
            f.write("    " + ", ".join(str(c) for c in row[i:i + 16]) + ",\n")
        f.write("};\n")
        if blk != 0x0900:
            dg = [order.index(cmap[d]) if d in cmap else 0 for d in DANDAS]
            f.write("\n/* Nominal glyph ids of the danda and double danda (U+0964,\n"
                    " * U+0965), for builds without Devanagari. */\n"
                    f"static const uint16_t {NAME}_DANDA[2] = {{ {dg[0]}, {dg[1]} }};\n")


if __name__ == "__main__":
    main()
