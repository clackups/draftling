/*
 * Japanese input: romaji -> hiragana, Space -> kanji.
 *
 * Composing: letters are converted to hiragana as soon as they form a
 * syllable (ka, shi, tsu, kya, ...); "nn" or "n'" types the moraic n,
 * n before a consonant does too, and a doubled consonant (kk, tt)
 * types the small tsu. "-" "," "." "[" "]" type the long-vowel mark,
 * the Japanese comma and full stop and the corner brackets.
 *
 * Space converts the reading. The engine splits it into words with a
 * small Viterbi search over dictionary costs (unigram costs plus a
 * per-word penalty; no connection costs) and offers candidates for the
 * first word: the dictionary entries, then the hiragana and katakana
 * forms. The candidate is shown in the text, the rest stays in kana.
 *
 *   Space / Down     next candidate         Up          previous
 *   PageDown/PageUp  next / previous page   1..9        pick from page
 *   Left / Right     shorter / longer word  Enter       accept
 *   Esc / Backspace  back to kana
 *
 * Picking a candidate converts the next word; Enter accepts the
 * candidate and keeps the rest as kana. Typing a letter accepts
 * everything and starts a new reading. Enter while composing keeps the
 * kana; Esc does too.
 */

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "ime_internal.h"

typedef struct {
    const char *romaji;
    const char *kana;
} romaji_t;

/* Romaji -> hiragana, sorted by romaji (byte order) for binary search.
 * Generated; the kana are UTF-8 escapes. "nn" and "n'" give the moraic
 * n; a doubled consonant gives the small tsu (handled in code). */
static const romaji_t ROMAJI[] = {
    { ",", "\xE3\x80\x81" },
    { "-", "\xE3\x83\xBC" },
    { ".", "\xE3\x80\x82" },
    { "/", "\xE3\x83\xBB" },
    { "[", "\xE3\x80\x8C" },
    { "]", "\xE3\x80\x8D" },
    { "a", "\xE3\x81\x82" },
    { "ba", "\xE3\x81\xB0" },
    { "be", "\xE3\x81\xB9" },
    { "bi", "\xE3\x81\xB3" },
    { "bo", "\xE3\x81\xBC" },
    { "bu", "\xE3\x81\xB6" },
    { "bya", "\xE3\x81\xB3\xE3\x82\x83" },
    { "bye", "\xE3\x81\xB3\xE3\x81\x87" },
    { "byo", "\xE3\x81\xB3\xE3\x82\x87" },
    { "byu", "\xE3\x81\xB3\xE3\x82\x85" },
    { "ca", "\xE3\x81\x8B" },
    { "cha", "\xE3\x81\xA1\xE3\x82\x83" },
    { "che", "\xE3\x81\xA1\xE3\x81\x87" },
    { "chi", "\xE3\x81\xA1" },
    { "cho", "\xE3\x81\xA1\xE3\x82\x87" },
    { "chu", "\xE3\x81\xA1\xE3\x82\x85" },
    { "co", "\xE3\x81\x93" },
    { "cu", "\xE3\x81\x8F" },
    { "cya", "\xE3\x81\xA1\xE3\x82\x83" },
    { "cye", "\xE3\x81\xA1\xE3\x81\x87" },
    { "cyo", "\xE3\x81\xA1\xE3\x82\x87" },
    { "cyu", "\xE3\x81\xA1\xE3\x82\x85" },
    { "da", "\xE3\x81\xA0" },
    { "de", "\xE3\x81\xA7" },
    { "dhi", "\xE3\x81\xA7\xE3\x81\x83" },
    { "dhu", "\xE3\x81\xA7\xE3\x82\x85" },
    { "di", "\xE3\x81\xA2" },
    { "do", "\xE3\x81\xA9" },
    { "du", "\xE3\x81\xA5" },
    { "dwu", "\xE3\x81\xA9\xE3\x81\x85" },
    { "dya", "\xE3\x81\xA2\xE3\x82\x83" },
    { "dye", "\xE3\x81\xA2\xE3\x81\x87" },
    { "dyo", "\xE3\x81\xA2\xE3\x82\x87" },
    { "dyu", "\xE3\x81\xA2\xE3\x82\x85" },
    { "e", "\xE3\x81\x88" },
    { "fa", "\xE3\x81\xB5\xE3\x81\x81" },
    { "fe", "\xE3\x81\xB5\xE3\x81\x87" },
    { "fi", "\xE3\x81\xB5\xE3\x81\x83" },
    { "fo", "\xE3\x81\xB5\xE3\x81\x89" },
    { "fu", "\xE3\x81\xB5" },
    { "fya", "\xE3\x81\xB5\xE3\x82\x83" },
    { "fyo", "\xE3\x81\xB5\xE3\x82\x87" },
    { "fyu", "\xE3\x81\xB5\xE3\x82\x85" },
    { "ga", "\xE3\x81\x8C" },
    { "ge", "\xE3\x81\x92" },
    { "gi", "\xE3\x81\x8E" },
    { "go", "\xE3\x81\x94" },
    { "gu", "\xE3\x81\x90" },
    { "gwa", "\xE3\x81\x90\xE3\x81\x81" },
    { "gya", "\xE3\x81\x8E\xE3\x82\x83" },
    { "gye", "\xE3\x81\x8E\xE3\x81\x87" },
    { "gyo", "\xE3\x81\x8E\xE3\x82\x87" },
    { "gyu", "\xE3\x81\x8E\xE3\x82\x85" },
    { "ha", "\xE3\x81\xAF" },
    { "he", "\xE3\x81\xB8" },
    { "hi", "\xE3\x81\xB2" },
    { "ho", "\xE3\x81\xBB" },
    { "hu", "\xE3\x81\xB5" },
    { "hya", "\xE3\x81\xB2\xE3\x82\x83" },
    { "hye", "\xE3\x81\xB2\xE3\x81\x87" },
    { "hyo", "\xE3\x81\xB2\xE3\x82\x87" },
    { "hyu", "\xE3\x81\xB2\xE3\x82\x85" },
    { "i", "\xE3\x81\x84" },
    { "ja", "\xE3\x81\x98\xE3\x82\x83" },
    { "je", "\xE3\x81\x98\xE3\x81\x87" },
    { "ji", "\xE3\x81\x98" },
    { "jo", "\xE3\x81\x98\xE3\x82\x87" },
    { "ju", "\xE3\x81\x98\xE3\x82\x85" },
    { "jya", "\xE3\x81\x98\xE3\x82\x83" },
    { "jye", "\xE3\x81\x98\xE3\x81\x87" },
    { "jyo", "\xE3\x81\x98\xE3\x82\x87" },
    { "jyu", "\xE3\x81\x98\xE3\x82\x85" },
    { "ka", "\xE3\x81\x8B" },
    { "ke", "\xE3\x81\x91" },
    { "ki", "\xE3\x81\x8D" },
    { "ko", "\xE3\x81\x93" },
    { "ku", "\xE3\x81\x8F" },
    { "kya", "\xE3\x81\x8D\xE3\x82\x83" },
    { "kye", "\xE3\x81\x8D\xE3\x81\x87" },
    { "kyi", "\xE3\x81\x8D\xE3\x81\x83" },
    { "kyo", "\xE3\x81\x8D\xE3\x82\x87" },
    { "kyu", "\xE3\x81\x8D\xE3\x82\x85" },
    { "la", "\xE3\x81\x81" },
    { "le", "\xE3\x81\x87" },
    { "li", "\xE3\x81\x83" },
    { "lo", "\xE3\x81\x89" },
    { "ltsu", "\xE3\x81\xA3" },
    { "ltu", "\xE3\x81\xA3" },
    { "lu", "\xE3\x81\x85" },
    { "lwa", "\xE3\x82\x8E" },
    { "lya", "\xE3\x82\x83" },
    { "lyo", "\xE3\x82\x87" },
    { "lyu", "\xE3\x82\x85" },
    { "ma", "\xE3\x81\xBE" },
    { "me", "\xE3\x82\x81" },
    { "mi", "\xE3\x81\xBF" },
    { "mo", "\xE3\x82\x82" },
    { "mu", "\xE3\x82\x80" },
    { "mya", "\xE3\x81\xBF\xE3\x82\x83" },
    { "mye", "\xE3\x81\xBF\xE3\x81\x87" },
    { "myo", "\xE3\x81\xBF\xE3\x82\x87" },
    { "myu", "\xE3\x81\xBF\xE3\x82\x85" },
    { "n'", "\xE3\x82\x93" },
    { "na", "\xE3\x81\xAA" },
    { "ne", "\xE3\x81\xAD" },
    { "ni", "\xE3\x81\xAB" },
    { "nn", "\xE3\x82\x93" },
    { "no", "\xE3\x81\xAE" },
    { "nu", "\xE3\x81\xAC" },
    { "nya", "\xE3\x81\xAB\xE3\x82\x83" },
    { "nye", "\xE3\x81\xAB\xE3\x81\x87" },
    { "nyo", "\xE3\x81\xAB\xE3\x82\x87" },
    { "nyu", "\xE3\x81\xAB\xE3\x82\x85" },
    { "o", "\xE3\x81\x8A" },
    { "pa", "\xE3\x81\xB1" },
    { "pe", "\xE3\x81\xBA" },
    { "pi", "\xE3\x81\xB4" },
    { "po", "\xE3\x81\xBD" },
    { "pu", "\xE3\x81\xB7" },
    { "pya", "\xE3\x81\xB4\xE3\x82\x83" },
    { "pye", "\xE3\x81\xB4\xE3\x81\x87" },
    { "pyo", "\xE3\x81\xB4\xE3\x82\x87" },
    { "pyu", "\xE3\x81\xB4\xE3\x82\x85" },
    { "qa", "\xE3\x81\x8F\xE3\x81\x81" },
    { "qe", "\xE3\x81\x8F\xE3\x81\x87" },
    { "qi", "\xE3\x81\x8F\xE3\x81\x83" },
    { "qo", "\xE3\x81\x8F\xE3\x81\x89" },
    { "ra", "\xE3\x82\x89" },
    { "re", "\xE3\x82\x8C" },
    { "ri", "\xE3\x82\x8A" },
    { "ro", "\xE3\x82\x8D" },
    { "ru", "\xE3\x82\x8B" },
    { "rya", "\xE3\x82\x8A\xE3\x82\x83" },
    { "rye", "\xE3\x82\x8A\xE3\x81\x87" },
    { "ryo", "\xE3\x82\x8A\xE3\x82\x87" },
    { "ryu", "\xE3\x82\x8A\xE3\x82\x85" },
    { "sa", "\xE3\x81\x95" },
    { "se", "\xE3\x81\x9B" },
    { "sha", "\xE3\x81\x97\xE3\x82\x83" },
    { "she", "\xE3\x81\x97\xE3\x81\x87" },
    { "shi", "\xE3\x81\x97" },
    { "sho", "\xE3\x81\x97\xE3\x82\x87" },
    { "shu", "\xE3\x81\x97\xE3\x82\x85" },
    { "si", "\xE3\x81\x97" },
    { "so", "\xE3\x81\x9D" },
    { "su", "\xE3\x81\x99" },
    { "sya", "\xE3\x81\x97\xE3\x82\x83" },
    { "sye", "\xE3\x81\x97\xE3\x81\x87" },
    { "syo", "\xE3\x81\x97\xE3\x82\x87" },
    { "syu", "\xE3\x81\x97\xE3\x82\x85" },
    { "ta", "\xE3\x81\x9F" },
    { "te", "\xE3\x81\xA6" },
    { "thi", "\xE3\x81\xA6\xE3\x81\x83" },
    { "thu", "\xE3\x81\xA6\xE3\x82\x85" },
    { "ti", "\xE3\x81\xA1" },
    { "to", "\xE3\x81\xA8" },
    { "tsa", "\xE3\x81\xA4\xE3\x81\x81" },
    { "tse", "\xE3\x81\xA4\xE3\x81\x87" },
    { "tsi", "\xE3\x81\xA4\xE3\x81\x83" },
    { "tso", "\xE3\x81\xA4\xE3\x81\x89" },
    { "tsu", "\xE3\x81\xA4" },
    { "tu", "\xE3\x81\xA4" },
    { "twu", "\xE3\x81\xA8\xE3\x81\x85" },
    { "tya", "\xE3\x81\xA1\xE3\x82\x83" },
    { "tye", "\xE3\x81\xA1\xE3\x81\x87" },
    { "tyo", "\xE3\x81\xA1\xE3\x82\x87" },
    { "tyu", "\xE3\x81\xA1\xE3\x82\x85" },
    { "u", "\xE3\x81\x86" },
    { "va", "\xE3\x82\x94\xE3\x81\x81" },
    { "ve", "\xE3\x82\x94\xE3\x81\x87" },
    { "vi", "\xE3\x82\x94\xE3\x81\x83" },
    { "vo", "\xE3\x82\x94\xE3\x81\x89" },
    { "vu", "\xE3\x82\x94" },
    { "wa", "\xE3\x82\x8F" },
    { "we", "\xE3\x81\x86\xE3\x81\x87" },
    { "wha", "\xE3\x81\x86\xE3\x81\x81" },
    { "whe", "\xE3\x81\x86\xE3\x81\x87" },
    { "whi", "\xE3\x81\x86\xE3\x81\x83" },
    { "who", "\xE3\x81\x86\xE3\x81\x89" },
    { "wi", "\xE3\x81\x86\xE3\x81\x83" },
    { "wo", "\xE3\x82\x92" },
    { "xa", "\xE3\x81\x81" },
    { "xe", "\xE3\x81\x87" },
    { "xi", "\xE3\x81\x83" },
    { "xka", "\xE3\x82\x95" },
    { "xke", "\xE3\x82\x96" },
    { "xn", "\xE3\x82\x93" },
    { "xo", "\xE3\x81\x89" },
    { "xtsu", "\xE3\x81\xA3" },
    { "xtu", "\xE3\x81\xA3" },
    { "xu", "\xE3\x81\x85" },
    { "xwa", "\xE3\x82\x8E" },
    { "xya", "\xE3\x82\x83" },
    { "xyo", "\xE3\x82\x87" },
    { "xyu", "\xE3\x82\x85" },
    { "ya", "\xE3\x82\x84" },
    { "ye", "\xE3\x81\x84\xE3\x81\x87" },
    { "yo", "\xE3\x82\x88" },
    { "yu", "\xE3\x82\x86" },
    { "za", "\xE3\x81\x96" },
    { "ze", "\xE3\x81\x9C" },
    { "zi", "\xE3\x81\x98" },
    { "zo", "\xE3\x81\x9E" },
    { "zu", "\xE3\x81\x9A" },
    { "zya", "\xE3\x81\x98\xE3\x82\x83" },
    { "zye", "\xE3\x81\x98\xE3\x81\x87" },
    { "zyo", "\xE3\x81\x98\xE3\x82\x87" },
    { "zyu", "\xE3\x81\x98\xE3\x82\x85" },
    { "~", "\xE3\x80\x9C" },
};

#define ROMAJI_COUNT (sizeof(ROMAJI) / sizeof(ROMAJI[0]))

#define KANA_MAX     180      /* bytes of converted kana */
#define ROMAJI_MAX   8
#define MAX_CHARS    60       /* kana characters in a reading */
#define MAX_WORD     12       /* longest dictionary reading, chars */
#ifndef WORD_PENALTY
#define WORD_PENALTY 4500     /* cost added per word: favours long words */
#endif
#define UNKNOWN_COST 12000    /* one kana not found in the dictionary */

static char s_kana[KANA_MAX + 1];      /* converted part */
static char s_romaji[ROMAJI_MAX + 1];  /* pending letters */
static bool s_active;

/* Conversion */
static bool s_conv;
static char s_reading[KANA_MAX + 1];   /* kana being converted */
static int  s_cut[MAX_CHARS + 1];      /* byte offset of each char */
static int  s_nchars;
static int  s_seg;                     /* chars in the current word */
static ime_cands_t s_cands;
static char s_status[256];

/* ---- Romaji ---- */

/* 1 = exact match, 2 = prefix of a longer entry, 3 = both, 0 = none. */
static int romaji_match(const char *p, const char **kana)
{
    size_t len = strlen(p);
    int lo = 0, hi = (int)ROMAJI_COUNT - 1, first = (int)ROMAJI_COUNT;
    while (lo <= hi) {          /* first entry >= p */
        int mid = (lo + hi) / 2;
        if (strcmp(ROMAJI[mid].romaji, p) >= 0) { first = mid; hi = mid - 1; }
        else lo = mid + 1;
    }
    int r = 0;
    for (size_t i = (size_t)first; i < ROMAJI_COUNT; i++) {
        if (strncmp(ROMAJI[i].romaji, p, len) != 0) break;
        if (strlen(ROMAJI[i].romaji) == len) {
            r |= 1;
            if (kana) *kana = ROMAJI[i].kana;
        } else {
            r |= 2;
        }
    }
    return r;
}

static bool kana_append(const char *s)
{
    size_t a = strlen(s_kana), b = strlen(s);
    if (a + b > KANA_MAX) return false;
    memcpy(s_kana + a, s, b + 1);
    return true;
}

static bool is_vowel_or_y(char c)
{
    return c == 'a' || c == 'i' || c == 'u' || c == 'e' || c == 'o' || c == 'y';
}

/* Convert as much of s_romaji as possible. With `flush`, also resolve
 * what would otherwise wait for the next letter. */
static void romaji_flush(bool flush)
{
    static const char SMALL_TSU[] = "\xE3\x81\xA3";
    static const char MORAIC_N[] = "\xE3\x82\x93";
    for (;;) {
        size_t len = strlen(s_romaji);
        if (len == 0) return;
        const char *kana = NULL;
        /* "nn" waits: before a vowel or y it is n + na/ni/..., otherwise
         * the moraic n. */
        if (s_romaji[0] == 'n' && len >= 2 && s_romaji[1] == 'n') {
            if (len == 2 && !flush) return;
            kana_append(MORAIC_N);
            if (len >= 3 && is_vowel_or_y(s_romaji[2])) {
                memmove(s_romaji, s_romaji + 1, len);       /* keep "n?" */
            } else {
                memmove(s_romaji, s_romaji + 2, len - 1);
            }
            continue;
        }
        int m = romaji_match(s_romaji, &kana);
        if (m == 1 || (m == 3 && flush)) {
            kana_append(kana);
            s_romaji[0] = '\0';
            return;
        }
        if (m & 2) {
            if (!flush) return;              /* wait for more letters */
            if (s_romaji[0] == 'n' && len == 1) {
                kana_append(MORAIC_N);
                s_romaji[0] = '\0';
                return;
            }
        }
        /* No entry starts with the pending letters. */
        if (s_romaji[0] == 'n' && len >= 2 && !is_vowel_or_y(s_romaji[1])) {
            kana_append(MORAIC_N);
            memmove(s_romaji, s_romaji + 1, len);
            continue;
        }
        if (len >= 2 && s_romaji[0] == s_romaji[1] && isalpha((unsigned char)s_romaji[0]) &&
            !is_vowel_or_y(s_romaji[0]) && s_romaji[0] != 'n') {
            kana_append(SMALL_TSU);
            memmove(s_romaji, s_romaji + 1, len);
            continue;
        }
        if (m == 0 || flush) {
            char lit[2] = { s_romaji[0], '\0' };
            kana_append(lit);
            memmove(s_romaji, s_romaji + 1, len);
            continue;
        }
        return;
    }
}

/* ---- Display ---- */

static bool show_composing(void)
{
    char buf[KANA_MAX + ROMAJI_MAX + 1];
    ime_join(buf, sizeof(buf), s_kana, s_romaji);
    return comp_show(buf);
}

static void reset(void)
{
    s_kana[0] = '\0';
    s_romaji[0] = '\0';
    s_active = false;
    s_conv = false;
    cands_clear(&s_cands);
}

/* ---- Conversion ---- */

static void to_katakana(const char *in, size_t len, char *out, size_t size)
{
    size_t i = 0, o = 0;
    while (i < len && o + 4 < size) {
        size_t n;
        uint32_t cp = utf8_decode(in + i, len - i, &n);
        if (n == 0) break;
        if (cp >= 0x3041 && cp <= 0x3096) cp += 0x60;
        o += utf8_encode(cp, out + o);
        i += n;
    }
    out[o] = '\0';
}

static void index_reading(void)
{
    s_nchars = 0;
    size_t i = 0, len = strlen(s_reading);
    while (i < len && s_nchars < MAX_CHARS) {
        s_cut[s_nchars++] = (int)i;
        size_t n;
        utf8_decode(s_reading + i, len - i, &n);
        if (n == 0) break;
        i += n;
    }
    s_cut[s_nchars] = (int)i;
    if (i < len) s_reading[i] = '\0';      /* too long: cut */
}

static bool word_cost(int from, int to, uint16_t *cost)
{
    return dict_lookup(&g_ja_dict, s_reading + s_cut[from],
                       (size_t)(s_cut[to] - s_cut[from]), cost, NULL, NULL);
}

/* Length in chars of the first word of the best split of the reading. */
static int best_first_word(void)
{
    uint32_t best[MAX_CHARS + 1];
    int back[MAX_CHARS + 1];
    for (int i = 0; i <= s_nchars; i++) { best[i] = UINT32_MAX; back[i] = -1; }
    best[0] = 0;
    for (int i = 0; i < s_nchars; i++) {
        if (best[i] == UINT32_MAX) continue;
        for (int j = i + 1; j <= s_nchars && j - i <= MAX_WORD; j++) {
            uint16_t c;
            if (!word_cost(i, j, &c)) continue;
            uint32_t v = best[i] + c + WORD_PENALTY;
            if (v < best[j]) { best[j] = v; back[j] = i; }
        }
        uint32_t v = best[i] + UNKNOWN_COST;
        if (v < best[i + 1]) { best[i + 1] = v; back[i + 1] = i; }
    }
    int j = s_nchars;
    while (j > 0 && back[j] > 0) j = back[j];
    return j > 0 ? j : 1;
}

static void build_candidates(void)
{
    cands_clear(&s_cands);
    const char *seg = s_reading;
    size_t seg_len = (size_t)s_cut[s_seg];
    uint8_t n = 0;
    const char *c = NULL;
    if (dict_lookup(&g_ja_dict, seg, seg_len, NULL, &n, &c)) {
        for (uint8_t i = 0; i < n; i++) {
            size_t l = strlen(c);
            cands_add(&s_cands, c, l, 0);
            c += l + 1;
        }
    }
    char kata[KANA_MAX + 1];
    cands_add(&s_cands, seg, seg_len, 0);
    to_katakana(seg, seg_len, kata, sizeof(kata));
    cands_add(&s_cands, kata, strlen(kata), 0);
    /* Without dictionary entries hiragana comes first; katakana is the
     * usual choice for an unknown word, so offer it first. */
    if (n == 0 && s_cands.count == 2) {
        ime_cand_t t = s_cands.c[0];
        s_cands.c[0] = s_cands.c[1];
        s_cands.c[1] = t;
    }
    cands_paginate(&s_cands, 0);
}

static bool show_conversion(void)
{
    char buf[IME_COMP_MAX];
    ime_join(buf, sizeof(buf), s_cands.c[s_cands.sel].text, s_reading + s_cut[s_seg]);
    return comp_show(buf);
}

/* Start converting the reading in s_reading (whole word search). */
static bool convert_start(int seg)
{
    index_reading();
    if (s_nchars == 0) { reset(); comp_forget(); return true; }
    s_conv = true;
    s_seg = seg > 0 ? seg : best_first_word();
    if (s_seg > s_nchars) s_seg = s_nchars;
    /* An unknown first word: take the whole reading (katakana words). */
    if (seg <= 0) {
        uint16_t c;
        if (!word_cost(0, s_seg, &c)) s_seg = s_nchars;
    }
    build_candidates();
    return show_conversion();
}

/* Accept the current candidate for the first word. Returns true when
 * reading remains. */
static bool accept_word(int idx)
{
    const char *done = s_cands.c[idx].text;
    char rest[KANA_MAX + 1];
    ime_copy(rest, sizeof(rest), s_reading + s_cut[s_seg]);
    /* Show the choice, then drop it from the composition. */
    char buf[IME_COMP_MAX];
    ime_join(buf, sizeof(buf), done, rest);
    if (comp_show(buf)) comp_drop_front(strlen(done));
    ime_copy(s_reading, sizeof(s_reading), rest);
    return rest[0] != '\0';
}

static void finish(void)
{
    reset();
    comp_forget();
}

static bool conv_key(uint8_t keycode, uint8_t modifier, const char *text)
{
    (void)modifier;
    switch (keycode) {
    case IME_KEY_SPACE:
    case IME_KEY_DOWN:
        s_cands.sel = (s_cands.sel + 1) % s_cands.count;
        return show_conversion();
    case IME_KEY_UP:
        s_cands.sel = (s_cands.sel + s_cands.count - 1) % s_cands.count;
        return show_conversion();
    case IME_KEY_PAGEDOWN: {
        int p = cands_page_of(&s_cands, s_cands.sel) + 1;
        if (p >= s_cands.pages) p = 0;
        s_cands.sel = s_cands.page_start[p];
        return show_conversion();
    }
    case IME_KEY_PAGEUP: {
        int p = cands_page_of(&s_cands, s_cands.sel) - 1;
        if (p < 0) p = s_cands.pages - 1;
        s_cands.sel = s_cands.page_start[p];
        return show_conversion();
    }
    case IME_KEY_LEFT:
        if (s_seg > 1) { s_seg--; build_candidates(); }
        return show_conversion();
    case IME_KEY_RIGHT:
        if (s_seg < s_nchars) { s_seg++; build_candidates(); }
        return show_conversion();
    case IME_KEY_ENTER:
    case IME_KEY_KP_ENTER:
        accept_word(s_cands.sel);
        finish();
        return true;
    case IME_KEY_ESCAPE:
    case IME_KEY_BACKSPACE:
        /* Back to the kana reading. */
        s_conv = false;
        ime_copy(s_kana, sizeof(s_kana), s_reading);
        s_romaji[0] = '\0';
        return show_composing();
    default:
        break;
    }
    if (text && text[0] >= '1' && text[0] <= '9' && !text[1]) {
        int idx = cands_digit(&s_cands, text[0] - '0');
        if (idx < 0) return true;
        if (accept_word(idx)) return convert_start(0);
        finish();
        return true;
    }
    /* Anything else accepts the whole conversion; letters then start a
     * new reading (the caller passes the key on). */
    accept_word(s_cands.sel);
    finish();
    return false;
}

static bool kana_key(uint8_t keycode, uint8_t modifier, const char *text)
{
    if (s_active && s_conv) {
        if (conv_key(keycode, modifier, text)) return true;
        /* The conversion was accepted; fall through for this key. */
    }

    char c = (text && text[0] && !text[1]) ? text[0] : 0;
    bool letter = isalpha((unsigned char)c) != 0;

    /* Japanese punctuation ends the reading (convert with Space first):
     * , . [ ] / ~ type the comma, full stop, corner brackets, middle
     * dot and wave dash. */
    if (c == ',' || c == '.' || c == '[' || c == ']' || c == '/' || c == '~') {
        char key[2] = { c, '\0' };
        const char *kana = NULL;
        romaji_match(key, &kana);
        if (s_active) {
            romaji_flush(true);
            show_composing();
            finish();
        }
        return kana ? ime_replace(0, kana) : false;
    }

    bool reading_char = letter || c == '-' ||
                        (c == '\'' && strcmp(s_romaji, "n") == 0);
    if (!s_active) {
        if (!letter && c != '-') return false;
        reset();
        s_active = true;
        comp_forget();
    }

    if (reading_char) {
        if (strlen(s_kana) + 12 > KANA_MAX) {
            /* Too long to convert as one reading: keep it as kana. */
            romaji_flush(true);
            show_composing();
            finish();
            s_active = true;
        }
        size_t rl = strlen(s_romaji);
        if (rl >= ROMAJI_MAX) {
            romaji_flush(true);
            rl = strlen(s_romaji);
        }
        s_romaji[rl] = (char)tolower((unsigned char)c);
        s_romaji[rl + 1] = '\0';
        romaji_flush(false);
        return show_composing();
    }

    switch (keycode) {
    case IME_KEY_SPACE:
        romaji_flush(true);
        ime_copy(s_reading, sizeof(s_reading), s_kana);
        if (!s_reading[0]) { finish(); return false; }
        return convert_start(0);
    case IME_KEY_ENTER:
    case IME_KEY_KP_ENTER:
    case IME_KEY_ESCAPE:
        romaji_flush(true);
        show_composing();
        finish();
        return true;
    case IME_KEY_BACKSPACE: {
        size_t rl = strlen(s_romaji);
        if (rl > 0) {
            s_romaji[rl - 1] = '\0';
        } else {
            size_t kl = strlen(s_kana);
            if (kl > 0) s_kana[kl - utf8_last_len(s_kana, kl)] = '\0';
        }
        show_composing();
        if (!s_kana[0] && !s_romaji[0]) finish();
        return true;
    }
    default:
        break;
    }
    /* Any other key: keep the kana and let the editor handle the key. */
    romaji_flush(true);
    show_composing();
    finish();
    return false;
}

static void kana_commit(void)
{
    if (s_active && comp_valid()) {
        if (s_conv) {
            accept_word(s_cands.sel);
        } else {
            romaji_flush(true);
            show_composing();
        }
    }
    reset();
}

static bool kana_composing(void)
{
    return s_active;
}

static const char *kana_status(void)
{
    if (!s_active) return NULL;
    if (!s_conv) return "Space: kanji  Enter: keep kana";
    cands_format(&s_cands, NULL, s_status, sizeof(s_status));
    return s_status[0] == ' ' ? s_status + 1 : s_status;
}

const ime_engine_t ime_kana_engine = {
    kana_key, kana_commit, kana_composing, kana_status,
};
