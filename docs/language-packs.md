# Language packs: Vietnamese, Korean, Japanese, Chinese, Tamil

Draftling firmware comes in several language packs. Each one is a
separate firmware image with US-English plus the layouts, input
methods and fonts of its languages:

| Pack | Layouts (Ctrl+L / Win+Space cycles) |
|------|-------------------------------------|
| Default | US, UA (Ukrainian), DE (German), FR (French), HE (Hebrew) |
| Vietnamese | US, VI |
| Korean | US, KO |
| Japanese | US, JA |
| Chinese | US, ZH (Simplified Chinese) |
| Tamil | US, TA99 (Tamil 99), TA-TW (Tamil Typewriter), TA-IN (Tamil InScript), TA-TV (Taravinyas) |

How to build an image for a pack is described in
[BUILDING.md](../BUILDING.md#language-packs).

Brief typing guides, one per pack:
[default](typing/default.md) (European and Canadian layouts),
[Vietnamese](typing/vietnamese.md), [Korean](typing/korean.md),
[Japanese](typing/japanese.md), [Chinese](typing/chinese.md),
[Tamil](typing/tamil.md). The sections below have the full details.

With an input method (VI, KO, JA, ZH, and the Tamil layouts other
than InScript), the text being typed is
composed right in the document, before the cursor. Candidates to pick
from are listed in the status bar at the bottom. Moving the cursor, a
Ctrl / Alt shortcut or switching the layout ends the composition and
leaves the text as it is shown. Input methods work in the editor; the
small text fields (search, file names, WiFi password) take plain
letters.

Use a US-English (QWERTY) keyboard for all of them. Korean keyboards
type the jamo printed on a US keyboard's keys in the Dubeolsik layout,
Tamil keyboards the letters of their layout.

## Vietnamese (VI): Telex

Type the letters, then the marks; the word is fixed up as you type:

| Keys | Result |
|------|--------|
| aa, ee, oo | a e o with circumflex |
| aw, ow, uw | a with breve, o / u with horn; `uow` gives both horns; `w` alone gives u with horn |
| dd | d with stroke |
| s, f, r, x, j | acute, grave, hook above, tilde, dot below |
| z | removes the tone mark |

Examples: `Vieejt Nam`, `tieengs Vieejt`, `dduwowcj`, `nguwowif`.

Typing a mark key twice types the letter itself (`ass` -> `as`). Marks
are applied only when the word looks like a Vietnamese syllable, so
most English words come through unchanged; switch to US for English
text anyway, since short words like `is` are valid Vietnamese
syllables. A tone can be added at any time while the cursor is at the
end of the word. Tones are placed on the traditional position (`hoaf`
-> hoa with the grave on the o).

## Korean (KO): 2-set Hangul (Dubeolsik)

The keys type jamo, which are joined into syllables as you type, e.g.
`dkssudgktpdy` -> annyeonghaseyo, `gksrnrdj` -> hangugeo. Shift types
the tense consonants (Q W E R T) and yae / ye (O P). Backspace removes
the last jamo of the syllable being typed, not the whole syllable.

## Japanese (JA): romaji, kana and kanji

Type romaji; it turns into hiragana as soon as it forms a syllable
(`konnichiha` -> konnichiha in hiragana). `nn` or `n'` types the moraic
n, a doubled consonant (`kk`, `tt`) the small tsu, `-` the long-vowel
mark, `xa` / `la` small kana.

Press **Space** to convert the reading. Draftling splits it into words
and converts the first one; its candidates are listed in the status
bar:

| Key | Action |
|-----|--------|
| Space / Down | next candidate |
| Up | previous candidate |
| PageDown / PageUp | next / previous page |
| 1 .. 9 | pick a candidate of the page; the next word is converted |
| Left / Right | make the converted word shorter / longer |
| Enter | accept the candidate, keep the rest as kana |
| Esc / Backspace | back to kana |

Typing a letter accepts the conversion and starts a new reading.
Enter or Esc while not converting keeps the kana. The candidates
always include the hiragana and katakana spellings.

`,` `.` `[` `]` `/` `~` type the Japanese comma and full stop, the
corner brackets, the middle dot and the wave dash. They end the
reading, so convert with Space before typing them.

The conversion dictionary is derived from the Mozc open-source
dictionary (about 30,000 readings). Words are split with word costs
only, without grammar, so a long sentence may split oddly: convert in
shorter pieces, or fix the split with Left / Right.

## Chinese (ZH): pinyin

Type pinyin without tones; the letters appear in the text and the
candidates for them in the status bar, e.g. `nihao` -> ni hao.
Syllables are split automatically; type `'` to force a break
(`xi'an`), and `v` for u-umlaut (`lv`, `nv`).

| Key | Action |
|-----|--------|
| Space | accept the selected candidate |
| 1 .. 9 | accept a candidate of the page |
| Left / Right | select the previous / next candidate |
| Up / Down, PageUp / PageDown, - / = | previous / next page |
| Enter | keep the letters as typed |
| Esc | drop the letters |
| Backspace | delete the last letter |

The list starts with words for all typed syllables, then for fewer
syllables, down to single characters. Accepting a shorter word leaves
the remaining letters in composition, with new candidates. Outside a
composition, `,` `.` `?` `!` `:` `;` and `\` type full-width Chinese
punctuation. The dictionary comes from the Android Open Source
Project's PinyinIME (about 65,000 words).

## Tamil

The Tamil pack has the official Tamil keyboard layouts -- Tamil 99
(the Government of Tamil Nadu standard), the Tamil Typewriter layout
and InScript (the Government of India standard) -- plus the
Taravinyas input engine. Only US and Tamil 99 are active at first;
turn the others on in F1 -> Settings -> Keyboard -> Active layouts.
The letters are named here by their usual transliteration (`ka`,
`Ta` = retroflex t, `Na` = retroflex n, `n2a` = the alveolar n,
`zha`, `La`, `Ra`); the vowel sign of a vowel is "the aa sign" and
so on, and the pulli is the dot that removes the inherent a.

### Tamil 99 (TA99)

Vowels are on the left hand, consonants on the right; every key types
a whole letter. A vowel typed after a consonant joins it as its vowel
sign (`h` `q` = kaa), so typing follows the spoken syllables.

| Keys | Letters |
|------|---------|
| `a q s w d e g t r c x z` | a aa i ii u uu e ee ai o oo au |
| `h b [ ] o p l ; j k` | ka nga ca nya Ta Na ta na pa ma |
| `' m n v / y u i` | ya ra la va zha La Ra n2a |
| `f` / Shift+`F` | pulli / aytham |
| Shift+`Q W E R U` | Grantha sa, ssa, ja, ha, sha |
| Shift+`T` / Shift+`Y` | k-ssa / sri |
| Shift+`A S D Z X C V B L N` | rupee, number, "as above", day, month, year, debit, credit, hundred, om signs |

The pulli is added automatically between two equal consonants (`h`
`h` = k-pulli ka) and between a nasal and its stop (nga-ka, nya-ca,
Na-Ta, na-ta, ma-pa, n2a-Ra). After an automatic pulli the next
consonant gets none, so three equal consonants give k-pulli ka ka.
`a` after a consonant confirms its inherent a: the next vowel then
stands on its own and the next consonant gets no automatic pulli.
Shift+`I O P K` and Shift+`;` `'` `/` type the punctuation the
consonant keys took (`:` `[` `]` `"` `;` `'` `/`).

### Tamil Typewriter (TA-TW)

The layout of the Tamil typewriter, used in the Tamil Nadu government
typewriting examinations (the eKalappai / Keyman "Thamizha" typewriter
keyboard). It types glyph pieces in the order they are seen: the e,
ee and ai signs (`b`, `n`, `i`) come before the consonant, and the aa
piece `h` after it, so ko is `b` + the ka key + `h`. Draftling stores
the letters in Unicode order as you type: the e sign waits on a dotted
circle until the consonant comes, e sign + consonant + `h` becomes the
o sign, e sign + consonant + `s` (the La key) the au sign (turned back
into e + La when a pulli or vowel sign follows), and `x` `s` the
vowel au.

| Keys | Letters |
|------|---------|
| `f ' r " l z j e g k` | ka nga ca nya Ta Na ta na pa ma |
| `a u y t H s w d` | ya ra la va zha La Ra n2a |
| `! $ & +` / `\` | Grantha sa, ja, ssa, ha / k-ssa |
| `m M , < c C v V I x X` | a aa i ii u uu e ee ai o oo |
| `h p P ;` | aa sign, i sign, ii sign, pulli |
| `b n i` | e, ee, ai sign (before the consonant) |
| `[` `{` / `%` `^` | u / uu sign of nga, pa, ya, va / of the Grantha letters |
| `}` `:` | turn the u syllable before into uu (`E }` = nuu, `L :` = Tuu) |
| `]` | ai sign of Na, n2a, la, La |
| `` ` `` | aytham |
| Shift + letters | the u syllables of the typewriter (`F` ku, `R` cu, `L` Tu, ...), `T` kuu, `N` cuu, `o` / `O` Ti / Tii, `q` Nu, `_` sri |

### Tamil InScript (TA-IN)

The Government of India InScript layout for Tamil (as Windows "Tamil"
and Linux "Tamil (InScript)"). It is a plain layout without an input
method: vowel signs on the left hand (Shift: the vowels), consonants
on the right, the pulli on `d`, and each character typed in Unicode
order (consonant first, then the vowel sign). AltGr + a digit types
the Tamil digit.

### Taravinyas (TA-TV)

Taravinyas, by Bandhan Pramanik
([tamil-input-engine-checkbox](https://github.com/BandhanPramanik/tamil-input-engine-checkbox)),
picks a letter by where and how it is pronounced, with the same keys
as its web page. `v` switches between the consonant and the vowel
mode; the mode stays until `v` is pressed again. The status bar shows
the mode and what the next key picks.

Consonants: the place, then the manner.

| Place | `j` hard | `k` nasal | `l` medial | `u` medial (lateral) | `i` sibilant |
|-------|----------|-----------|------------|----------------------|--------------|
| `f` velar | ka | nga | | | |
| `d` palatal | ca | nya | ya | ya | sha |
| `s` retroflex | Ta | Na | zha | La | ssa |
| `r` dental | ta | na | | | |
| `e` labial | pa | ma | va | va | |
| `w` alveolar | Ra | n2a | ra | la | sa |

`a` then `j` / `k` / `l`: the Grantha ja, ha and k-ssa.

Vowels: the vowel (`f` a, `d` i, `s` u, `r` e, `e` o), its length
(`j` short, `k` long, `l` the diphthong: ai from e, au from o), then
`x` for the vowel sign (it joins the consonant before) or `c` for the
vowel letter. `a` then `j` / `k` / `l`: pulli, aytham, zero-width
non-joiner.

So kaa is `f j` (ka), `v`, `f k x` (aa sign), and tamizh, starting in
consonant mode, is `r j` (ta) `e k` (ma) `v` `d j x` (i sign) `v`
`s l` (zha) `v` `a j` (pulli): each letter takes two or three keys. Esc drops a half-made pick, Backspace undoes its last
step, other letter keys do nothing, and digits, punctuation and Space
type as usual.

### Tamil text on the screen

The Tamil letters come from Noto Sans Tamil, drawn in black and
white. Tamil needs much more room above and below the letters than
Latin text, so the Tamil firmware spaces all lines about 30 % further
apart than the other firmware images and draws the Tamil letters to
fill that height; the Latin letters keep their size. A consonant and its vowel sign
are drawn as one syllable, the letters of a word join up with the
font's own spacing (Tamil is not squeezed into the monospace grid of
the Latin text), and the ligatures k-ssa and sri are drawn as such.
The cursor steps over a syllable or ligature as a whole with Left /
Right; Backspace removes the vowel sign first, then the consonant. A vowel sign without a consonant before it is shown on a
dotted circle.

## Fonts

Hangul, kana, kanji and hanzi come from the pixel fonts Fusion Pixel
Font (10 and 12 px) and GNU Unifont (16 px), in the glyph style of the
pack's language (Japanese kanji in the Japanese pack, simplified hanzi
in the Chinese pack). For the larger font sizes they are scaled up by
2x or 3x, which keeps their pixels sharp. These characters take two
character cells. Each pack carries only its own language's characters:
Japanese has the kanji of JIS X 0208, Chinese the hanzi of GB 2312,
plus every character of their dictionaries.

Vietnamese letters, curly quotes and dashes come from Greybeard, or on
high-density screens from Noto Sans Mono.

Tamil letters come from Noto Sans Tamil (see "Tamil text on the
screen" above).

Font and dictionary licenses: `firmware/components/fonts/cjk/LICENSE.txt`,
`firmware/components/fonts/tamil/LICENSE.txt` and
`firmware/components/ime/dict/LICENSE.txt`.
