# HarfBuzz (vendored)

The OpenType text shaper used by the Indic language pack to shape
Devanagari on the device: conjuncts, half forms, reph, the pre-base
i-matra and mark positioning come from the font's GSUB / GPOS tables
(see `components/fonts/deva_font.c`).

- Upstream: https://github.com/harfbuzz/harfbuzz, release 15.0.0
  (`harfbuzz-15.0.0.tar.xz`, SHA-256
  `968536e9abd782f07f7954044c0c4020199fcdd39b41d221ec42d2a3be4a44df`).
- License: the "Old MIT" license in `COPYING` (some files carry their
  own notices, kept unchanged).
- Built from the amalgamation `src/harfbuzz.cc` with `HB_TINY` and
  `HB_NO_MT` (`CMakeLists.txt`): OpenType layout and the complex
  shapers, no variable / colour / bitmap fonts, AAT, drawing or
  subsetting. `config/hb_draftling_config.h` (outside `src/`) adjusts
  that for ESP-IDF.

`src/` holds only the files that build needs, unmodified. To update,
unpack the new release and copy the files the compiler reports:

```bash
cd harfbuzz-X.Y.Z/src
g++ -std=c++11 -DHB_TINY -DHB_NO_MT -M harfbuzz.cc | tr ' \\' '\n\n' |
  grep -v '^$\|^/usr\|harfbuzz.o:' | xargs -n1 realpath --relative-to=. |
  sort -u > /tmp/hb_files.txt
while read f; do mkdir -p "$DEST/src/$(dirname "$f")"; cp "$f" "$DEST/src/$f"; done < /tmp/hb_files.txt
```

with `DEST` this directory (remove the old `src/` first), and update the
version above.
