# Typing guide: Japanese pack

Layouts: US and JA (romaji input with kana-kanji conversion). Use a
US-English keyboard. JA is active from the start; **Ctrl+L** or
**Win+Space** switches between US and JA.

## 1. Type romaji

Romaji turns into hiragana as soon as it forms a syllable:
`konnichiha` -> konnichiha in hiragana.

| Keys | Result |
|------|--------|
| `nn` or `n'` | the moraic n |
| doubled consonant (`kk`, `tt`) | small tsu |
| `-` | long-vowel mark |
| `xa`, `la` (any vowel) | small kana |
| `,` `.` `[` `]` `/` `~` | Japanese comma and full stop, corner brackets, middle dot, wave dash |

## 2. Convert with Space

| Key | Action |
|-----|--------|
| Space / Down | convert, then next candidate |
| Up | previous candidate |
| PageDown / PageUp | next / previous page |
| 1 .. 9 | pick a candidate; the next word is converted |
| Left / Right | make the converted word shorter / longer |
| Enter | accept, keep the rest as kana |
| Esc / Backspace | back to kana |

The candidates are listed in the status bar and always include the
hiragana and katakana spellings. Typing a letter accepts the
conversion and starts a new reading.

## Examples

| Keys | Result |
|------|--------|
| `nihongo` Space Enter | nihongo in kanji (Japanese language) |
| `toukyou` Space Enter | Tokyo in kanji |
| `ko-hi-` Space, pick the katakana | koohii in katakana (coffee) |

## Tips

- Convert in short pieces: the dictionary splits sentences by word
  cost only, without grammar. Fix an odd split with Left / Right.
- The punctuation keys end the reading, so convert before typing them.
- The search, file-name and WiFi fields take plain letters.
