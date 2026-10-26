# Language packs: Vietnamese, Korean, Japanese, Chinese

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

How to build an image for a pack is described in
[BUILDING.md](../BUILDING.md#language-packs).

With an input method (VI, KO, JA, ZH), the text being typed is
composed right in the document, before the cursor. Candidates to pick
from are listed in the status bar at the bottom. Moving the cursor, a
Ctrl / Alt shortcut or switching the layout ends the composition and
leaves the text as it is shown. Input methods work in the editor; the
small text fields (search, file names, WiFi password) take plain
letters.

Use a US-English (QWERTY) keyboard for all four. Korean keyboards
type the jamo printed on a US keyboard's keys in the Dubeolsik layout.

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

Font and dictionary licenses: `firmware/components/fonts/cjk/LICENSE.txt`
and `firmware/components/ime/dict/LICENSE.txt`.
