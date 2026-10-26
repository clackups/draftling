/*
 * Vietnamese Telex input.
 *
 * Stateless: every letter key re-reads the word right before the
 * cursor, applies the key to it and writes the word back. So a tone or
 * vowel mark can be added to a word at any time while the cursor is at
 * its end ("free" Telex), and the composition never needs tracking.
 *
 *   aa ee oo -> a e o with circumflex     aw ow uw -> a-breve, o/u horn
 *   uow -> u-horn o-horn                  w alone -> u-horn
 *   dd -> d-stroke
 *   s f r x j -> acute, grave, hook, tilde, dot below;  z removes it
 *
 * Typing the same mark key again undoes it and types the letter
 * ("ass" -> "as"). Marks are only applied when the word is a plausible
 * Vietnamese syllable, so most English words come through unchanged.
 * Tones are placed by the traditional rules (hoa + f -> "hoa" with the
 * grave on o; a vowel with a mark takes the tone; with a final
 * consonant the last vowel takes it).
 */

#include <string.h>
#include <ctype.h>
#include "ime_internal.h"

enum { MOD_NONE = 0, MOD_CIRC, MOD_BREVE, MOD_HORN, MOD_STROKE };
enum { TONE_NONE = 0, TONE_ACUTE, TONE_GRAVE, TONE_HOOK, TONE_TILDE, TONE_DOT };

/* Rows: a, a-breve, a-circ, e, e-circ, i, o, o-circ, o-horn, u, u-horn, y.
 * Columns: tone none, acute, grave, hook, tilde, dot below. */
static const uint16_t VOWEL_LOWER[12][6] = {
    { 0x0061, 0x00E1, 0x00E0, 0x1EA3, 0x00E3, 0x1EA1 }, /* a */
    { 0x0103, 0x1EAF, 0x1EB1, 0x1EB3, 0x1EB5, 0x1EB7 }, /* a breve */
    { 0x00E2, 0x1EA5, 0x1EA7, 0x1EA9, 0x1EAB, 0x1EAD }, /* a circumflex */
    { 0x0065, 0x00E9, 0x00E8, 0x1EBB, 0x1EBD, 0x1EB9 }, /* e */
    { 0x00EA, 0x1EBF, 0x1EC1, 0x1EC3, 0x1EC5, 0x1EC7 }, /* e circumflex */
    { 0x0069, 0x00ED, 0x00EC, 0x1EC9, 0x0129, 0x1ECB }, /* i */
    { 0x006F, 0x00F3, 0x00F2, 0x1ECF, 0x00F5, 0x1ECD }, /* o */
    { 0x00F4, 0x1ED1, 0x1ED3, 0x1ED5, 0x1ED7, 0x1ED9 }, /* o circumflex */
    { 0x01A1, 0x1EDB, 0x1EDD, 0x1EDF, 0x1EE1, 0x1EE3 }, /* o horn */
    { 0x0075, 0x00FA, 0x00F9, 0x1EE7, 0x0169, 0x1EE5 }, /* u */
    { 0x01B0, 0x1EE9, 0x1EEB, 0x1EED, 0x1EEF, 0x1EF1 }, /* u horn */
    { 0x0079, 0x00FD, 0x1EF3, 0x1EF7, 0x1EF9, 0x1EF5 }, /* y */
};
static const uint16_t VOWEL_UPPER[12][6] = {
    { 0x0041, 0x00C1, 0x00C0, 0x1EA2, 0x00C3, 0x1EA0 }, /* A */
    { 0x0102, 0x1EAE, 0x1EB0, 0x1EB2, 0x1EB4, 0x1EB6 }, /* A breve */
    { 0x00C2, 0x1EA4, 0x1EA6, 0x1EA8, 0x1EAA, 0x1EAC }, /* A circumflex */
    { 0x0045, 0x00C9, 0x00C8, 0x1EBA, 0x1EBC, 0x1EB8 }, /* E */
    { 0x00CA, 0x1EBE, 0x1EC0, 0x1EC2, 0x1EC4, 0x1EC6 }, /* E circumflex */
    { 0x0049, 0x00CD, 0x00CC, 0x1EC8, 0x0128, 0x1ECA }, /* I */
    { 0x004F, 0x00D3, 0x00D2, 0x1ECE, 0x00D5, 0x1ECC }, /* O */
    { 0x00D4, 0x1ED0, 0x1ED2, 0x1ED4, 0x1ED6, 0x1ED8 }, /* O circumflex */
    { 0x01A0, 0x1EDA, 0x1EDC, 0x1EDE, 0x1EE0, 0x1EE2 }, /* O horn */
    { 0x0055, 0x00DA, 0x00D9, 0x1EE6, 0x0168, 0x1EE4 }, /* U */
    { 0x01AF, 0x1EE8, 0x1EEA, 0x1EEC, 0x1EEE, 0x1EF0 }, /* U horn */
    { 0x0059, 0x00DD, 0x1EF2, 0x1EF6, 0x1EF8, 0x1EF4 }, /* Y */
};
/* Base letter and mark of each row. */
static const char ROW_BASE[12] = { 'a','a','a','e','e','i','o','o','o','u','u','y' };
static const uint8_t ROW_MOD[12] = {
    MOD_NONE, MOD_BREVE, MOD_CIRC, MOD_NONE, MOD_CIRC, MOD_NONE,
    MOD_NONE, MOD_CIRC, MOD_HORN, MOD_NONE, MOD_HORN, MOD_NONE,
};

#define MAX_LETTERS 12

typedef struct {
    char base;        /* 'a'..'z' */
    uint8_t mod;
    bool upper;
} letter_t;

typedef struct {
    letter_t l[MAX_LETTERS + 1];
    int n;
    uint8_t tone;
} word_t;

static bool is_vowel(char c)
{
    return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' || c == 'y';
}

static int row_of(char base, uint8_t mod)
{
    for (int r = 0; r < 12; r++) {
        if (ROW_BASE[r] == base && ROW_MOD[r] == mod) return r;
    }
    return -1;
}

/* Parse one code point into a letter (+ tone). False if it is not a
 * Vietnamese letter. */
static bool parse_cp(uint32_t cp, letter_t *out, uint8_t *tone)
{
    *tone = TONE_NONE;
    out->mod = MOD_NONE;
    if (cp < 0x80 && isalpha((int)cp)) {
        out->upper = isupper((int)cp) != 0;
        out->base = (char)tolower((int)cp);
        return true;
    }
    if (cp == 0x0111 || cp == 0x0110) {
        out->base = 'd';
        out->mod = MOD_STROKE;
        out->upper = cp == 0x0110;
        return true;
    }
    for (int r = 0; r < 12; r++) {
        for (int t = 0; t < 6; t++) {
            if (VOWEL_LOWER[r][t] == cp || VOWEL_UPPER[r][t] == cp) {
                out->base = ROW_BASE[r];
                out->mod = ROW_MOD[r];
                out->upper = VOWEL_UPPER[r][t] == cp;
                *tone = (uint8_t)t;
                return true;
            }
        }
    }
    return false;
}

/* Syllable structure: [0, vstart) initial consonant, [vstart, vend)
 * vowel cluster, [vend, n) final consonant. */
typedef struct {
    int vstart, vend;
} split_t;

static split_t split_word(const word_t *w)
{
    split_t s;
    int i = 0;
    while (i < w->n && !is_vowel(w->l[i].base)) i++;
    /* "qu" and "gi" followed by another vowel: the u / i belongs to
     * the initial consonant. */
    if (i == 1 && w->l[0].base == 'q' && i < w->n && w->l[i].base == 'u' &&
        i + 1 < w->n && is_vowel(w->l[i + 1].base)) {
        i++;
    } else if (i == 1 && w->l[0].base == 'g' && i < w->n && w->l[i].base == 'i' &&
               w->l[i].mod == MOD_NONE && i + 1 < w->n && is_vowel(w->l[i + 1].base)) {
        i++;
    }
    s.vstart = i;
    while (i < w->n && is_vowel(w->l[i].base)) i++;
    s.vend = i;
    return s;
}

static bool seq_is(const word_t *w, int from, int to, const char *s)
{
    if ((int)strlen(s) != to - from) return false;
    for (int i = from; i < to; i++) {
        if (w->l[i].base != s[i - from]) return false;
    }
    return true;
}

static bool valid_initial(const word_t *w, int end)
{
    static const char *const INITIALS[] = {
        "", "b", "c", "ch", "d", "g", "gh", "gi", "h", "k", "kh", "l", "m",
        "n", "ng", "ngh", "nh", "p", "ph", "qu", "r", "s", "t", "th", "tr",
        "v", "x", NULL,
    };
    for (int i = 0; INITIALS[i]; i++) {
        if (seq_is(w, 0, end, INITIALS[i])) return true;
    }
    return false;
}

static bool valid_final(const word_t *w, int start)
{
    static const char *const FINALS[] = {
        "", "c", "ch", "m", "n", "ng", "nh", "p", "t", NULL,
    };
    for (int i = 0; FINALS[i]; i++) {
        if (seq_is(w, start, w->n, FINALS[i])) return true;
    }
    return false;
}

static bool plausible(const word_t *w)
{
    split_t s = split_word(w);
    if (s.vend == s.vstart) return false;
    if (s.vend - s.vstart > 3) return false;
    return valid_initial(w, s.vstart) && valid_final(w, s.vend);
}

/* Index of the vowel carrying the tone. */
static int tone_pos(const word_t *w)
{
    split_t s = split_word(w);
    int len = s.vend - s.vstart;
    if (len <= 0) return -1;
    /* u-horn o-horn: the tone goes on the o. */
    for (int i = s.vstart; i + 1 < s.vend; i++) {
        if (w->l[i].base == 'u' && w->l[i + 1].base == 'o' &&
            w->l[i + 1].mod == MOD_HORN) return i + 1;
    }
    for (int i = s.vend - 1; i >= s.vstart; i--) {
        if (w->l[i].mod != MOD_NONE) return i;
    }
    if (len == 1) return s.vstart;
    if (s.vend < w->n) return s.vend - 1;         /* final consonant */
    if (len == 3) return s.vstart + 1;
    return s.vstart;                               /* hoa, thuy, mia */
}

static size_t render(const word_t *w, char *out, size_t size)
{
    int tp = w->tone ? tone_pos(w) : -1;
    size_t pos = 0;
    for (int i = 0; i < w->n && pos + 4 < size; i++) {
        const letter_t *l = &w->l[i];
        uint32_t cp;
        if (l->base == 'd' && l->mod == MOD_STROKE) {
            cp = l->upper ? 0x0110 : 0x0111;
        } else if (is_vowel(l->base)) {
            int r = row_of(l->base, l->mod);
            if (r < 0) r = row_of(l->base, MOD_NONE);
            int t = (i == tp) ? w->tone : 0;
            cp = l->upper ? VOWEL_UPPER[r][t] : VOWEL_LOWER[r][t];
        } else {
            cp = (uint32_t)(l->upper ? toupper((int)l->base) : l->base);
        }
        pos += utf8_encode(cp, out + pos);
    }
    out[pos] = '\0';
    return pos;
}

static void append(word_t *w, char c, bool upper)
{
    if (w->n >= MAX_LETTERS) return;
    w->l[w->n].base = c;
    w->l[w->n].mod = MOD_NONE;
    w->l[w->n].upper = upper;
    w->n++;
}

/* Apply a key to the word. Returns false when the key just appends. */
static bool apply(word_t *w, char key, bool upper)
{
    word_t before = *w;
    split_t s = split_word(w);

    if (key == 'd') {
        if (w->n > 0 && w->l[0].base == 'd') {
            if (w->l[0].mod == MOD_NONE) {
                w->l[0].mod = MOD_STROKE;
                if (w->n == 1 || plausible(w)) return true;
                *w = before;
            } else if (w->l[0].mod == MOD_STROKE && w->n == 1) {
                w->l[0].mod = MOD_NONE;      /* "ddd" -> "dd" */
                append(w, key, upper);
                return true;
            }
        }
        append(w, key, upper);
        return false;
    }

    if (key == 'a' || key == 'e' || key == 'o') {
        for (int i = s.vend - 1; i >= s.vstart; i--) {
            if (w->l[i].base != key) continue;
            if (w->l[i].mod == MOD_NONE || w->l[i].mod == MOD_BREVE ||
                w->l[i].mod == MOD_HORN) {
                w->l[i].mod = MOD_CIRC;
                if (plausible(w)) return true;
                *w = before;
            } else if (w->l[i].mod == MOD_CIRC) {
                w->l[i].mod = MOD_NONE;      /* "aaa" -> "aa" */
                append(w, key, upper);
                return true;
            }
            break;
        }
        append(w, key, upper);
        return false;
    }

    if (key == 'w') {
        int targets[2] = { -1, -1 };
        uint8_t mods[2] = { MOD_HORN, MOD_HORN };
        for (int i = s.vstart; i + 1 < s.vend; i++) {
            if (w->l[i].base == 'u' && w->l[i + 1].base == 'o') {
                targets[0] = i; targets[1] = i + 1;
                break;
            }
        }
        if (targets[0] < 0) {
            for (int i = s.vstart; i + 1 < s.vend; i++) {
                if (w->l[i].base == 'o' && w->l[i + 1].base == 'a') {
                    targets[0] = i + 1; mods[0] = MOD_BREVE;
                    break;
                }
            }
        }
        if (targets[0] < 0) {
            const char order[3] = { 'o', 'u', 'a' };
            for (int k = 0; k < 3 && targets[0] < 0; k++) {
                for (int i = s.vend - 1; i >= s.vstart; i--) {
                    if (w->l[i].base == order[k]) {
                        targets[0] = i;
                        mods[0] = order[k] == 'a' ? MOD_BREVE : MOD_HORN;
                        break;
                    }
                }
            }
        }
        if (targets[0] < 0) {
            /* No vowel yet: w types u-horn ("nhw" -> "nhu-horn"). */
            if (s.vstart == w->n && valid_initial(w, w->n) && w->n < MAX_LETTERS) {
                append(w, 'u', upper);
                w->l[w->n - 1].mod = MOD_HORN;
                return true;
            }
            append(w, key, upper);
            return false;
        }
        bool all_set = true;
        for (int k = 0; k < 2; k++) {
            if (targets[k] >= 0 && w->l[targets[k]].mod != mods[k]) all_set = false;
        }
        if (all_set) {
            /* Second w undoes the marks and types the letter. */
            for (int k = 0; k < 2; k++) {
                if (targets[k] >= 0) w->l[targets[k]].mod = MOD_NONE;
            }
            append(w, key, upper);
            return true;
        }
        for (int k = 0; k < 2; k++) {
            if (targets[k] >= 0) w->l[targets[k]].mod = mods[k];
        }
        if (plausible(w)) return true;
        *w = before;
        append(w, key, upper);
        return false;
    }

    int tone = -1;
    switch (key) {
    case 's': tone = TONE_ACUTE; break;
    case 'f': tone = TONE_GRAVE; break;
    case 'r': tone = TONE_HOOK; break;
    case 'x': tone = TONE_TILDE; break;
    case 'j': tone = TONE_DOT; break;
    case 'z': tone = TONE_NONE; break;
    default: break;
    }
    if (tone >= 0 && plausible(w)) {
        if (tone == TONE_NONE) {
            if (w->tone == TONE_NONE) { append(w, key, upper); return false; }
            w->tone = TONE_NONE;
            return true;
        }
        if (w->tone == tone) {
            w->tone = TONE_NONE;             /* "ass" -> "as" */
            append(w, key, upper);
            return true;
        }
        w->tone = (uint8_t)tone;
        return true;
    }

    append(w, key, upper);
    /* A new final consonant can move the tone (hoa' + n -> hoan'). */
    return w->tone != TONE_NONE;
}

static bool telex_key(uint8_t keycode, uint8_t modifier, const char *text)
{
    (void)keycode;
    (void)modifier;
    if (!text || !text[0] || text[1] || !isalpha((unsigned char)text[0])) return false;
    char key = (char)tolower((unsigned char)text[0]);
    bool upper = isupper((unsigned char)text[0]) != 0;

    /* Read the word before the cursor. */
    char buf[MAX_LETTERS * 4 + 4];
    size_t n = ime_peek(buf, sizeof(buf));
    word_t w;
    memset(&w, 0, sizeof(w));
    letter_t rev[MAX_LETTERS + 1];
    uint8_t tones[MAX_LETTERS + 1];
    int count = 0;
    size_t end = n;
    while (end > 0) {
        size_t cl = utf8_last_len(buf, end);
        size_t used;
        uint32_t cp = utf8_decode(buf + end - cl, cl, &used);
        letter_t l;
        uint8_t t;
        if (!parse_cp(cp, &l, &t)) break;
        if (count >= MAX_LETTERS) return false;   /* too long to be Vietnamese */
        rev[count] = l;
        tones[count] = t;
        count++;
        end -= cl;
    }
    size_t word_bytes = n - end;
    for (int i = 0; i < count; i++) {
        w.l[i] = rev[count - 1 - i];
        if (tones[count - 1 - i]) w.tone = tones[count - 1 - i];
    }
    w.n = count;

    bool changed = apply(&w, key, upper);
    if (!changed) return false;           /* the editor types the letter */

    char out[MAX_LETTERS * 4 + 8];
    render(&w, out, sizeof(out));
    return ime_replace(word_bytes, out);
}

static void telex_commit(void) {}
static bool telex_composing(void) { return false; }
static const char *telex_status(void) { return NULL; }

const ime_engine_t ime_telex_engine = {
    telex_key, telex_commit, telex_composing, telex_status,
};
