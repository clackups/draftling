# Typing guide: Indic pack (Devanagari and Tamil)

Layouts: US plus four Devanagari and four Tamil ones. US, HI-IN and
TA99 are active from the start; turn the others on in F1 -> Settings ->
Keyboard -> Active layouts. **Ctrl+L** or **Win+Space** switches
between the active layouts.

| Tag | Layout | Best for |
|-----|--------|----------|
| HI-IN | Hindi InScript (Government of India standard) | Hindi, Sanskrit and the other Devanagari languages |
| MR-IN | Marathi InScript | Marathi (adds eyelash ra and candra a) |
| NE-TR | Nepali Traditional | Nepali typists used to the traditional layout |
| NE-RO | Nepali Romanized | letters on the keys of their Latin sound |
| TA99 | Tamil 99 (Government of Tamil Nadu standard) | most Tamil typists |
| TA-TW | Tamil Typewriter | trained typewriter typists |
| TA-IN | Tamil InScript (Government of India standard) | users of the Indian InScript layouts |
| TA-TV | Taravinyas | typing Tamil by place and manner of articulation |

## Devanagari

All four are plain layouts: type in the order the letters are spoken
-- consonant, then its vowel sign -- and the editor draws the syllable
(the i sign before its consonant, conjuncts, the reph above). To join
two consonants, type the virama (halant) between them: `k` `d` `m` on
InScript gives k-virama-sa, drawn as one conjunct.

### HI-IN and MR-IN: InScript

Vowel signs on the left hand (with Shift: the vowels), consonants on
the right; the number row types the Devanagari digits.

| Keys | Letters |
|------|---------|
| `e f r g t s w a q` | aa, i, ii, u, uu, e, ai, o, au signs |
| Shift + `D E F R G T S W A Q` | a aa i ii u uu e ai o au |
| `d` / `x` / Shift+`X` / Shift+`-` | virama / anusvara / candrabindu / visarga |
| `k` Shift+`K` `i` Shift+`I` Shift+`U` | ka kha ga gha nga |
| `;` Shift+`;` `p` Shift+`P` Shift+`]` | ca cha ja jha nya |
| `'` Shift+`'` `[` Shift+`[` Shift+`C` | Ta Tha Da Dha Na |
| `l` Shift+`L` `o` Shift+`O` `v` | ta tha da dha na |
| `h` Shift+`H` `y` Shift+`Y` `c` | pa pha ba bha ma |
| `/` `j` `n` `b` `M` `<` `m` `u` | ya ra la va sha ssa sa ha |
| `]` / `=` / Shift+`.` | nukta / vocalic r sign / danda |
| Shift + `3 4 5 6 7 8` | ra below (rakar), reph, jnya, tra, ksha, shra |
| AltGr + `1` / `2` | zero-width joiner / non-joiner |

Examples: `u f x o r` = hindi, `v c m d l s` = namaste, `Y e j l` =
bharat. Shift+`N` types lla. MR-IN differs only in a few Marathi
letters: Shift+`J` eyelash ra and Shift+`1` candra a.

### NE-TR: Nepali Traditional

The layout of Nepali typewriters and the Preeti font (as m17n
"ne-trad"). Example: `g ] k f n` = nepal.

### NE-RO: Nepali Romanized

Letters on the keys of their Latin sound: `k` ka, `g` ga, `n` na,
`a` aa sign, `i` i sign, `e` e sign, `o` o sign, `/` virama, Shift for
the aspirated or retroflex letter (`K` kha, `T` tha, `N` Na). Examples:
`n e p a l` = nepal, `n m s / t e` = namaste.

## Tamil

Letters are named here by transliteration: `Ta` / `Na` = retroflex t /
n, `n2a` = the alveolar n, `zha`, `La` (retroflex l), `Ra` (alveolar r).

### TA99: Tamil 99

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

### TA-TW: Tamil Typewriter

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

### TA-IN: InScript

A plain layout: vowel signs on the left hand (Shift: the vowels),
consonants on the right, pulli on `d`. Type in Unicode order:
consonant first, then the vowel sign. Examples: `k e` = kaa,
`l c f` Shift+`B` `d` = tamizh. AltGr + digit types the Tamil digits.

### TA-TV: Taravinyas

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

- Left / Right step over a whole syllable or conjunct; Backspace
  removes the vowel sign first, then the consonant.
- The search, file-name and WiFi fields take plain letters.
- Full tables: [docs/language-packs.md](../language-packs.md#indic-devanagari-and-tamil).
