#!/usr/bin/env python3
"""Generate the CJK pixel-font blobs used by components/fonts/cjk_font.c.

Sources (all SIL OFL 1.1; Unifont is dual OFL / GPLv2+ with the font
embedding exception and is used here under the OFL):

  * Fusion Pixel Font, 10 px and 12 px, monospaced, BDF release
    (github.com/TakWolf/fusion-pixel-font), one BDF per regional style:
    fusion-pixel-<N>px-monospaced-{zh_hans,ja,ko}.bdf
  * GNU Unifont 16 px, BDF release (unifoundry.com / ftp.gnu.org/gnu/unifont):
    unifont-<ver>.bdf and unifont_jp-<ver>.bdf (Japanese Han shapes)

Each language pack gets its own set (cjk/<set>/cjk10.bin, cjk12.bin,
cjk16.bin) with that language's glyph style and only the characters it
needs:

  ja  kana, the kanji of JIS X 0208 (as encodable in EUC-JP) and every
      character of the Japanese conversion dictionary
  zh  bopomofo, the hanzi of GB 2312 and every character of the pinyin
      dictionary
  ko  all 11,172 Hangul syllables and the compatibility jamo

plus, for all three, CJK symbols and punctuation and the fullwidth
forms. Only full-width glyphs are taken; Latin text keeps using
Greybeard / Hack. Every glyph is stored as a full N x N cell, 1 bit per
pixel, rows packed MSB-first without per-row padding, so a glyph's
bitmap is found by index alone.

Blob layout (little endian):
    0   char[4]  "DCJK"
    4   u8       format version (1)
    5   u8       N, the cell size in pixels (glyphs are N x N)
    6   u8       ascent: rows of the cell above the baseline
    7   u8       reserved (0)
    8   u32      glyph count
    12  u16[count]  code points, ascending (all glyphs are in the BMP)
    ..  u8[count * ceil(N*N/8)]  bitmaps, same order

Usage:
    gen_cjk_fonts.py <set> <fusion 10px bdf dir> <fusion 12px bdf dir> \
                     <unifont.bdf> <unifont_jp.bdf> <out dir> [<dict.bin>]

<dict.bin> is components/ime/dict/ja.bin or zh.bin (for ja / zh).
"""

import os
import struct
import sys

COMMON = [
    (0x3000, 0x303F),   # CJK symbols and punctuation
    (0xFF01, 0xFF60),   # Fullwidth ASCII variants
    (0xFFE0, 0xFFE6),   # Fullwidth signs
]
SET_RANGES = {
    'ja': [(0x3040, 0x30FF), (0x31F0, 0x31FF)],   # kana
    'zh': [(0x3100, 0x312F)],                     # bopomofo
    'ko': [(0x3130, 0x318F), (0xAC00, 0xD7A3)],   # jamo, syllables
}
SET_CODEC = {'ja': 'euc_jp', 'zh': 'gb2312'}
STYLE = {'ja': 'ja', 'zh': 'zh_hans', 'ko': 'ko'}
HAN = [(0x3400, 0x4DBF), (0x4E00, 0x9FFF), (0xF900, 0xFAFF)]


def repertoire(set_name, dict_path):
    cps = set()
    for a, b in COMMON + SET_RANGES[set_name]:
        cps.update(range(a, b + 1))
    codec = SET_CODEC.get(set_name)
    if codec:
        for a, b in HAN:
            for cp in range(a, b + 1):
                try:
                    chr(cp).encode(codec)
                except UnicodeEncodeError:
                    continue
                cps.add(cp)
    if dict_path:
        data = open(dict_path, 'rb').read()
        count = struct.unpack_from('<I', data, 8)[0]
        text = data[12 + 4 * (count + 1):].decode('utf-8', 'replace')
        cps.update(ord(c) for c in text if ord(c) >= 0x2E80 and ord(c) <= 0xFFFF)
    return cps


def load_bdf(path, wanted):
    """Return (cell N, ascent, {cp: N*N bit list}) for full-width glyphs."""
    glyphs = {}
    ascent = descent = None
    pixel = None
    cur = None
    with open(path, encoding='latin-1') as f:
        lines = iter(f)
        for line in lines:
            if line.startswith('FONT_ASCENT'):
                ascent = int(line.split()[1])
            elif line.startswith('FONT_DESCENT'):
                descent = int(line.split()[1])
            elif line.startswith('STARTCHAR'):
                cur = {'cp': None}
            elif line.startswith('ENCODING') and cur is not None:
                cur['cp'] = int(line.split()[1])
            elif line.startswith('DWIDTH') and cur is not None:
                cur['dw'] = int(line.split()[1])
            elif line.startswith('BBX') and cur is not None:
                cur['bbx'] = [int(x) for x in line.split()[1:5]]
            elif line.startswith('BITMAP') and cur is not None:
                rows = []
                for bl in lines:
                    if bl.startswith('ENDCHAR'):
                        break
                    rows.append(bl.strip())
                cur['rows'] = rows
                glyphs[cur['cp']] = cur
                cur = None
    pixel = ascent + descent
    out = {}
    for cp, g in glyphs.items():
        if cp is None or cp < 0 or cp > 0xFFFF or cp not in wanted:
            continue
        if g.get('dw') != pixel:
            continue  # not a full-width glyph
        w, h, xo, yo = g['bbx']
        cell = [0] * (pixel * pixel)
        for r, hexrow in enumerate(g['rows']):
            bits = int(hexrow, 16) if hexrow else 0
            nbits = len(hexrow) * 4
            y = yo + h - 1 - r              # row above the baseline
            row = ascent - 1 - y            # cell row, 0 = top
            if row < 0 or row >= pixel:
                continue
            for c in range(w):
                if bits & (1 << (nbits - 1 - c)):
                    col = xo + c
                    if 0 <= col < pixel:
                        cell[row * pixel + col] = 1
        out[cp] = cell
    return pixel, ascent, out


def pack(cell):
    b = bytearray((len(cell) + 7) // 8)
    for i, v in enumerate(cell):
        if v:
            b[i >> 3] |= 0x80 >> (i & 7)
    return bytes(b)


def write_blob(path, n, ascent, glyphs):
    cps = sorted(glyphs)
    with open(path, 'wb') as f:
        f.write(b'DCJK')
        f.write(struct.pack('<BBBBI', 1, n, ascent, 0, len(cps)))
        f.write(struct.pack('<%dH' % len(cps), *cps))
        for cp in cps:
            f.write(pack(glyphs[cp]))
    print('%s: %d glyphs, %d bytes' % (path, len(cps), os.path.getsize(path)))


def main():
    if len(sys.argv) not in (7, 8) or sys.argv[1] not in STYLE:
        sys.exit(__doc__)
    set_name, f10, f12, uni, uni_jp, out = sys.argv[1:7]
    dict_path = sys.argv[7] if len(sys.argv) == 8 else None
    wanted = repertoire(set_name, dict_path)
    out = os.path.join(out, set_name)
    os.makedirs(out, exist_ok=True)
    style = STYLE[set_name]
    for n, d in ((10, f10), (12, f12)):
        bdf = os.path.join(d, 'fusion-pixel-%dpx-monospaced-%s.bdf' % (n, style))
        px, asc, g = load_bdf(bdf, wanted)
        assert px == n, (px, n)
        write_blob(os.path.join(out, 'cjk%d.bin' % n), n, asc, g)
    px, asc, g = load_bdf(uni_jp if set_name == 'ja' else uni, wanted)
    assert px == 16, px
    write_blob(os.path.join(out, 'cjk16.bin'), 16, asc, g)


if __name__ == '__main__':
    main()
