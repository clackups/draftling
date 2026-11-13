/*
 * Tamil 99 input (the Government of Tamil Nadu keyboard standard,
 * G.O. Ms. 17 of 13.06.1999).
 *
 * Vowels on the left hand, consonants on the right; every key types a
 * whole letter (consonants with the inherent a). The rules of the
 * standard:
 *
 *   consonant + vowel        -> the vowel joins as its sign (k + aa)
 *   consonant + f            -> pulli (dead consonant)
 *   consonant + same one     -> pulli added to the first (kk -> k-pulli k)
 *   nasal + its stop         -> pulli added (ng-k, ny-c, nn-tt, n-t, m-p, n-r)
 *   after such an automatic pulli the next consonant gets none
 *       (k k k -> k-pulli k k, four k -> k-pulli k k-pulli k)
 *   consonant + a            -> confirms the a: the next vowel stands
 *       alone and the next consonant gets no automatic pulli
 *   a vowel after anything else is the independent vowel
 *
 * The state is the last consonant this engine typed (kept as the
 * composition, so moving the cursor away resets it).
 */

#include <string.h>
#include "ime_tamil.h"

enum { K_NONE = 0, K_VOWEL, K_CONS, K_PULLI, K_TEXT };

typedef struct {
    char key;
    uint8_t kind;
    uint8_t vowel;          /* K_VOWEL: index into TA_VOWEL_* */
    uint32_t cps[4];        /* K_CONS / K_TEXT: code points, 0-ended */
} t99_key_t;

/* Shift + T: k-pulli-ssa (a consonant cluster that takes signs);
 * Shift + Y: sri. */
static const t99_key_t KEYS[] = {
    /* Vowels (left hand). */
    { 'a', K_VOWEL, 0,  { 0 } },
    { 'q', K_VOWEL, 1,  { 0 } },
    { 's', K_VOWEL, 2,  { 0 } },
    { 'w', K_VOWEL, 3,  { 0 } },
    { 'd', K_VOWEL, 4,  { 0 } },
    { 'e', K_VOWEL, 5,  { 0 } },
    { 'g', K_VOWEL, 6,  { 0 } },
    { 't', K_VOWEL, 7,  { 0 } },
    { 'r', K_VOWEL, 8,  { 0 } },
    { 'c', K_VOWEL, 9,  { 0 } },
    { 'x', K_VOWEL, 10, { 0 } },
    { 'z', K_VOWEL, 11, { 0 } },
    /* Pulli and aytham. */
    { 'f', K_PULLI, 0,  { 0 } },
    { 'F', K_TEXT,  0,  { TA_AYTHAM } },
    /* Consonants (right hand and v / b). */
    { 'h', K_CONS,  0,  { 0x0B95 } },   /* ka */
    { 'b', K_CONS,  0,  { 0x0B99 } },   /* nga */
    { '[', K_CONS,  0,  { 0x0B9A } },   /* ca */
    { ']', K_CONS,  0,  { 0x0B9E } },   /* nya */
    { 'o', K_CONS,  0,  { 0x0B9F } },   /* tta */
    { 'p', K_CONS,  0,  { 0x0BA3 } },   /* nna */
    { 'l', K_CONS,  0,  { 0x0BA4 } },   /* ta */
    { ';', K_CONS,  0,  { 0x0BA8 } },   /* na */
    { 'j', K_CONS,  0,  { 0x0BAA } },   /* pa */
    { 'k', K_CONS,  0,  { 0x0BAE } },   /* ma */
    { '\'', K_CONS, 0,  { 0x0BAF } },   /* ya */
    { 'm', K_CONS,  0,  { 0x0BB0 } },   /* ra */
    { 'n', K_CONS,  0,  { 0x0BB2 } },   /* la */
    { 'v', K_CONS,  0,  { 0x0BB5 } },   /* va */
    { '/', K_CONS,  0,  { 0x0BB4 } },   /* llla */
    { 'y', K_CONS,  0,  { 0x0BB3 } },   /* lla */
    { 'u', K_CONS,  0,  { 0x0BB1 } },   /* rra */
    { 'i', K_CONS,  0,  { 0x0BA9 } },   /* nnna */
    /* Grantha (Shift). */
    { 'Q', K_CONS,  0,  { 0x0BB8 } },   /* sa */
    { 'W', K_CONS,  0,  { 0x0BB7 } },   /* ssa */
    { 'E', K_CONS,  0,  { 0x0B9C } },   /* ja */
    { 'R', K_CONS,  0,  { 0x0BB9 } },   /* ha */
    { 'U', K_CONS,  0,  { 0x0BB6 } },   /* sha */
    { 'T', K_CONS,  0,  { TA_KA, TA_PULLI, TA_SSA } },             /* k.ssa */
    { 'Y', K_TEXT,  0,  { TA_SA, TA_PULLI, TA_RA, TA_SIGN_II } },  /* sri */
    /* Tamil symbols (Shift). */
    { 'A', K_TEXT,  0,  { 0x0BF9 } },   /* rupee sign */
    { 'S', K_TEXT,  0,  { 0x0BFA } },   /* number sign */
    { 'D', K_TEXT,  0,  { 0x0BF8 } },   /* as above */
    { 'Z', K_TEXT,  0,  { 0x0BF3 } },   /* day */
    { 'X', K_TEXT,  0,  { 0x0BF4 } },   /* month */
    { 'C', K_TEXT,  0,  { 0x0BF5 } },   /* year */
    { 'V', K_TEXT,  0,  { 0x0BF6 } },   /* debit */
    { 'B', K_TEXT,  0,  { 0x0BF7 } },   /* credit */
    { 'L', K_TEXT,  0,  { 0x0BF1 } },   /* one hundred */
    { 'N', K_TEXT,  0,  { 0x0BD0 } },   /* om */
    /* The punctuation the consonants displaced. */
    { 'I', K_TEXT,  0,  { ':' } },
    { 'O', K_TEXT,  0,  { '[' } },
    { 'P', K_TEXT,  0,  { ']' } },
    { 'K', K_TEXT,  0,  { '"' } },
    { ':', K_TEXT,  0,  { ';' } },
    { '"', K_TEXT,  0,  { '\'' } },
    { 'M', K_TEXT,  0,  { '/' } },
};

/* Nasal + stop pairs that get an automatic pulli. */
static const uint16_t PAIRS[][2] = {
    { 0x0B99, 0x0B95 },   /* nga ka */
    { 0x0B9E, 0x0B9A },   /* nya ca */
    { 0x0BA8, 0x0BA4 },   /* na ta */
    { 0x0BA3, 0x0B9F },   /* nna tta */
    { 0x0BAE, 0x0BAA },   /* ma pa */
    { 0x0BA9, 0x0BB1 },   /* nnna rra */
};

enum { LAST_NONE = 0, LAST_CONS, LAST_CONFIRMED };

static uint8_t  s_last;        /* what the previous key left */
static bool     s_no_auto;     /* an automatic pulli was just added */
static uint32_t s_cons[4];     /* the last consonant typed */

static const t99_key_t *find_key(char c)
{
    for (size_t i = 0; i < sizeof(KEYS) / sizeof(KEYS[0]); i++) {
        if (KEYS[i].key == c) return &KEYS[i];
    }
    return NULL;
}

static size_t cps_len(const uint32_t *cps)
{
    size_t n = 0;
    while (n < 4 && cps[n]) n++;
    return n;
}

static bool auto_pulli(const uint32_t *next)
{
    if (s_last != LAST_CONS || s_no_auto) return false;
    size_t a = cps_len(s_cons), b = cps_len(next);
    if (a == b && memcmp(s_cons, next, a * sizeof(uint32_t)) == 0) return true;
    if (a != 1 || b != 1) return false;
    for (size_t i = 0; i < sizeof(PAIRS) / sizeof(PAIRS[0]); i++) {
        if (PAIRS[i][0] == s_cons[0] && PAIRS[i][1] == next[0]) return true;
    }
    return false;
}

static void reset(void)
{
    s_last = LAST_NONE;
    s_no_auto = false;
    memset(s_cons, 0, sizeof(s_cons));
    comp_forget();
}

/* Insert cps and make the composition what the engine must find
 * before the cursor next time (keep_n trailing code points). */
static bool put_track(const uint32_t *cps, size_t n, size_t keep_n)
{
    if (!ta_put(0, cps, n)) return false;
    comp_forget();
    if (keep_n) {
        ta_encode(cps + n - keep_n, keep_n, g_comp, sizeof(g_comp));
        g_comp_len = strlen(g_comp);
    }
    return true;
}

static bool t99_key(uint8_t keycode, uint8_t modifier, const char *text)
{
    (void)keycode;
    (void)modifier;
    char c = ta_key_char(text);
    const t99_key_t *k = c ? find_key(c) : NULL;
    if (!k) {
        reset();
        return false;
    }

    switch (k->kind) {
    case K_CONS: {
        uint32_t buf[8];
        size_t n = 0, len = cps_len(k->cps);
        bool pulli = auto_pulli(k->cps);
        if (pulli) buf[n++] = TA_PULLI;
        memcpy(buf + n, k->cps, len * sizeof(uint32_t));
        n += len;
        if (!put_track(buf, n, len)) return true;
        s_last = LAST_CONS;
        s_no_auto = pulli;
        memset(s_cons, 0, sizeof(s_cons));
        memcpy(s_cons, k->cps, len * sizeof(uint32_t));
        return true;
    }
    case K_VOWEL: {
        uint32_t cp;
        if (s_last == LAST_CONS) {
            if (k->vowel == 0) {
                /* a confirms the inherent vowel: nothing to type. */
                s_last = LAST_CONFIRMED;
                s_no_auto = false;
                return true;
            }
            cp = TA_VOWEL_SIGN[k->vowel];
        } else {
            cp = TA_VOWEL_LETTER[k->vowel];
        }
        ta_put(0, &cp, 1);
        reset();
        return true;
    }
    case K_PULLI: {
        uint32_t cp = TA_PULLI;
        ta_put(0, &cp, 1);
        reset();
        return true;
    }
    case K_TEXT:
    default:
        ta_put(0, k->cps, cps_len(k->cps));
        reset();
        return true;
    }
}

static void t99_commit(void)
{
    reset();
}

static bool t99_composing(void)
{
    return s_last != LAST_NONE;
}

static const char *t99_status(void)
{
    return NULL;
}

const ime_engine_t ime_tamil99_engine = {
    t99_key, t99_commit, t99_composing, t99_status,
};
