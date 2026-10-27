#!/usr/bin/env python3
"""Generate the conversion dictionaries of the Japanese and Chinese input
methods (components/ime/dict/ja.bin and zh.bin).

Sources:
  * Japanese: the Mozc open-source dictionary
    (github.com/google/mozc, src/data/dictionary_oss/dictionary0*.txt
    and id.def). Mozc is BSD-3-Clause; the dictionary data is derived
    from IPAdic and is distributed under its license (see
    dict/LICENSE.txt).
  * Chinese: the Android Open Source Project PinyinIME raw dictionary
    (packages/inputmethods/PinyinIME, jni/data/rawdict_utf16_65105_freq.txt,
    Apache License 2.0).

Blob layout (little endian):
    0   char[4]  "DIME"
    4   u8       format version (1)
    5   u8[3]    reserved
    8   u32      key count
    12  u32[count + 1]  offsets of the entries, relative to the data area
    ..  data area: per key, in ascending byte order of the keys:
          key bytes, 0
          u16 cost of the best candidate (lower = more frequent)
          u8  candidate count
          candidates, each UTF-8 followed by 0, best first

Keys are hiragana readings (Japanese) or pinyin syllables separated by
single spaces, "v" standing for u-umlaut (Chinese).

Usage:
    gen_ime_dicts.py ja <mozc dictionary_oss dir> <out.bin>
    gen_ime_dicts.py zh <rawdict_utf16_65105_freq.txt> <out.bin>
"""

import glob
import math
import os
import struct
import sys

MAX_CANDIDATES = 12


def write_blob(path, entries):
    """entries: {key: (cost, [candidates])}"""
    keys = sorted(entries, key=lambda k: k.encode('utf-8'))
    data = bytearray()
    offs = []
    for k in keys:
        cost, cands = entries[k]
        offs.append(len(data))
        data += k.encode('utf-8') + b'\0'
        data += struct.pack('<HB', min(cost, 0xFFFF), len(cands))
        for c in cands:
            data += c.encode('utf-8') + b'\0'
    offs.append(len(data))
    with open(path, 'wb') as f:
        f.write(b'DIME' + bytes([1, 0, 0, 0]))
        f.write(struct.pack('<I', len(keys)))
        f.write(struct.pack('<%dI' % len(offs), *offs))
        f.write(data)
    print('%s: %d keys, %d bytes' % (path, len(keys), os.path.getsize(path)))


def hira_to_kata(s):
    return ''.join(chr(ord(c) + 0x60) if 0x3041 <= ord(c) <= 0x3096 else c for c in s)


# Japanese -----------------------------------------------------------------

# Proper nouns (person / place / organisation names) are kept only when
# common; everything else up to COST_MAX.
JA_COST_MAX = 4500
JA_PROPER_COST_MAX = 3500
JA_KATAKANA_COST_MAX = 4000
JA_HIRAGANA_COST_MAX = 2000
JA_HIRAGANA_RANK_BIAS = 500
JA_NOUN_POS = '\u540d\u8a5e'   # noun
# Function words are kept whatever their surface, so the segmenter can
# split particles and auxiliaries off.
JA_FUNCTION_POS = ('\u52a9\u8a5e', '\u52a9\u52d5\u8a5e')  # particle, auxiliary verb


def gen_ja(src, out):
    pos = {}
    for line in open(os.path.join(src, 'id.def'), encoding='utf-8'):
        i, p = line.split(' ', 1)
        pos[int(i)] = p.strip()
    best = {}
    for f in sorted(glob.glob(os.path.join(src, 'dictionary0*.txt'))):
        for line in open(f, encoding='utf-8'):
            cols = line.rstrip('\n').split('\t')
            if len(cols) > 5:
                continue            # SPELLING_CORRECTION and similar
            reading, lid, _, cost, surface = cols
            cost = int(cost)
            p = pos[int(lid)]
            function = p.startswith(JA_FUNCTION_POS)
            # Katakana spellings of a reading: the engine always offers
            # one; a dictionary entry only ranks it, which matters for
            # common loanwords (nouns). Katakana forms of function words
            # and inflected words (desu -> DESU) are dropped.
            if surface == hira_to_kata(reading) and surface != reading:
                if not p.startswith(JA_NOUN_POS) or cost > JA_KATAKANA_COST_MAX:
                    continue
            if not function:
                if '\u56fa\u6709\u540d\u8a5e' in p:   # proper noun
                    if cost > JA_PROPER_COST_MAX:
                        continue
                elif cost > JA_COST_MAX:
                    continue
                # The engine always offers the hiragana form itself; an
                # entry only ranks it, for words usually written in kana.
                if surface == reading and cost > JA_HIRAGANA_COST_MAX:
                    continue
            # Ranking cost: a hiragana spelling of a content word ranks
            # a little lower than its cost says (watashi: the kanji
            # first); particles and auxiliaries keep their cost.
            rank = cost
            if surface == reading and not function:
                rank += JA_HIRAGANA_RANK_BIAS
            k = (reading, surface)
            if k not in best or rank < best[k][1]:
                best[k] = (cost, rank)
    by_reading = {}
    for (r, s), (c, rank) in best.items():
        by_reading.setdefault(r, []).append((c, rank, s))
    entries = {}
    for r, lst in by_reading.items():
        # Equal ranks put katakana last.
        kata = hira_to_kata(r)
        lst.sort(key=lambda e: (e[1], e[2] == kata, e[2]))
        entries[r] = (min(e[0] for e in lst), [e[2] for e in lst[:MAX_CANDIDATES]])
    write_blob(out, entries)


# Chinese ------------------------------------------------------------------

def gen_zh(src, out):
    raw = open(src, 'rb').read()
    text = raw.decode('utf-16')
    by_key = {}
    for line in text.splitlines():
        cols = line.split()
        if len(cols) < 4:
            continue
        word, freq = cols[0], float(cols[1])
        key = ' '.join(cols[3:])
        by_key.setdefault(key, {})
        prev = by_key[key].get(word, 0.0)
        by_key[key][word] = max(prev, freq)
    entries = {}
    for key, words in by_key.items():
        lst = sorted(words.items(), key=lambda x: -x[1])
        # cost = -log(freq), scaled to roughly the range of Mozc's costs
        cost = int(min(0xFFFF, max(0, 8000 - 500 * math.log(lst[0][1] + 1))))
        entries[key] = (cost, [w for w, _ in lst[:MAX_CANDIDATES]])
    write_blob(out, entries)


def main():
    if len(sys.argv) != 4 or sys.argv[1] not in ('ja', 'zh'):
        sys.exit(__doc__)
    (gen_ja if sys.argv[1] == 'ja' else gen_zh)(sys.argv[2], sys.argv[3])


if __name__ == '__main__':
    main()
