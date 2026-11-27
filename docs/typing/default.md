# Typing guide: default pack (European, Canadian and Brazilian layouts)

The default firmware has US-English plus 27 layouts: Ukrainian (UA),
German (DE), French (FR), Hebrew (HE), Swiss German (DE-CH), Swiss
French (FR-CH), Italian (IT), Portuguese (PT), Brazilian Portuguese
ABNT2 (BR), Spanish (ES), United
Kingdom (GB), Swedish / Finnish (SE/FI), Norwegian (NO), Danish (DK),
Polish (PL), Czech (CZ), Slovak (SK), Hungarian (HU), Romanian (RO),
Croatian / Slovenian (HR/SI), Turkish Q (TR), Canadian French
(FR-CA), Crimean Tatar Latin (CRH, CRH-B), US Dvorak (DV), French
BEPO (BEPO) and the French standard AZERTY of 2019 (FR-AF). Each one
matches the standard Windows layout of its country (Crimean Tatar:
Ave Team's layout; Dvorak, BEPO and AZERTY AFNOR: their standard
layouts), so use a keyboard with those keys printed on it.

## Getting started

1. F1 -> Settings -> Keyboard -> Active layouts. Tick your layouts with
   Enter (only US is on at first) and leave with Esc.
2. **Ctrl+L** or **Win+Space** switches to the next active layout. The
   title bar shows the current one (e.g. `[DE]`) when more than one is
   active.
3. Ctrl / Alt shortcuts follow the letters printed on your keyboard
   (Ctrl+Z is the key marked Z on a German keyboard). On Ukrainian and
   Hebrew they use the US key positions.

## Accents (dead keys)

The accent keys work as on a computer: press the accent, then the
letter.

| Keys | Result |
|------|--------|
| accent, then a letter | the accented letter (acute + e = e-acute) |
| accent, then Space | the accent on its own |
| accent twice | the accent on its own |
| accent, then a letter it does not go with | the accent, then the letter |

Czech, Slovak, Hungarian, Romanian and Croatian / Slovenian also
have the Windows AltGr accents on the number row: AltGr + `2`
caron, `3` circumflex, `4` breve, `5` ring, `6` ogonek, `8` dot above,
`9` acute, `0` double acute, each followed by the letter.

## Other keys

- **AltGr** (right Alt) types the third character printed on a key,
  e.g. the euro sign or `@` on many layouts. Use the left Alt for Alt
  shortcuts on these layouts.
- **Caps Lock** gives capital letters, including accented and Cyrillic
  ones; digits and symbols stay as they are. "CAPS" shows in the title
  bar while it is on.
- The key left of Z on European keyboards (`<` `>`) works on all
  layouts.
- **Hebrew** is written right to left; Left / Right move the cursor
  the way the arrows point. Caps Lock types Latin capitals, as on
  Windows.
- **Turkish**: the dotless i key counts as I for shortcuts.
- **Brazilian Portuguese** (BR, ABNT2): c-cedilla right of L, the
  acute / grave accent right of P, tilde / circumflex right of
  c-cedilla, the diaeresis on Shift + `6`. The extra key left of the
  right Shift types `/` and `?` (AltGr: the degree sign); AltGr + Q and
  W type them too, for keyboards without that key. AltGr + `[` and `]`
  give the ordinal indicators, AltGr + E the degree sign.
- **Crimean Tatar** (CRH, Ave Team's
  [Crimean Tatar Roman layout](https://keyboard.aveteam.org/en/crh-more)):
  QWERTY letters with the Ukrainian punctuation on the number row
  (Shift + `2` `"`, `3` No., `4` `;`, `6` `:`, `7` `?`, and `.` `,` on
  the `/` key). The Crimean Tatar letters have their own keys: g-breve
  and u-diaeresis on `[` `]`, s-cedilla and i on `;` `'`, o-diaeresis
  and c-cedilla on `,` `.`, a-circumflex left of `1`, n-tilde on `\`;
  the I key types the dotless i. AltGr + a, c, g, i, n, o, s, u also
  types the letter with its mark (AltGr + i gives i with a dot).
  AltGr + `-` en dash (with Shift: em dash), AltGr + `'` the right
  single quote, AltGr + `[` `]` `\` `/` the brackets, backslash and
  `<`. For the Romanian Crimean Tatar spelling, AltGr + Shift + `6` is
  a dead breve (dotless i -> i-breve), `7` a dead acute (a, i, o, u),
  `8` a dead comma below (s -> s-comma) and AltGr + Shift + the
  a-circumflex key a dead circumflex (dotless i -> i-circumflex).
  **CRH-B** is the same layout with the I key and the key right of
  s-cedilla swapped (I key: i, with Shift the dotted capital I; AltGr +
  i: the dotless i).
- **Dvorak** (DV): the US Dvorak layout. Ctrl / Alt shortcuts follow
  the Dvorak letters (Ctrl+C is the key that types c).
- **BEPO**: the French ergonomic layout, as on Linux (`fr(bepo)`), for
  an ISO keyboard: e-circumflex on the key left of Z, c-cedilla right
  of M. The number row types `" << >> ( ) @ + - / *` and the digits
  with Shift or Caps Lock. The key right of e-grave is a dead
  circumflex. Shift + Space types a no-break space (for the French
  spacing before `: ; ! ?`), AltGr + Space `_`, AltGr + Shift + Space
  a narrow no-break space. AltGr gives every character printed on BEPO
  keyboards, and all of its dead keys work: acute, double acute,
  grave, caron, breve, diaeresis, dot above, tilde, cedilla, comma
  below, ring, ogonek, macron (AltGr + M), stroke (AltGr + L: o, d, h,
  l, t, and `=` for not equal), currency (AltGr + Shift + E: c, e, l,
  y, S, d), Greek (AltGr + G: the Greek letter of each Latin letter,
  e.g. j theta, q chi, y psi, w omega; Space gives the micro sign),
  horn (AltGr + Shift + `,`: o, u), hook (AltGr + Shift + `'`) and dot
  below (AltGr + Shift + Q) for the Vietnamese vowels and, with dot
  below, the consonants of transliterations. Horn, hook and dot below
  followed by Space type `'`, `?` and `.`.
- **AZERTY AFNOR** (FR-AF): the French standard AZERTY of 2019 (AFNOR
  NF Z71-300, https://norme-azerty.fr/), as on Linux (`fr(afnor)`),
  for an ISO keyboard. The letters are where the classic AZERTY (FR)
  has them; the number row types `a-grave e-acute e-grave
  e-circumflex ( )`, the curly single quotes and `<< >>`, with the
  digits on Shift. With Caps Lock on, the four accented keys type
  their capitals (AltGr + Shift does too). Right of the zero are
  `'` (Shift `"`) and the dead circumflex (Shift: diaeresis), left of
  the one `@` (Shift `#`); `- + / * . , : ; < >` each have their own
  key. AltGr gives every character printed on the keyboard: c-cedilla
  on C, oe on O, ae on A, u-grave on U, the dashes, French quotes,
  no-break spaces (AltGr + Space, with Shift a narrow one) and the
  dead keys: acute and grave (AltGr + e-acute / e-grave), breve (AltGr
  + `@`), double acute, macron, ring, caron, dot above and below (I),
  currency (F), Greek (G), stroke (K and B), cedilla and ogonek (V),
  tilde (N) and comma below. **AltGr + H** then a key types the
  "European" letters: a / o the ordinals, e schwa, t thorn, i dotless
  i (Shift: I with a dot), d eth, g eng, j ij, s long s, and on 7, 8,
  9, 0 and `'` the low and English quotes (with Shift the single
  guillemets). The dead inverted breve, double grave and macron below
  of the standard type nothing: their letters are not in the fonts.
