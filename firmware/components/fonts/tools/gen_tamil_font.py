#!/usr/bin/env python3
"""Generate the Tamil cluster-glyph blobs of the Tamil language pack.

LVGL draws one code point at a time and does no shaping, but Tamil
needs it: a vowel sign after a consonant is drawn before it (U+0BC6..U+0BC8),
around it (U+0BCA..U+0BCC) or fused with it (ka + u, Ta + i, ...). LVGL does pass the
next code point to a font's get_glyph_dsc(), so the runtime font
(tamil_font.c) draws a consonant together with the vowel sign that
follows it as one pre-shaped glyph, and the sign itself takes no
space. This script shapes every such cluster with HarfBuzz, renders it
1 bpp with FreeType (the font's TrueType hints up to 13 px, the
auto-hinter above, see AUTOHINT_LINE) and writes one blob per font
slot.

The glyphs keep the font's own advance widths (proportional, like the
Tamil letters of any text editor), so the syllables of a word join up
the way they are written. For code that measures text in monospace
cells without a font (status bar, title), tamil_cells.h has the most
cells each glyph takes in any slot.

Usage (needs uharfbuzz and freetype-py; see tamil/LICENSE.txt for the
font release used):
    gen_tamil_font.py NotoSansTamil-Regular.ttf fonts_dir out_dir

fonts_dir holds lowres_NN.c / hires_NN.c, whose line height
(plus the Indic pack's extra, line_extra()), baseline and cell width
each slot copies. Writes
out_dir/tamil_<family>_<NN>.bin and out_dir/tamil_cells.h.

Blob format (little endian):
  "DTAM", u8 version (2), u8 line_height, u8 base_line, u8 cell_w,
  u32 count, then count entries of 16 bytes sorted by key:
      u32 key (base << 16 | mark, mark 0 = none), u32 bitmap offset
      (from the start of the bitmap area), u8 box_w, u8 box_h,
      i8 ofs_x, i8 ofs_y, u8 adv_w (LVGL glyph metrics, pixels),
      3 bytes pad;
  then the bitmaps, 1 bpp, rows packed MSB-first, ceil(w*h/8) bytes.
"""

import math
import os
import re
import struct
import sys

import freetype
import uharfbuzz as hb

DOTTED_CIRCLE = 0x25CC

# Consonants: the 18 Tamil ones and the Grantha ja, sha, ssa, sa, ha.
CONSONANTS = [0x0B95, 0x0B99, 0x0B9A, 0x0B9E, 0x0B9F, 0x0BA3, 0x0BA4,
              0x0BA8, 0x0BAA, 0x0BAE, 0x0BAF, 0x0BB0, 0x0BB1, 0x0BB2,
              0x0BB3, 0x0BB4, 0x0BB5, 0x0BA9,
              0x0B9C, 0x0BB6, 0x0BB7, 0x0BB8, 0x0BB9]
# Marks that join the preceding consonant: vowel signs, the pulli
# (virama), the au length mark and the anusvara.
MARKS = [0x0BBE, 0x0BBF, 0x0BC0, 0x0BC1, 0x0BC2, 0x0BC6, 0x0BC7, 0x0BC8,
         0x0BCA, 0x0BCB, 0x0BCC, 0x0BCD, 0x0BD7, 0x0B82]
# Ligatures of several code points, which the editor shows as one
# private-use code point (see tamil_font.h): k-ssa (k, pulli, ssa),
# which takes vowel signs, and sri (sa or sha, pulli, ra, ii).
LIG_KSSA = 0xE001
LIG_SRI = 0xE002
LIGATURES = {
    LIG_KSSA: "\u0b95\u0bcd\u0bb7",
    LIG_SRI: "\u0bb8\u0bcd\u0bb0\u0bc0",
}
# Characters drawn on their own.
STANDALONE = ([0x0B83, 0x0B85, 0x0B86, 0x0B87, 0x0B88, 0x0B89, 0x0B8A,
               0x0B8E, 0x0B8F, 0x0B90, 0x0B92, 0x0B93, 0x0B94, 0x0BD0] +
              list(range(0x0BE6, 0x0BFB)))

# Slots (line heights, with the Indic pack's extra) rendered with
# FreeType's auto-hinter. The font's own TrueType hints give clean
# single-pixel strokes at the small sizes but a mix of one- and
# two-pixel strokes from about 13 px up; the auto-hinter keeps the
# strokes even there.
AUTOHINT_LINE = 17

# Low-res (Greybeard) slots whose Latin letters have two-pixel stems (22 px and
# the heading sizes above it): the Tamil outlines are widened
# horizontally by EMBOLDEN_PX before rendering, so their strokes come
# out about as heavy. (0.6 px and more fill the small loops.)
EMBOLDEN_FAMILY = "lowres"
EMBOLDEN_LINE = 21
EMBOLDEN_PX = 0.4


def line_extra(lh):
    """Extra line height of the Indic pack over a base slot of line
    height lh, and the part of it below the baseline. Same formulas as
    TALL_LINE_EXTRA / TALL_BASE_EXTRA in include/tall_lines.h."""
    extra = (lh * 3 + 5) // 10
    return extra, extra // 2


def slot_metrics(fonts_dir, family):
    """(slot, line_height, base_line, cell_w) of every <family>_NN.c."""
    out = []
    for name in sorted(os.listdir(fonts_dir)):
        m = re.fullmatch(family + r"_(\d+)\.c", name)
        if not m:
            continue
        src = open(os.path.join(fonts_dir, name), encoding="utf-8").read()
        lh = int(re.search(r"\.line_height = (\d+)", src).group(1))
        bl = int(re.search(r"\.base_line = (\d+)", src).group(1))
        adv = int(re.search(r"\.adv_w = ([1-9]\d*)", src).group(1))
        out.append((int(m.group(1)), lh, bl, adv // 16))
    return sorted(out)


def clusters():
    """(key, text) of every glyph in the blob."""
    out = [((cp << 16), chr(cp)) for cp in STANDALONE]
    out.append((LIG_SRI << 16, LIGATURES[LIG_SRI]))
    bases = [(cp, chr(cp)) for cp in CONSONANTS + [DOTTED_CIRCLE]]
    bases.append((LIG_KSSA, LIGATURES[LIG_KSSA]))
    for base, text in bases:
        out.append((base << 16, text))
        for mark in MARKS:
            out.append(((base << 16) | mark, text + chr(mark)))
    return sorted(out)


class Renderer:
    def __init__(self, path):
        self.blob = hb.Blob.from_file_path(path)
        self.face = hb.Face(self.blob)
        self.font = hb.Font(self.face)
        self.upem = self.face.upem
        self.ft = freetype.Face(path)
        self.size = None
        self.flags = freetype.FT_LOAD_TARGET_MONO
        self.embolden = 0.0     # extra stroke width in pixels

    def set_autohint(self, on):
        """FreeType's auto-hinter instead of the font's TrueType hints."""
        self.flags = freetype.FT_LOAD_TARGET_MONO
        if on:
            self.flags |= freetype.FT_LOAD_FORCE_AUTOHINT

    def set_size(self, px):
        self.size = px
        self.ft.set_pixel_sizes(0, px)
        self.font.scale = (self.upem, self.upem)

    def bounds(self, text):
        """(ascent, descent) of the shaped text's ink in pixels."""
        buf = hb.Buffer()
        buf.add_str(text)
        buf.guess_segment_properties()
        hb.shape(self.font, buf, {})
        scale = self.size / self.upem
        asc = desc = 0
        for info, pos in zip(buf.glyph_infos, buf.glyph_positions):
            self.ft.load_glyph(info.codepoint, self.flags)
            g = self.ft.glyph
            m = g.metrics
            if m.width == 0 or m.height == 0:
                continue
            oy = int(round(pos.y_offset * scale))
            top = (m.horiBearingY + 63) // 64 + oy
            bot = (m.horiBearingY - m.height) // 64 + oy
            asc = max(asc, top)
            desc = max(desc, -bot)
        return asc, desc

    def render(self, text):
        """Shape and render text. Returns (rows, x0, top, width_px):
        rows = list of 0/1 lists, x0 / top = left / top edge of the
        bitmap relative to the pen origin / baseline (top > 0 = above),
        width_px = advance width in pixels."""
        buf = hb.Buffer()
        buf.add_str(text)
        buf.guess_segment_properties()
        hb.shape(self.font, buf, {})
        scale = self.size / self.upem
        pen = 0.0
        pieces = []
        for info, pos in zip(buf.glyph_infos, buf.glyph_positions):
            self.ft.load_glyph(info.codepoint, self.flags)
            g = self.ft.glyph
            if self.embolden:
                freetype.FT_Outline_EmboldenXY(
                    freetype.byref(g._FT_GlyphSlot.contents.outline),
                    int(round(self.embolden * 64)), 0)
            g.render(freetype.FT_RENDER_MODE_MONO)
            bm = g.bitmap
            ox = int(round(pen + pos.x_offset * scale))
            oy = int(round(pos.y_offset * scale))
            buf8 = bytes(bm.buffer)
            rows = []
            for y in range(bm.rows):
                line = buf8[y * bm.pitch:(y + 1) * bm.pitch]
                bits = bin(int.from_bytes(line, "big") | (1 << (8 * len(line))))[3:] if line else ""
                rows.append([1 if c == "1" else 0 for c in bits[:bm.width]])
            pieces.append((ox + g.bitmap_left, oy + g.bitmap_top, rows))
            pen += pos.x_advance * scale
        width = pen
        pieces = [p for p in pieces if p[2] and p[2][0]]
        if not pieces:
            return [], 0, 0, width
        x0 = min(p[0] for p in pieces)
        x1 = max(p[0] + len(p[2][0]) for p in pieces)
        top = max(p[1] for p in pieces)
        bot = min(p[1] - len(p[2]) for p in pieces)
        w, h = x1 - x0, top - bot
        canvas = [[0] * w for _ in range(h)]
        for (px, py, rows) in pieces:
            for y, row in enumerate(rows):
                for x, v in enumerate(row):
                    if v:
                        canvas[top - py + y][px - x0 + x] = 1
        return canvas, x0, top, width


def trim(rows, x0, top):
    """Drop blank border rows / columns."""
    if not rows:
        return rows, x0, top
    while rows and not any(rows[0]):
        rows = rows[1:]
        top -= 1
    while rows and not any(rows[-1]):
        rows = rows[:-1]
    if not rows:
        return [], 0, 0
    w = len(rows[0])
    left = min(r.index(1) for r in rows if any(r))
    right = max(w - r[::-1].index(1) for r in rows if any(r))
    rows = [r[left:right] for r in rows]
    return rows, x0 + left, top


def pick_size(r, items, lh):
    """Largest pixel size whose clusters fit the line height."""
    def measure(px):
        r.set_size(px)
        asc = desc = 0
        for _, text in items:
            a, d = r.bounds(text)
            asc, desc = max(asc, a), max(desc, d)
        return asc, desc

    best = None
    for px in range(max(4, lh // 2), 4 * lh):
        asc, desc = measure(px)
        if asc + desc > lh:
            break
        best = (px, asc, desc)
    # The hinted outline's bounds are an estimate: check the rendered
    # ink and step down while it does not fit.
    while True:
        px = best[0]
        r.set_size(px)
        asc = desc = 0
        for _, text in items:
            rows, _, top, _ = r.render(text)
            rows, _, top = trim(rows, 0, top)
            if rows:
                asc = max(asc, top)
                desc = max(desc, len(rows) - top)
        if asc + desc <= lh or px <= 4:
            return (px, asc, desc)
        best = (px - 1, 0, 0)


def main():
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    ttf, fonts_dir, out_dir = sys.argv[1:]
    os.makedirs(out_dir, exist_ok=True)
    r = Renderer(ttf)
    items = clusters()

    plans = []   # (family, slot, lh, bl, cw, px, shift, glyphs)
    cells = {}   # most monospace cells of each glyph in any slot
    # The two font sets: lowres_NN (Greybeard typeface) and hires_NN
    # (Hack typeface).
    for family in ("lowres", "hires"):
        for slot, lh, bl, cw in slot_metrics(fonts_dir, family):
            # Emboldening follows the Latin letters of the slot.
            bold = family == EMBOLDEN_FAMILY and lh >= EMBOLDEN_LINE
            r.embolden = EMBOLDEN_PX if bold else 0.0
            # The Indic pack's fonts have taller lines (see line_extra()),
            # which the Tamil letters fill.
            extra, below = line_extra(lh)
            lh, bl = lh + extra, bl + below
            r.set_autohint(lh >= AUTOHINT_LINE)
            px, asc, desc = pick_size(r, items, lh)
            r.set_size(px)
            glyphs = {}
            for key, text in items:
                rows, x0, top, adv = r.render(text)
                rows, x0, top = trim(rows, x0, top)
                w = len(rows[0]) if rows else 0
                # The font's advance, but at least one blank column
                # right of the (hinted, maybe emboldened) ink.
                adv = int(round(adv))
                if rows:
                    adv = max(adv, x0 + w + 1)
                assert 0 < adv < 256, (key, adv)
                cells[key] = max(cells.get(key, 1), math.ceil(adv / cw))
                glyphs[key] = (rows, x0, top, adv)
            # Tamil baseline: on the Latin one when the clusters fit
            # there, else raised just enough to keep them in the line.
            shift = max(0, desc - bl)
            plans.append((family, slot, lh, bl, cw, px, shift, glyphs))
            print(f"{family}_{slot}: line {lh}, cell {cw}, tamil {px} px, "
                  f"asc {asc} desc {desc} shift {shift}")

    for family, slot, lh, bl, cw, px, shift, glyphs in plans:
        entries = []
        bitmaps = bytearray()
        for key, _ in items:
            rows, x0, top, adv = glyphs[key]
            if not rows:
                entries.append(struct.pack("<IIBBbbB3x", key, len(bitmaps),
                                           0, 0, 0, 0, adv))
                continue
            h = len(rows)
            w = len(rows[0])
            # LVGL ofs_y: bottom of the box above the baseline.
            ofs_y = top - h + shift
            bits = [v for row in rows for v in row]
            packed = bytearray((len(bits) + 7) // 8)
            for i, v in enumerate(bits):
                if v:
                    packed[i >> 3] |= 0x80 >> (i & 7)
            entries.append(struct.pack("<IIBBbbB3x", key, len(bitmaps),
                                       w, h, x0, ofs_y, adv))
            bitmaps += packed
        out = bytearray(b"DTAM")
        out += struct.pack("<BBBBI", 2, lh, bl, cw, len(entries))
        for e in entries:
            out += e
        out += bitmaps
        path = os.path.join(out_dir, f"tamil_{family}_{slot}.bin")
        with open(path, "wb") as f:
            f.write(out)

    # Cell counts for text measured without a font (status bar, title).
    with open(os.path.join(out_dir, "tamil_cells.h"), "w", encoding="ascii") as f:
        f.write("/* Generated by tools/gen_tamil_font.py -- do not edit.\n"
                " * Most monospace cells each Tamil cluster glyph takes in any\n"
                " * font size (an upper bound for width estimates): key =\n"
                " * base << 16 | mark (mark 0 = alone), sorted. */\n")
        f.write("#pragma once\n#include <stdint.h>\n\n")
        f.write("static const uint32_t TAMIL_CELL_KEYS[] = {\n")
        keys = [k for k, _ in items]
        for i in range(0, len(keys), 6):
            f.write("    " + ", ".join(f"0x{k:08X}" for k in keys[i:i + 6]) + ",\n")
        f.write("};\n\nstatic const uint8_t TAMIL_CELL_COUNTS[] = {\n")
        for i in range(0, len(keys), 16):
            f.write("    " + ", ".join(str(cells[k]) for k in keys[i:i + 16]) + ",\n")
        f.write("};\n")


if __name__ == "__main__":
    main()
