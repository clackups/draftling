#pragma once

/* Helpers shared by the Tamil input methods (ime_tamil99.c,
 * ime_tamil_tw.c, ime_taravinyas.c). Not public API. */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "ime_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Code points used by more than one engine. */
#define TA_A        0x0B85u   /* independent vowel a */
#define TA_KA       0x0B95u
#define TA_SSA      0x0BB7u
#define TA_SA       0x0BB8u
#define TA_RA       0x0BB0u
#define TA_SIGN_AA  0x0BBEu
#define TA_SIGN_I   0x0BBFu
#define TA_SIGN_II  0x0BC0u
#define TA_SIGN_U   0x0BC1u
#define TA_SIGN_UU  0x0BC2u
#define TA_SIGN_E   0x0BC6u
#define TA_SIGN_EE  0x0BC7u
#define TA_SIGN_AI  0x0BC8u
#define TA_SIGN_O   0x0BCAu
#define TA_SIGN_OO  0x0BCBu
#define TA_SIGN_AU  0x0BCCu
#define TA_PULLI    0x0BCDu
#define TA_AYTHAM   0x0B83u
#define TA_ZWNJ     0x200Cu
#define TA_DOTTED_CIRCLE 0x25CCu

/* The twelve vowels in the order a aa i ii u uu e ee ai o oo au:
 * independent letter and vowel sign (0 for a, which has none). */
#define TA_VOWELS 12
extern const uint16_t TA_VOWEL_LETTER[TA_VOWELS];
extern const uint16_t TA_VOWEL_SIGN[TA_VOWELS];

/* A consonant letter (Tamil or Grantha). */
bool ta_is_consonant(uint32_t cp);

/* Read up to `max` code points right before the cursor (same line),
 * last one in cps[n - 1]; bytes[i] is the UTF-8 length of cps[i].
 * Returns n. */
size_t ta_peek(uint32_t *cps, uint8_t *bytes, size_t max);

/* Replace `del` bytes before the cursor with the n code points. */
bool ta_put(size_t del, const uint32_t *cps, size_t n);

/* Encode n code points as UTF-8 into out (size bytes, NUL-terminated). */
void ta_encode(const uint32_t *cps, size_t n, char *out, size_t size);

/* The single ASCII character of a key's text, or 0. */
char ta_key_char(const char *text);

#ifdef __cplusplus
}
#endif
