/*
 * Bengali National (Jatiya) input: the "automatic vowel forming" of
 * m17n's bn-national-jatiya input method.
 *
 * The layout (kb_layout.cpp, BN-NJ) puts the vowel signs on the vowel
 * keys, as typed after a consonant (k-key + h-key = ka + aa sign).
 * Where no consonant comes before the cursor, the key types the
 * independent vowel instead (h-key alone = the letter aa), and after
 * the hasanta (virama, g-key) it replaces the hasanta with the
 * independent vowel: that is how a vowel letter is typed right after a
 * consonant (k, g, h = ka + letter aa). The danda key after the
 * hasanta gives the double danda.
 *
 * Stateless: every key looks at the text before the cursor, so moving
 * the cursor next to a consonant and typing a vowel key gives the
 * sign there.
 */

#include <string.h>
#include "ime_internal.h"

#define BN_VIRAMA 0x09CDu
#define BN_NUKTA  0x09BCu
#define DANDA     0x0964u
#define DDANDA    0x0965u

/* Vowel sign -> independent vowel. */
static const uint16_t VOWELS[][2] = {
    { 0x09BE, 0x0986 },   /* aa */
    { 0x09BF, 0x0987 },   /* i */
    { 0x09C0, 0x0988 },   /* ii */
    { 0x09C1, 0x0989 },   /* u */
    { 0x09C2, 0x098A },   /* uu */
    { 0x09C3, 0x098B },   /* vocalic r */
    { 0x09C4, 0x09E0 },   /* vocalic rr */
    { 0x09C7, 0x098F },   /* e */
    { 0x09C8, 0x0990 },   /* ai */
    { 0x09CB, 0x0993 },   /* o */
    { 0x09CC, 0x0994 },   /* au */
    { 0x09E2, 0x098C },   /* vocalic l */
    { 0x09E3, 0x09E1 },   /* vocalic ll */
};

static bool is_consonant(uint32_t cp)
{
    return (cp >= 0x0995 && cp <= 0x09B9) || (cp >= 0x09DC && cp <= 0x09DF) ||
           cp == 0x09F0 || cp == 0x09F1;
}

/* The last code point before the cursor (same line) and its UTF-8
 * length, or 0. */
static uint32_t peek_last(size_t *bytes)
{
    char buf[8];
    size_t n = ime_peek(buf, sizeof(buf));
    *bytes = 0;
    if (n == 0) return 0;
    size_t l = utf8_last_len(buf, n);
    size_t used;
    uint32_t cp = utf8_decode(buf + n - l, l, &used);
    *bytes = l;
    return cp;
}

static bool put(size_t del, uint32_t cp)
{
    char out[5];
    size_t n = utf8_encode(cp, out);
    out[n] = '\0';
    ime_replace(del, out);
    return true;
}

static bool bn_key(uint8_t keycode, uint8_t modifier, const char *text)
{
    (void)keycode;
    (void)modifier;
    if (!text || !text[0]) return false;
    size_t len = strlen(text), used;
    uint32_t cp = utf8_decode(text, len, &used);
    if (used != len) return false;           /* not a single character */

    size_t pb;
    uint32_t prev = peek_last(&pb);
    if (cp == DANDA) {
        if (prev != BN_VIRAMA) return false;
        return put(pb, DDANDA);
    }
    for (size_t i = 0; i < sizeof(VOWELS) / sizeof(VOWELS[0]); i++) {
        if (cp != VOWELS[i][0]) continue;
        if (prev == BN_VIRAMA) return put(pb, VOWELS[i][1]);
        if (is_consonant(prev) || prev == BN_NUKTA) return false;   /* the sign */
        return put(0, VOWELS[i][1]);
    }
    return false;
}

static void bn_commit(void) {}
static bool bn_composing(void) { return false; }
static const char *bn_status(void) { return NULL; }

const ime_engine_t ime_bengali_nj_engine = {
    bn_key, bn_commit, bn_composing, bn_status,
};
