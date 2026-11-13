# Typing guide: Tamil pack

Layouts: US plus four Tamil methods. US and TA99 are active from the
start; turn the others on in F1 -> Settings -> Keyboard -> Active
layouts. **Ctrl+L** or **Win+Space** switches between the active
layouts.

| Tag | Method | Best for |
|-----|--------|----------|
| TA99 | Tamil 99 (Government of Tamil Nadu standard) | most people: one key per letter, syllables formed automatically |
| TA-TW | Tamil Typewriter | trained typewriter typists |
| TA-IN | Tamil InScript (Government of India standard) | users of the Indian InScript layouts |
| TA-TV | Taravinyas | typing by place and manner of articulation |

Letters are named here by transliteration: `Ta` / `Na` = retroflex t /
n, `n2a` = the alveolar n, `zha`, `La` (retroflex l), `Ra` (alveolar r).

## TA99: Tamil 99

Vowels on the left hand, consonants on the right. A vowel after a
consonant joins it as its vowel sign.

| Keys | Letters |
|------|---------|
| `a q s w d e g t r c x z` | a aa i ii u uu e ee ai o oo au |
| `h b [ ] o p l ; j k` | ka nga ca nya Ta Na ta na pa ma |
| `' m n v / y u i` | ya ra la va zha La Ra n2a |
| `f` / Shift+`F` | pulli / aytham |
| Shift+`Q W E R U`, `T`, `Y` | sa ssa ja ha sha, k-ssa, sri |

Doubled consonants and nasal + stop pairs get the pulli by themselves:
`h h` = k-pulli ka, `; l` = n-pulli ta. Type `a` after a consonant to
keep the next vowel separate.

Examples: `lks/f` = tamizh, `hq` = kaa, `jhh` = pakka, `k;l` = manta.

## TA-TW: Tamil Typewriter

Type in the order the letters are seen: the e, ee and ai signs (`b`,
`n`, `i`) before the consonant, the aa piece (`h`) after it. The sign
waits on a dotted circle until its consonant comes.

| Keys | Letters |
|------|---------|
| `f ' r " l z j e g k` | ka nga ca nya Ta Na ta na pa ma |
| `a u y t H s w d` | ya ra la va zha La Ra n2a |
| `m , c v x` (Shift: aa ii uu ee oo, `I` ai) | a i u e o |
| `h p P ;` | aa sign, i sign, ii sign, pulli |
| `b n i` | e, ee, ai sign (typed first) |

Examples: `b f h` = ko, `n f h` = koo, `j k p H ;` = tamizh,
`i g` = pai.

## TA-IN: InScript

A plain layout: vowel signs on the left hand (Shift: the vowels),
consonants on the right, pulli on `d`. Type in Unicode order:
consonant first, then the vowel sign. Examples: `k e` = kaa,
`l c f` Shift+`B` `d` = tamizh. AltGr + digit types the Tamil digits.

## TA-TV: Taravinyas

`v` switches between consonant and vowel mode. The status bar shows
the mode and the next choices.

- Consonant: place (`f` velar, `d` palatal, `s` retroflex, `r`
  dental, `e` labial, `w` alveolar), then manner (`j` hard, `k`
  nasal, `l` medial, `u` lateral medial, `i` sibilant).
- Vowel: vowel (`f` a, `d` i, `s` u, `r` e, `e` o), length (`j` short,
  `k` long, `l` ai / au), then `x` for the sign or `c` for the letter.
- `a` then `j` / `k` / `l`: Grantha ja / ha / k-ssa in consonant mode;
  pulli / aytham / ZWNJ in vowel mode.
- Esc drops a half-made pick, Backspace undoes its last step.

Example, starting in consonant mode: `r j` `e k` `v` `d j x` `v`
`s l` `v` `a j` = tamizh.

## Tips

- Left / Right step over a whole syllable; Backspace removes the vowel
  sign first, then the consonant.
- The search, file-name and WiFi fields take plain letters.
- Full tables: [docs/language-packs.md](../language-packs.md#tamil).
