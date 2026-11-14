#pragma once

/*
 * Taller lines for the Indic scripts (CONFIG_DRAFTLING_TALL_LINES).
 *
 * Tamil, Devanagari and Bengali need much more room above and below
 * the letter body than Latin text (loops, hooks, vowel signs and the
 * reph above, conjunct and vowel-sign stacks below). With any of them
 * compiled in, lowres_init() / hires_init() make copies of the base fonts whose
 * lines are TALL_LINE_EXTRA() taller, TALL_BASE_EXTRA() of it below
 * the baseline, and the editor renders with them; the Tamil,
 * Devanagari and Bengali glyphs are generated to fill those lines
 * (tools/gen_tamil_font.py line_extra(), tools/gen_indic_font.py).
 */

/* Extra line height over a base font slot of line height lh (about
 * 30 %), and how much of it goes below the baseline. */
#define TALL_LINE_EXTRA(lh) (((lh) * 3 + 5) / 10)
#define TALL_BASE_EXTRA(lh) (TALL_LINE_EXTRA(lh) / 2)
