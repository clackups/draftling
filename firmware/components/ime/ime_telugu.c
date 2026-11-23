/*
 * Telugu phonetic and Apple input methods, after the m17n-db input
 * methods "te-rts", "te-pothana" and "te-apple". All three type on the
 * US keyboard (kb_layout.cpp: TE-RT, TE-PO, TE-AP).
 *
 * RTS (Rice Transliteration Standard, with the RIT supplement): a word
 * is typed in Latin letters and shown in Telugu as it is typed. The
 * Latin letters of the word are kept (s_rom) and the whole word is
 * transliterated again after every key, longest match first, so `k`
 * shows k-virama and `kh` then turns it into kha-virama. A consonant is
 * shown with the virama until a vowel follows (`ka`, `kaa`, `ki`).
 * Sunna (anusvara): n or m before another consonant (except r, y) is
 * the anusvara, and so is m at the end of a word (when Space or
 * punctuation ends it); `n&` / `m&` keep the letter. Backspace removes
 * the last Latin letter while the word is open. Digits type Telugu
 * digits, `|` the danda, `||` the double danda.
 *
 * Pothana: one key per letter, consonants with the inherent a
 * (k = ka, h = virama); a vowel key right after a consonant types the
 * vowel sign, anywhere else the independent vowel; `a` after a
 * consonant just ends the syllable.
 *
 * Apple: one key per letter as on Apple's Telugu keyboard; a vowel key
 * after a consonant types the sign; `q` after a consonant ends the
 * syllable. A conjunct can also be typed the Apple way, with the vowel
 * before the halant: consonant, vowel, halant, consonant gives
 * consonant-virama-consonant-vowel.
 *
 * Shift + Space types the zero-width non-joiner in all three.
 */

#include <string.h>
#include "ime_internal.h"

#define TE_VIRAMA "\xE0\xB1\x8D"
#define TE_ANUSVARA "\xE0\xB0\x82"
#define ZWNJ "\xE2\x80\x8C"

/* The single ASCII character of a key's text, or 0. */
static char key_char(const char *text)
{
    if (!text || !text[0] || text[1] || (unsigned char)text[0] >= 0x80) return 0;
    return text[0];
}

static bool is_shift_space(uint8_t keycode, uint8_t modifier)
{
    return keycode == IME_KEY_SPACE && (modifier & IME_MOD_SHIFT);
}

/* ================================================================== */
/* RTS                                                                */
/* ================================================================== */

enum { R_CONS = 1, R_NOSUNNA, R_SUNNA, R_VOWEL, R_SIGN_ONLY, R_TEXT };

typedef struct {
    const char *rom;
    uint8_t kind;
    const char *text;       /* consonant (no virama), independent vowel, text */
    const char *sign;       /* R_VOWEL: the vowel sign ("" for a) */
} rts_tok_t;

static const rts_tok_t RTS[] = {
    /* Consonants: velar. */
    { "k", R_CONS, "\xE0\xB0\x95", NULL },  /* ka */
    { "kh", R_CONS, "\xE0\xB0\x96", NULL }, { "kH", R_CONS, "\xE0\xB0\x96", NULL },  /* kha, kha */
    { "K", R_CONS, "\xE0\xB0\x96", NULL }, { "Kh", R_CONS, "\xE0\xB0\x96", NULL }, { "KH", R_CONS, "\xE0\xB0\x96", NULL },  /* kha, kha, kha */
    { "g", R_CONS, "\xE0\xB0\x97", NULL },  /* ga */
    { "gh", R_CONS, "\xE0\xB0\x98", NULL }, { "gH", R_CONS, "\xE0\xB0\x98", NULL },  /* gha, gha */
    { "G", R_CONS, "\xE0\xB0\x98", NULL }, { "Gh", R_CONS, "\xE0\xB0\x98", NULL }, { "GH", R_CONS, "\xE0\xB0\x98", NULL },  /* gha, gha, gha */
    { "~m", R_CONS, "\xE0\xB0\x99", NULL },  /* nga */
    /* Palatal. */
    { "c", R_CONS, "\xE0\xB0\x9A", NULL }, { "ch", R_CONS, "\xE0\xB0\x9A", NULL }, { "cH", R_CONS, "\xE0\xB0\x9A", NULL },  /* ca, ca, ca */
    { "~c", R_CONS, "\xE0\xB1\x98", NULL },  /* tsa */
    { "C", R_CONS, "\xE0\xB0\x9B", NULL }, { "Ch", R_CONS, "\xE0\xB0\x9B", NULL }, { "CH", R_CONS, "\xE0\xB0\x9B", NULL },  /* cha, cha, cha */
    { "c'", R_CONS, "\xE0\xB0\x9B", NULL },  /* cha */
    { "j", R_CONS, "\xE0\xB0\x9C", NULL }, { "z", R_CONS, "\xE0\xB0\x9C", NULL },  /* ja, ja */
    { "~j", R_CONS, "\xE0\xB1\x99", NULL },  /* dza */
    { "jh", R_CONS, "\xE0\xB0\x9D", NULL }, { "jH", R_CONS, "\xE0\xB0\x9D", NULL },  /* jha, jha */
    { "J", R_CONS, "\xE0\xB0\x9D", NULL }, { "Jh", R_CONS, "\xE0\xB0\x9D", NULL }, { "JH", R_CONS, "\xE0\xB0\x9D", NULL },  /* jha, jha, jha */
    { "~n", R_CONS, "\xE0\xB0\x9E", NULL },  /* nya */
    /* Retroflex. */
    { "T", R_CONS, "\xE0\xB0\x9F", NULL }, { "t'", R_CONS, "\xE0\xB0\x9F", NULL },  /* tta, tta */
    { "Th", R_CONS, "\xE0\xB0\xA0", NULL }, { "TH", R_CONS, "\xE0\xB0\xA0", NULL },  /* ttha, ttha */
    { "th'", R_CONS, "\xE0\xB0\xA0", NULL }, { "tH'", R_CONS, "\xE0\xB0\xA0", NULL },  /* ttha, ttha */
    { "D", R_CONS, "\xE0\xB0\xA1", NULL }, { "d'", R_CONS, "\xE0\xB0\xA1", NULL },  /* dda, dda */
    { "Dh", R_CONS, "\xE0\xB0\xA2", NULL }, { "DH", R_CONS, "\xE0\xB0\xA2", NULL },  /* ddha, ddha */
    { "dh'", R_CONS, "\xE0\xB0\xA2", NULL }, { "dH'", R_CONS, "\xE0\xB0\xA2", NULL },  /* ddha, ddha */
    { "N", R_CONS, "\xE0\xB0\xA3", NULL }, { "nh", R_CONS, "\xE0\xB0\xA3", NULL },  /* nna, nna */
    { "nH", R_CONS, "\xE0\xB0\xA3", NULL }, { "n'", R_CONS, "\xE0\xB0\xA3", NULL },  /* nna, nna */
    /* Dental. */
    { "t", R_CONS, "\xE0\xB0\xA4", NULL }, { "th", R_CONS, "\xE0\xB0\xA5", NULL }, { "tH", R_CONS, "\xE0\xB0\xA5", NULL },  /* ta, tha, tha */
    { "d", R_CONS, "\xE0\xB0\xA6", NULL }, { "dh", R_CONS, "\xE0\xB0\xA7", NULL }, { "dH", R_CONS, "\xE0\xB0\xA7", NULL },  /* da, dha, dha */
    /* Labial. */
    { "p", R_CONS, "\xE0\xB0\xAA", NULL },  /* pa */
    { "f", R_CONS, "\xE0\xB0\xAB", NULL }, { "P", R_CONS, "\xE0\xB0\xAB", NULL }, { "ph", R_CONS, "\xE0\xB0\xAB", NULL },  /* pha, pha, pha */
    { "pH", R_CONS, "\xE0\xB0\xAB", NULL }, { "Ph", R_CONS, "\xE0\xB0\xAB", NULL }, { "PH", R_CONS, "\xE0\xB0\xAB", NULL },  /* pha, pha, pha */
    { "b", R_CONS, "\xE0\xB0\xAC", NULL },  /* ba */
    { "bh", R_CONS, "\xE0\xB0\xAD", NULL }, { "bH", R_CONS, "\xE0\xB0\xAD", NULL },  /* bha, bha */
    { "B", R_CONS, "\xE0\xB0\xAD", NULL }, { "Bh", R_CONS, "\xE0\xB0\xAD", NULL }, { "BH", R_CONS, "\xE0\xB0\xAD", NULL },  /* bha, bha, bha */
    /* Semivowels, sibilants, others. */
    { "l", R_CONS, "\xE0\xB0\xB2", NULL },  /* la */
    { "v", R_CONS, "\xE0\xB0\xB5", NULL }, { "V", R_CONS, "\xE0\xB0\xB5", NULL },  /* va, va */
    { "w", R_CONS, "\xE0\xB0\xB5", NULL }, { "W", R_CONS, "\xE0\xB0\xB5", NULL },  /* va, va */
    { "S", R_CONS, "\xE0\xB0\xB6", NULL }, { "s'", R_CONS, "\xE0\xB0\xB6", NULL },  /* sha, sha */
    { "sh", R_CONS, "\xE0\xB0\xB7", NULL }, { "sH", R_CONS, "\xE0\xB0\xB7", NULL },  /* ssa, ssa */
    { "Sh", R_CONS, "\xE0\xB0\xB7", NULL }, { "SH", R_CONS, "\xE0\xB0\xB7", NULL },  /* ssa, ssa */
    { "s", R_CONS, "\xE0\xB0\xB8", NULL },  /* sa */
    { "h", R_CONS, "\xE0\xB0\xB9", NULL }, { "H", R_CONS, "\xE0\xB0\xB9", NULL },  /* ha, ha */
    { "L", R_CONS, "\xE0\xB0\xB3", NULL }, { "lh", R_CONS, "\xE0\xB0\xB3", NULL }, { "lH", R_CONS, "\xE0\xB0\xB3", NULL },  /* lla, lla, lla */
    { "Lh", R_CONS, "\xE0\xB0\xB3", NULL }, { "LH", R_CONS, "\xE0\xB0\xB3", NULL }, { "l'", R_CONS, "\xE0\xB0\xB3", NULL },  /* lla, lla, lla */
    { "x", R_CONS, "\xE0\xB0\x95\xE0\xB1\x8D\xE0\xB0\xB7", NULL }, { "ksh", R_CONS, "\xE0\xB0\x95\xE0\xB1\x8D\xE0\xB0\xB7", NULL },  /* ka-virama-ssa, ka-virama-ssa */
    { "ksH", R_CONS, "\xE0\xB0\x95\xE0\xB1\x8D\xE0\xB0\xB7", NULL }, { "ks", R_CONS, "\xE0\xB0\x95\xE0\xB1\x8D\xE0\xB0\xB8", NULL },  /* ka-virama-ssa, ka-virama-sa */
    { "~r", R_CONS, "\xE0\xB0\xB1", NULL }, { "r''", R_CONS, "\xE0\xB0\xB1", NULL },  /* rra, rra */
    /* Compounds. */
    { "jn", R_CONS, "\xE0\xB0\x9C\xE0\xB1\x8D\xE0\xB0\x9E", NULL }, { "j&n", R_CONS, "\xE0\xB0\x9C\xE0\xB1\x8D\xE0\xB0\xA8", NULL },  /* ja-virama-nya, ja-virama-na */
    { "dd'", R_CONS, "\xE0\xB0\xA1\xE0\xB1\x8D\xE0\xB0\xA1", NULL }, { "dd", R_CONS, "\xE0\xB0\xA6\xE0\xB1\x8D\xE0\xB0\xA6", NULL },  /* dda-virama-dda, da-virama-da */
    { "tt'", R_CONS, "\xE0\xB0\x9F\xE0\xB1\x8D\xE0\xB0\x9F", NULL }, { "tt", R_CONS, "\xE0\xB0\xA4\xE0\xB1\x8D\xE0\xB0\xA4", NULL },  /* tta-virama-tta, ta-virama-ta */
    /* Consonants that never turn a preceding n / m into the anusvara. */
    { "r", R_NOSUNNA, "\xE0\xB0\xB0", NULL }, { "y", R_NOSUNNA, "\xE0\xB0\xAF", NULL },  /* ra, ya */
    { "n&", R_NOSUNNA, "\xE0\xB0\xA8", NULL }, { "m&", R_NOSUNNA, "\xE0\xB0\xAE", NULL },  /* na, ma */
    /* n and m: the anusvara before another consonant. */
    { "n", R_SUNNA, "\xE0\xB0\xA8", NULL }, { "m", R_SUNNA, "\xE0\xB0\xAE", NULL },  /* na, ma */
    /* Vowels: independent letter, sign. */
    { "a", R_VOWEL, "\xE0\xB0\x85", "" },  /* a */
    { "aa", R_VOWEL, "\xE0\xB0\x86", "\xE0\xB0\xBE" }, { "a'", R_VOWEL, "\xE0\xB0\x86", "\xE0\xB0\xBE" }, { "A", R_VOWEL, "\xE0\xB0\x86", "\xE0\xB0\xBE" },  /* aa, aa, aa */
    { "i", R_VOWEL, "\xE0\xB0\x87", "\xE0\xB0\xBF" },  /* i */
    { "ee", R_VOWEL, "\xE0\xB0\x88", "\xE0\xB1\x80" }, { "ii", R_VOWEL, "\xE0\xB0\x88", "\xE0\xB1\x80" }, { "ia", R_VOWEL, "\xE0\xB0\x88", "\xE0\xB1\x80" },  /* ii, ii, ii */
    { "i'", R_VOWEL, "\xE0\xB0\x88", "\xE0\xB1\x80" }, { "I", R_VOWEL, "\xE0\xB0\x88", "\xE0\xB1\x80" },  /* ii, ii */
    { "u", R_VOWEL, "\xE0\xB0\x89", "\xE0\xB1\x81" },  /* u */
    { "oo", R_VOWEL, "\xE0\xB0\x8A", "\xE0\xB1\x82" }, { "uu", R_VOWEL, "\xE0\xB0\x8A", "\xE0\xB1\x82" }, { "U", R_VOWEL, "\xE0\xB0\x8A", "\xE0\xB1\x82" },  /* uu, uu, uu */
    { "ua", R_VOWEL, "\xE0\xB0\x8A", "\xE0\xB1\x82" }, { "u'", R_VOWEL, "\xE0\xB0\x8A", "\xE0\xB1\x82" },  /* uu, uu */
    { "R", R_VOWEL, "\xE0\xB0\x8B", "\xE0\xB1\x83" }, { "r'", R_VOWEL, "\xE0\xB0\x8B", "\xE0\xB1\x83" },  /* vocalic r, vocalic r */
    { "Ru", R_VOWEL, "\xE0\xB1\xA0", "\xE0\xB1\x84" }, { "r'u", R_VOWEL, "\xE0\xB1\xA0", "\xE0\xB1\x84" },  /* vocalic rr, vocalic rr */
    { "~l", R_VOWEL, "\xE0\xB0\x8C", "\xE0\xB1\xA2" }, { "~L", R_VOWEL, "\xE0\xB1\xA1", "\xE0\xB1\xA3" },  /* vocalic l, vocalic ll */
    { "e", R_VOWEL, "\xE0\xB0\x8E", "\xE0\xB1\x86" },  /* e */
    { "ea", R_VOWEL, "\xE0\xB0\x8F", "\xE0\xB1\x87" }, { "ae", R_VOWEL, "\xE0\xB0\x8F", "\xE0\xB1\x87" }, { "E", R_VOWEL, "\xE0\xB0\x8F", "\xE0\xB1\x87" },  /* ee, ee, ee */
    { "e'", R_VOWEL, "\xE0\xB0\x8F", "\xE0\xB1\x87" },  /* ee */
    { "ai", R_VOWEL, "\xE0\xB0\x90", "\xE0\xB1\x88" }, { "ei", R_VOWEL, "\xE0\xB0\x90", "\xE0\xB1\x88" },  /* ai, ai */
    { "o", R_VOWEL, "\xE0\xB0\x92", "\xE0\xB1\x8A" },  /* o */
    { "oe", R_VOWEL, "\xE0\xB0\x93", "\xE0\xB1\x8B" }, { "O", R_VOWEL, "\xE0\xB0\x93", "\xE0\xB1\x8B" }, { "oa", R_VOWEL, "\xE0\xB0\x93", "\xE0\xB1\x8B" },  /* oo, oo, oo */
    { "o'", R_VOWEL, "\xE0\xB0\x93", "\xE0\xB1\x8B" },  /* oo */
    { "au", R_VOWEL, "\xE0\xB0\x94", "\xE0\xB1\x8C" }, { "ou", R_VOWEL, "\xE0\xB0\x94", "\xE0\xB1\x8C" }, { "ow", R_VOWEL, "\xE0\xB0\x94", "\xE0\xB1\x8C" },  /* au, au, au */
    /* After a consonant: virama + ZWNJ (the explicit half form). */
    { "^", R_SIGN_ONLY, NULL, "\xE0\xB1\x8D\xE2\x80\x8C" },  /* virama-zero width non-joiner */
    /* Signs, digits, dandas. */
    { "M", R_TEXT, "\xE0\xB0\x82", NULL },  /* anusvara */
    { "@M", R_TEXT, "\xE0\xB0\x81", NULL }, { "@m", R_TEXT, "\xE0\xB0\x81", NULL },  /* candrabindu, candrabindu */
    { "@h", R_TEXT, "\xE0\xB0\x83", NULL }, { "@2", R_TEXT, "\xE0\xB0\xBD", NULL },  /* visarga, avagraha */
    { "0", R_TEXT, "\xE0\xB1\xA6", NULL }, { "1", R_TEXT, "\xE0\xB1\xA7", NULL }, { "2", R_TEXT, "\xE0\xB1\xA8", NULL },  /* zero, one, two */
    { "3", R_TEXT, "\xE0\xB1\xA9", NULL }, { "4", R_TEXT, "\xE0\xB1\xAA", NULL }, { "5", R_TEXT, "\xE0\xB1\xAB", NULL },  /* three, four, five */
    { "6", R_TEXT, "\xE0\xB1\xAC", NULL }, { "7", R_TEXT, "\xE0\xB1\xAD", NULL }, { "8", R_TEXT, "\xE0\xB1\xAE", NULL },  /* six, seven, eight */
    { "9", R_TEXT, "\xE0\xB1\xAF", NULL },  /* nine */
    { "|", R_TEXT, "\xE0\xA5\xA4", NULL }, { "||", R_TEXT, "\xE0\xA5\xA5", NULL },  /* danda, double danda */
};

#define RTS_COUNT (sizeof(RTS) / sizeof(RTS[0]))
#define RTS_ROM_MAX 40

static char s_rom[RTS_ROM_MAX + 1];
static size_t s_rom_len;
static char s_rts_status[RTS_ROM_MAX + 8];

/* Keys that belong to an RTS word. */
static bool rts_word_char(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
           c == '~' || c == '\'' || c == '@' || c == '|' || c == '&' || c == '^';
}

/* The longest token at rom[i..len), or NULL. */
static const rts_tok_t *rts_match(const char *rom, size_t i, size_t len, size_t *tlen)
{
    const rts_tok_t *best = NULL;
    size_t best_len = 0;
    for (size_t k = 0; k < RTS_COUNT; k++) {
        size_t l = strlen(RTS[k].rom);
        if (l > best_len && i + l <= len && memcmp(rom + i, RTS[k].rom, l) == 0) {
            best = &RTS[k];
            best_len = l;
        }
    }
    *tlen = best_len;
    return best;
}

static void app(char *out, size_t size, size_t *pos, const char *s)
{
    size_t l = strlen(s);
    if (*pos + l >= size) return;
    memcpy(out + *pos, s, l);
    *pos += l;
    out[*pos] = '\0';
}

/* Transliterate rom[0..len) into out. word_end: the word is finished
 * (Space or punctuation follows), so a final m is the anusvara. */
static void rts_translit(const char *rom, size_t len, bool word_end, char *out, size_t size)
{
    size_t pos = 0;
    out[0] = '\0';
    bool after_cons = false;     /* out ends in consonant + virama */
    size_t sunna_at = (size_t)-1; /* out offset of a pending n / m */
    bool sunna_m = false;
    size_t i = 0;
    while (i < len) {
        size_t tl;
        const rts_tok_t *t = rts_match(rom, i, len, &tl);
        bool pending = sunna_at != (size_t)-1;
        if (t && t->kind == R_CONS && pending) {
            /* n / m before a consonant: the anusvara. */
            pos = sunna_at;
            out[pos] = '\0';
            app(out, size, &pos, TE_ANUSVARA);
        } else if (pending && sunna_m && (!t || t->kind == R_TEXT)) {
            /* m before punctuation-like text ends the word. */
            pos = sunna_at;
            out[pos] = '\0';
            app(out, size, &pos, TE_ANUSVARA);
            after_cons = false;
        }
        sunna_at = (size_t)-1;
        if (!t) {
            char c[2] = { rom[i], '\0' };
            app(out, size, &pos, c);
            after_cons = false;
            i++;
            continue;
        }
        switch (t->kind) {
        case R_CONS:
        case R_NOSUNNA:
        case R_SUNNA:
            if (t->kind == R_SUNNA) {
                sunna_at = pos;
                sunna_m = t->rom[0] == 'm';
            }
            app(out, size, &pos, t->text);
            app(out, size, &pos, TE_VIRAMA);
            after_cons = true;
            break;
        case R_VOWEL:
            if (after_cons) {
                pos -= strlen(TE_VIRAMA);
                out[pos] = '\0';
                app(out, size, &pos, t->sign);
            } else {
                app(out, size, &pos, t->text);
            }
            after_cons = false;
            break;
        case R_SIGN_ONLY:
            if (after_cons) {
                pos -= strlen(TE_VIRAMA);
                out[pos] = '\0';
                app(out, size, &pos, t->sign);
            } else {
                app(out, size, &pos, t->rom);
            }
            after_cons = false;
            break;
        default:
            app(out, size, &pos, t->text);
            after_cons = false;
            break;
        }
        i += tl;
    }
    if (word_end && sunna_at != (size_t)-1 && sunna_m) {
        pos = sunna_at;
        out[pos] = '\0';
        app(out, size, &pos, TE_ANUSVARA);
    }
}

static void rts_show(bool word_end)
{
    char out[IME_COMP_MAX];
    rts_translit(s_rom, s_rom_len, word_end, out, sizeof(out));
    comp_show(out);
}

static void rts_reset(void)
{
    s_rom_len = 0;
    s_rom[0] = '\0';
}

/* End the word: final rules, keep the text. */
static void rts_finish(bool word_end)
{
    if (s_rom_len) rts_show(word_end);
    rts_reset();
    comp_forget();
}

static bool rts_key(uint8_t keycode, uint8_t modifier, const char *text)
{
    if (is_shift_space(keycode, modifier)) {
        rts_finish(false);
        ime_replace(0, ZWNJ);
        return true;
    }
    if (keycode == IME_KEY_BACKSPACE && s_rom_len) {
        s_rom[--s_rom_len] = '\0';
        rts_show(false);
        if (s_rom_len == 0) rts_finish(false);
        return true;
    }
    char c = key_char(text);
    if (c && rts_word_char(c)) {
        if (s_rom_len >= RTS_ROM_MAX) rts_finish(false);
        s_rom[s_rom_len++] = c;
        s_rom[s_rom_len] = '\0';
        rts_show(false);
        return true;
    }
    /* Space, punctuation, Enter, Tab end the word (a final m becomes
     * the anusvara); other keys leave it as it is. */
    bool word_end = c != 0 || keycode == IME_KEY_ENTER || keycode == IME_KEY_TAB ||
                    keycode == IME_KEY_KP_ENTER;
    rts_finish(word_end);
    return false;
}

static void rts_commit(void)
{
    rts_finish(false);
}

static bool rts_composing(void)
{
    return s_rom_len > 0;
}

static const char *rts_status(void)
{
    if (!s_rom_len) return NULL;
    ime_join(s_rts_status, sizeof(s_rts_status), "RTS: ", s_rom);
    return s_rts_status;
}

const ime_engine_t ime_telugu_rts_engine = {
    rts_key, rts_commit, rts_composing, rts_status,
};

/* ================================================================== */
/* Pothana and Apple: one key per letter                              */
/* ================================================================== */

enum { K_CONS = 1, K_VOWEL, K_HALANT, K_TEXT };

typedef struct {
    char key;
    uint8_t kind;
    const char *text;       /* consonant, independent vowel, text */
    const char *sign;       /* K_VOWEL: the vowel sign ("" for a) */
} te_key_t;

static const te_key_t POTHANA[] = {
    { 'k', K_CONS, "\xE0\xB0\x95", NULL }, { 'K', K_CONS, "\xE0\xB0\x96", NULL },  /* ka, kha */
    { 'g', K_CONS, "\xE0\xB0\x97", NULL }, { 'G', K_CONS, "\xE0\xB0\x98", NULL }, { 'M', K_CONS, "\xE0\xB0\x99", NULL },  /* ga, gha, nga */
    { 'c', K_CONS, "\xE0\xB0\x9A", NULL }, { 'C', K_CONS, "\xE0\xB0\x9B", NULL },  /* ca, cha */
    { 'j', K_CONS, "\xE0\xB0\x9C", NULL }, { 'J', K_CONS, "\xE0\xB0\x9D", NULL }, { 'V', K_CONS, "\xE0\xB0\x9E", NULL },  /* ja, jha, nya */
    { 't', K_CONS, "\xE0\xB0\x9F", NULL }, { 'T', K_CONS, "\xE0\xB0\xA0", NULL },  /* tta, ttha */
    { 'z', K_CONS, "\xE0\xB0\xA1", NULL }, { 'Z', K_CONS, "\xE0\xB0\xA2", NULL }, { 'N', K_CONS, "\xE0\xB0\xA3", NULL },  /* dda, ddha, nna */
    { 'w', K_CONS, "\xE0\xB0\xA4", NULL }, { 'W', K_CONS, "\xE0\xB0\xA5", NULL },  /* ta, tha */
    { 'd', K_CONS, "\xE0\xB0\xA6", NULL }, { 'D', K_CONS, "\xE0\xB0\xA7", NULL }, { 'n', K_CONS, "\xE0\xB0\xA8", NULL },  /* da, dha, na */
    { 'p', K_CONS, "\xE0\xB0\xAA", NULL }, { 'P', K_CONS, "\xE0\xB0\xAB", NULL },  /* pa, pha */
    { 'b', K_CONS, "\xE0\xB0\xAC", NULL }, { 'B', K_CONS, "\xE0\xB0\xAD", NULL }, { 'm', K_CONS, "\xE0\xB0\xAE", NULL },  /* ba, bha, ma */
    { 'y', K_CONS, "\xE0\xB0\xAF", NULL }, { 'r', K_CONS, "\xE0\xB0\xB0", NULL }, { 'R', K_CONS, "\xE0\xB0\xB1", NULL },  /* ya, ra, rra */
    { 'l', K_CONS, "\xE0\xB0\xB2", NULL }, { 'L', K_CONS, "\xE0\xB0\xB3", NULL }, { 'v', K_CONS, "\xE0\xB0\xB5", NULL },  /* la, lla, va */
    { 'S', K_CONS, "\xE0\xB0\xB6", NULL }, { 'x', K_CONS, "\xE0\xB0\xB7", NULL }, { 's', K_CONS, "\xE0\xB0\xB8", NULL },  /* sha, ssa, sa */
    { 'H', K_CONS, "\xE0\xB0\xB9", NULL }, { 'X', K_CONS, "\xE0\xB0\x95\xE0\xB1\x8D\xE0\xB0\xB7", NULL },  /* ha, ka-virama-ssa */
    { 'h', K_HALANT, "\xE0\xB1\x8D", NULL },  /* virama */
    { 'a', K_VOWEL, "\xE0\xB0\x85", "" }, { 'A', K_VOWEL, "\xE0\xB0\x86", "\xE0\xB0\xBE" },  /* a, aa */
    { 'i', K_VOWEL, "\xE0\xB0\x87", "\xE0\xB0\xBF" }, { 'I', K_VOWEL, "\xE0\xB0\x88", "\xE0\xB1\x80" },  /* i, ii */
    { 'u', K_VOWEL, "\xE0\xB0\x89", "\xE0\xB1\x81" }, { 'U', K_VOWEL, "\xE0\xB0\x8A", "\xE0\xB1\x82" },  /* u, uu */
    { 'q', K_VOWEL, "\xE0\xB0\x8B", "\xE0\xB1\x83" }, { 'Q', K_VOWEL, "\xE0\xB1\xA0", "\xE0\xB1\x84" },  /* vocalic r, vocalic rr */
    { 'e', K_VOWEL, "\xE0\xB0\x8E", "\xE0\xB1\x86" }, { 'E', K_VOWEL, "\xE0\xB0\x8F", "\xE0\xB1\x87" }, { 'Y', K_VOWEL, "\xE0\xB0\x90", "\xE0\xB1\x88" },  /* e, ee, ai */
    { 'o', K_VOWEL, "\xE0\xB0\x92", "\xE0\xB1\x8A" }, { 'O', K_VOWEL, "\xE0\xB0\x93", "\xE0\xB1\x8B" }, { '[', K_VOWEL, "\xE0\xB0\x94", "\xE0\xB1\x8C" },  /* o, oo, au */
    { '{', K_TEXT, "\xE0\xB0\x8C", NULL }, { '}', K_TEXT, "\xE0\xB1\xA1", NULL },  /* vocalic l */
    { 'f', K_TEXT, "\xE0\xB0\x82", NULL }, { 'F', K_TEXT, "\xE0\xB0\x81", NULL }, { ']', K_TEXT, "\xE0\xB0\x83", NULL },  /* anusvara, candrabindu, visarga */
};

static const te_key_t APPLE[] = {
    { 'j', K_CONS, "\xE0\xB0\x95", NULL }, { 'J', K_CONS, "\xE0\xB0\x96", NULL },  /* ka, kha */
    { 'x', K_CONS, "\xE0\xB0\x97", NULL }, { 'X', K_CONS, "\xE0\xB0\x98", NULL }, { 'R', K_CONS, "\xE0\xB0\x99", NULL },  /* ga, gha, nga */
    { 'm', K_CONS, "\xE0\xB0\x9A", NULL }, { 'M', K_CONS, "\xE0\xB0\x9B", NULL },  /* ca, cha */
    { '/', K_CONS, "\xE0\xB0\x9C", NULL }, { '?', K_CONS, "\xE0\xB0\x9D", NULL }, { 'T', K_CONS, "\xE0\xB0\x9E", NULL },  /* ja, jha, nya */
    { 'z', K_CONS, "\xE0\xB0\x9F", NULL }, { 'Z', K_CONS, "\xE0\xB0\xA0", NULL },  /* tta, ttha */
    { 'c', K_CONS, "\xE0\xB0\xA1", NULL }, { 'C', K_CONS, "\xE0\xB0\xA2", NULL }, { 'L', K_CONS, "\xE0\xB0\xA3", NULL },  /* dda, ddha, nna */
    { 's', K_CONS, "\xE0\xB0\xA4", NULL }, { 'S', K_CONS, "\xE0\xB0\xA5", NULL },  /* ta, tha */
    { 'd', K_CONS, "\xE0\xB0\xA6", NULL }, { 'D', K_CONS, "\xE0\xB0\xA7", NULL }, { 'l', K_CONS, "\xE0\xB0\xA8", NULL },  /* da, dha, na */
    { ';', K_CONS, "\xE0\xB0\xAA", NULL }, { ':', K_CONS, "\xE0\xB0\xAB", NULL },  /* pa, pha */
    { 'v', K_CONS, "\xE0\xB0\xAC", NULL }, { 'V', K_CONS, "\xE0\xB0\xAD", NULL }, { 'b', K_CONS, "\xE0\xB0\xAE", NULL },  /* ba, bha, ma */
    { 'n', K_CONS, "\xE0\xB0\xAF", NULL }, { 'k', K_CONS, "\xE0\xB0\xB0", NULL }, { 'a', K_CONS, "\xE0\xB0\xB2", NULL },  /* ya, ra, la */
    { 'f', K_CONS, "\xE0\xB0\xB5", NULL }, { 'F', K_CONS, "\xE0\xB0\xB6", NULL }, { '\"', K_CONS, "\xE0\xB0\xB7", NULL },  /* va, sha, ssa */
    { '\'', K_CONS, "\xE0\xB0\xB8", NULL }, { 'B', K_CONS, "\xE0\xB0\xB9", NULL }, { 'A', K_CONS, "\xE0\xB0\xB3", NULL },  /* sa, ha, lla */
    { 'K', K_CONS, "\xE0\xB0\xB1", NULL }, { 'Y', K_CONS, "\xE0\xB0\x95\xE0\xB1\x8D\xE0\xB0\xB7", NULL },  /* rra, ka-virama-ssa */
    { 'h', K_HALANT, "\xE0\xB1\x8D", NULL }, { 'H', K_HALANT, "\xE0\xB1\x8D", NULL },  /* virama, virama */
    { 'q', K_VOWEL, "\xE0\xB0\x85", "" }, { 'e', K_VOWEL, "\xE0\xB0\x86", "\xE0\xB0\xBE" },  /* a, aa */
    { 'r', K_VOWEL, "\xE0\xB0\x87", "\xE0\xB0\xBF" }, { 'w', K_VOWEL, "\xE0\xB0\x88", "\xE0\xB1\x80" },  /* i, ii */
    { 'i', K_VOWEL, "\xE0\xB0\x89", "\xE0\xB1\x81" }, { 'p', K_VOWEL, "\xE0\xB0\x8A", "\xE0\xB1\x82" },  /* u, uu */
    { 'W', K_VOWEL, "\xE0\xB0\x8B", "\xE0\xB1\x83" }, { 'E', K_VOWEL, "\xE0\xB1\xA0", "\xE0\xB1\x84" },  /* vocalic r, vocalic rr */
    { 'u', K_VOWEL, "\xE0\xB0\x8E", "\xE0\xB1\x86" }, { 'o', K_VOWEL, "\xE0\xB0\x8F", "\xE0\xB1\x87" }, { '[', K_VOWEL, "\xE0\xB0\x90", "\xE0\xB1\x88" },  /* e, ee, ai */
    { 't', K_VOWEL, "\xE0\xB0\x92", "\xE0\xB1\x8A" }, { 'y', K_VOWEL, "\xE0\xB0\x93", "\xE0\xB1\x8B" }, { ']', K_VOWEL, "\xE0\xB0\x94", "\xE0\xB1\x8C" },  /* o, oo, au */
    /* Text keys: signs, conjunct shortcuts, punctuation moved off the
     * letter keys. */
    { 'g', K_TEXT, "\xE0\xB0\x82", NULL }, { '|', K_TEXT, "\xE0\xB0\x81", NULL }, { '\\', K_TEXT, "\xE0\xB0\x83", NULL },  /* anusvara, candrabindu, visarga */
    { 'U', K_TEXT, "\xE0\xB0\xB6\xE0\xB1\x8D\xE0\xB0\xB0\xE0\xB1\x80", NULL },  /* sha-virama-ra-ii sign */
    { 'Q', K_TEXT, "\xE0\xB0\x95\xE0\xB1\x8D\xE0\xB0\xB7\xE0\xB1\x8D\xE0\xB0\xAE\xE0\xB0\xBF", NULL }, { 'N', K_TEXT, "\xE0\xB0\x95\xE0\xB1\x8D\xE0\xB0\xB7\xE0\xB1\x8D\xE0\xB0\xAE\xE0\xB1\x80", NULL },  /* ka-virama-ssa-virama-ma-i sign, ka-virama-ssa-virama-ma-ii sign */
    { '{', K_TEXT, "\xE0\xB0\x95\xE0\xB1\x8D\xE0\xB0\xB7\xE0\xB1\x8D\xE0\xB0\xAE", NULL },  /* ka-virama-ssa-virama-ma */
    { 'O', K_TEXT, "\xE0\xB0\xB8\xE0\xB1\x8D\xE0\xB0\xA4\xE0\xB1\x8D\xE0\xB0\xB0", NULL }, { 'P', K_TEXT, "\xE0\xB0\xB7\xE0\xB1\x8D\xE0\xB0\x9F\xE0\xB1\x8D\xE0\xB0\xB0", NULL },  /* sa-virama-ta-virama-ra, ssa-virama-tta-virama-ra */
    { '$', K_TEXT, "\xE0\xB1\x8D\xE0\xB0\xAA\xE0\xB1\x81", NULL }, { '%', K_TEXT, "\xE0\xB1\x8D\xE0\xB0\xB0", NULL },  /* virama-pa-u sign, virama-ra */
    { '@', K_TEXT, "'", NULL }, { '#', K_TEXT, "%", NULL }, { '^', K_TEXT, "-", NULL },
    { '&', K_TEXT, "|", NULL }, { '*', K_TEXT, "`", NULL },
    { '-', K_TEXT, "\xC3\x97", NULL }, { '_', K_TEXT, "\xC3\xB7", NULL },  /* multiplication sign, division sign */
    { '}', K_TEXT, "!", NULL }, { 'I', K_TEXT, "/", NULL }, { '<', K_TEXT, ";", NULL },
    { '>', K_TEXT, "?", NULL }, { 'G', K_TEXT, ":", NULL }, { '`', K_TEXT, "_", NULL },
};

/* State of the one-key engines: what the previous key left. */
enum {
    S_NONE = 0,
    S_CONS,          /* a consonant (a vowel key types its sign) */
    S_SIGN,          /* consonant + vowel sign (Apple: halant moves it) */
    S_SIGN_HALANT,   /* Apple: consonant + virama + the moved sign */
};

static uint8_t s_state;
static char s_sign[16];          /* S_SIGN / S_SIGN_HALANT: the sign */

static void set_comp(const char *tail)
{
    ime_copy(g_comp, sizeof(g_comp), tail);
    g_comp_len = strlen(g_comp);
}

static void one_reset(void)
{
    s_state = S_NONE;
    s_sign[0] = '\0';
    comp_forget();
}

static const te_key_t *find_key(const te_key_t *tab, size_t n, char c)
{
    for (size_t i = 0; i < n; i++) {
        if (tab[i].key == c) return &tab[i];
    }
    return NULL;
}

static bool one_key(const te_key_t *tab, size_t n, bool apple, uint8_t keycode,
                    uint8_t modifier, const char *text)
{
    if (is_shift_space(keycode, modifier)) {
        one_reset();
        ime_replace(0, ZWNJ);
        return true;
    }
    char c = key_char(text);
    /* AltGr types the layout's own characters (Telugu digits, the
     * punctuation the letter keys took). */
    const te_key_t *k = (c && !(modifier & IME_MOD_RALT)) ? find_key(tab, n, c) : NULL;
    if (!k) {
        one_reset();
        return false;
    }
    switch (k->kind) {
    case K_CONS:
        if (apple && s_state == S_SIGN_HALANT) {
            /* consonant, sign, halant, consonant: the sign moves after
             * the second consonant. */
            char buf[48];
            ime_join(buf, sizeof(buf), k->text, s_sign);
            ime_replace(strlen(s_sign), buf);
            set_comp(s_sign);
            s_state = S_SIGN;
            return true;
        }
        ime_replace(0, k->text);
        set_comp(k->text);
        s_state = S_CONS;
        return true;
    case K_VOWEL:
        if (s_state == S_CONS) {
            if (!k->sign[0]) {
                /* a (Apple: q) ends the syllable: nothing to type. */
                one_reset();
                return true;
            }
            ime_replace(0, k->sign);
            if (apple) {
                ime_copy(s_sign, sizeof(s_sign), k->sign);
                set_comp(k->sign);
                s_state = S_SIGN;
            } else {
                one_reset();
            }
            return true;
        }
        ime_replace(0, k->text);
        one_reset();
        return true;
    case K_HALANT:
        if (apple && s_state == S_SIGN) {
            char buf[32];
            ime_join(buf, sizeof(buf), TE_VIRAMA, s_sign);
            ime_replace(strlen(s_sign), buf);
            set_comp(buf);
            s_state = S_SIGN_HALANT;
            return true;
        }
        ime_replace(0, k->text);
        one_reset();
        return true;
    case K_TEXT:
    default:
        ime_replace(0, k->text);
        one_reset();
        return true;
    }
}

static void one_commit(void)
{
    one_reset();
}

static bool one_composing(void)
{
    return s_state != S_NONE;
}

static const char *one_status(void)
{
    return NULL;
}

static bool pothana_key(uint8_t keycode, uint8_t modifier, const char *text)
{
    return one_key(POTHANA, sizeof(POTHANA) / sizeof(POTHANA[0]), false,
                   keycode, modifier, text);
}

static bool apple_key(uint8_t keycode, uint8_t modifier, const char *text)
{
    return one_key(APPLE, sizeof(APPLE) / sizeof(APPLE[0]), true,
                   keycode, modifier, text);
}

const ime_engine_t ime_telugu_pothana_engine = {
    pothana_key, one_commit, one_composing, one_status,
};

const ime_engine_t ime_telugu_apple_engine = {
    apple_key, one_commit, one_composing, one_status,
};
