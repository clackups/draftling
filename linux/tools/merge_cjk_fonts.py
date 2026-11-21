#!/usr/bin/env python3
"""Merge the ja, zh and ko CJK font sets into the "all" set.

Usage: merge_cjk_fonts.py <components/fonts/cjk dir> <out dir>

The "all" language pack (Linux only) has the Japanese, Chinese and
Korean input methods at once, but components/fonts/cjk_font.c embeds
one set (cjk10.bin, cjk12.bin, cjk16.bin). This writes that set from
the three committed ones, size by size: every code point of any set,
taken from the first set that has it in this order --

  kana (U+3040..U+30FF, U+31F0..U+31FF)   ja
  bopomofo (U+3100..U+312F)                zh
  Hangul (U+3130..U+318F, U+AC00..U+D7A3)  ko
  everything else (Han, punctuation)       ja, then zh, then ko

so Han characters keep the Japanese glyph style where the Japanese set
has them (JIS X 0208) and the Chinese one otherwise. The sets share the
"DCJK" format (see components/fonts/tools/gen_cjk_fonts.py) and, per
size, the cell size and ascent; the script checks that.
"""
import os
import struct
import sys

SIZES = (10, 12, 16)


def load(path):
    d = open(path, "rb").read()
    if d[:4] != b"DCJK" or d[4] != 1:
        sys.exit("%s: not a DCJK v1 blob" % path)
    n_cell, ascent = d[5], d[6]
    count = struct.unpack_from("<I", d, 8)[0]
    cps = struct.unpack_from("<%dH" % count, d, 12)
    gsz = (n_cell * n_cell + 7) // 8
    base = 12 + 2 * count
    glyphs = {cp: d[base + i * gsz: base + (i + 1) * gsz] for i, cp in enumerate(cps)}
    return n_cell, ascent, glyphs


def order(cp):
    if 0x3040 <= cp <= 0x30FF or 0x31F0 <= cp <= 0x31FF:
        return ("ja", "zh", "ko")
    if 0x3100 <= cp <= 0x312F:
        return ("zh", "ja", "ko")
    if 0x3130 <= cp <= 0x318F or 0xAC00 <= cp <= 0xD7A3:
        return ("ko", "ja", "zh")
    return ("ja", "zh", "ko")


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    src, out = sys.argv[1:]
    os.makedirs(out, exist_ok=True)
    for size in SIZES:
        sets = {s: load(os.path.join(src, s, "cjk%d.bin" % size)) for s in ("ja", "zh", "ko")}
        cells = {v[0] for v in sets.values()}
        ascents = {v[1] for v in sets.values()}
        if len(cells) != 1 or len(ascents) != 1:
            sys.exit("cjk%d.bin: the sets differ in cell size or ascent" % size)
        n_cell, ascent = cells.pop(), ascents.pop()
        all_cps = sorted(set().union(*(v[2].keys() for v in sets.values())))
        bitmaps = []
        for cp in all_cps:
            for s in order(cp):
                g = sets[s][2].get(cp)
                if g is not None:
                    bitmaps.append(g)
                    break
        blob = b"DCJK" + bytes([1, n_cell, ascent, 0]) + struct.pack("<I", len(all_cps))
        blob += struct.pack("<%dH" % len(all_cps), *all_cps) + b"".join(bitmaps)
        path = os.path.join(out, "cjk%d.bin" % size)
        tmp = "%s.tmp%d" % (path, os.getpid())
        with open(tmp, "wb") as f:
            f.write(blob)
        os.replace(tmp, path)


if __name__ == "__main__":
    main()
