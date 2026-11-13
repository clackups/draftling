/*
 * Tamil Typewriter input: the layout of the Tamil typewriter, as used
 * in the Tamil Nadu government typewriting examinations and by
 * eKalappai / Keyman's "Thamizha" typewriter keyboard (the rules here
 * follow that keyboard).
 *
 * The typewriter types glyph pieces in visual order: the e / ee / ai
 * signs (keys b, n, i) come before the consonant, the aa piece (h)
 * after it, so "ko" is b + k-key + h. Unicode stores the consonant
 * first, so the engine reorders:
 *
 *   e / ee / ai sign with no consonant before it, then a consonant
 *       -> consonant + sign
 *   consonant + e / ee sign, then h      -> o / oo sign
 *   consonant + e sign, then s (the lla key) -> au sign
 *       (and back to e + lla when a pulli or vowel sign follows)
 *   o, then s                            -> au
 *   consonant + u sign, then } or :      -> uu sign (the typewriter's
 *       separate uu pieces)
 *
 * Several keys type a whole syllable (o = tti, q = nnu, Shift + F =
 * ku, ...), as on the typewriter. Stateless: every key looks at the
 * text before the cursor.
 */

#include <string.h>
#include "ime_tamil.h"

enum {
    T_NONE = 0,
    T_TEXT,     /* type cps */
    T_CONS,     /* a consonant: takes a waiting e / ee / ai sign */
    T_PRE,      /* e / ee / ai sign typed before the consonant */
    T_AA,       /* the aa piece: aa sign, or completes o / oo */
    T_LLA,      /* lla, or completes au after the e sign / o */
    T_SIGN,     /* a sign that may split a just-made au */
    T_UU_HOOK,  /* turns a u sign before it into uu */
};

typedef struct {
    char key;
    uint8_t kind;
    uint32_t cps[4];
} tw_key_t;

static const tw_key_t KEYS[] = {
    /* Consonants. */
    { 'f', T_CONS, { 0x0B95 } },            /* ka */
    { '\'', T_CONS, { 0x0B99 } },           /* nga */
    { 'r', T_CONS, { 0x0B9A } },            /* ca */
    { '"', T_CONS, { 0x0B9E } },            /* nya */
    { 'l', T_CONS, { 0x0B9F } },            /* tta */
    { 'z', T_CONS, { 0x0BA3 } },            /* nna */
    { 'j', T_CONS, { 0x0BA4 } },            /* ta */
    { 'e', T_CONS, { 0x0BA8 } },            /* na */
    { 'g', T_CONS, { 0x0BAA } },            /* pa */
    { 'k', T_CONS, { 0x0BAE } },            /* ma */
    { 'a', T_CONS, { 0x0BAF } },            /* ya */
    { 'u', T_CONS, { 0x0BB0 } },            /* ra */
    { 'y', T_CONS, { 0x0BB2 } },            /* la */
    { 't', T_CONS, { 0x0BB5 } },            /* va */
    { 'H', T_CONS, { 0x0BB4 } },            /* llla */
    { 's', T_LLA,  { 0x0BB3 } },            /* lla */
    { 'w', T_CONS, { 0x0BB1 } },            /* rra */
    { 'd', T_CONS, { 0x0BA9 } },            /* nnna */
    { '!', T_CONS, { 0x0BB8 } },            /* sa */
    { '$', T_CONS, { 0x0B9C } },            /* ja */
    { '&', T_CONS, { 0x0BB7 } },            /* ssa */
    { '+', T_CONS, { 0x0BB9 } },            /* ha */
    { '\\', T_CONS, { TA_KA, TA_PULLI, TA_SSA } },  /* k.ssa */
    /* Vowels. */
    { 'm', T_TEXT, { 0x0B85 } },            /* a */
    { 'M', T_TEXT, { 0x0B86 } },            /* aa */
    { ',', T_TEXT, { 0x0B87 } },            /* i */
    { '<', T_TEXT, { 0x0B88 } },            /* ii */
    { 'c', T_TEXT, { 0x0B89 } },            /* u */
    { 'C', T_TEXT, { 0x0B8A } },            /* uu */
    { 'v', T_TEXT, { 0x0B8E } },            /* e */
    { 'V', T_TEXT, { 0x0B8F } },            /* ee */
    { 'I', T_TEXT, { 0x0B90 } },            /* ai */
    { 'x', T_TEXT, { 0x0B92 } },            /* o */
    { 'X', T_TEXT, { 0x0B93 } },            /* oo */
    { '`', T_TEXT, { TA_AYTHAM } },
    /* Signs. */
    { 'b', T_PRE,  { TA_SIGN_E } },
    { 'B', T_PRE,  { TA_SIGN_E } },
    { 'n', T_PRE,  { TA_SIGN_EE } },
    { 'i', T_PRE,  { TA_SIGN_AI } },
    { 'h', T_AA,   { TA_SIGN_AA } },
    { 'p', T_SIGN, { TA_SIGN_I } },
    { 'P', T_SIGN, { TA_SIGN_II } },
    { ';', T_SIGN, { TA_PULLI } },
    { '%', T_SIGN, { TA_SIGN_U } },         /* u sign of the Grantha letters */
    { '^', T_SIGN, { TA_SIGN_UU } },        /* uu sign of the Grantha letters */
    { '[', T_SIGN, { TA_SIGN_U } },         /* u hook of nga, pa, ya, va */
    { '{', T_SIGN, { TA_SIGN_UU } },        /* uu hook of nga, pa, ya, va */
    { ']', T_SIGN, { TA_SIGN_AI } },        /* ai of nna, nnna, la, lla */
    { '}', T_UU_HOOK, { TA_SIGN_UU } },     /* uu of nya nna ta na nnna rra la */
    { ':', T_UU_HOOK, { TA_SIGN_UU } },     /* uu of tta ma llla ra lla */
    /* Whole syllables. */
    { 'o', T_TEXT, { 0x0B9F, TA_SIGN_I } },     /* tti */
    { 'O', T_TEXT, { 0x0B9F, TA_SIGN_II } },    /* ttii */
    { 'q', T_TEXT, { 0x0BA3, TA_SIGN_U } },     /* nnu */
    { 'Q', T_TEXT, { 0x0B9E, TA_SIGN_U } },     /* nyu */
    { 'R', T_TEXT, { 0x0B9A, TA_SIGN_U } },     /* cu */
    { 'S', T_TEXT, { 0x0BB3, TA_SIGN_U } },     /* llu */
    { 'D', T_TEXT, { 0x0BA9, TA_SIGN_U } },     /* nnnu */
    { 'E', T_TEXT, { 0x0BA8, TA_SIGN_U } },     /* nu */
    { 'F', T_TEXT, { 0x0B95, TA_SIGN_U } },     /* ku */
    { 'G', T_TEXT, { 0x0BB4, TA_SIGN_U } },     /* lllu */
    { 'J', T_TEXT, { 0x0BA4, TA_SIGN_U } },     /* tu */
    { 'K', T_TEXT, { 0x0BAE, TA_SIGN_U } },     /* mu */
    { 'L', T_TEXT, { 0x0B9F, TA_SIGN_U } },     /* ttu */
    { 'U', T_TEXT, { 0x0BB0, TA_SIGN_U } },     /* ru */
    { 'W', T_TEXT, { 0x0BB1, TA_SIGN_U } },     /* rru */
    { 'Y', T_TEXT, { 0x0BB2, TA_SIGN_U } },     /* lu */
    { 'T', T_TEXT, { 0x0B95, TA_SIGN_UU } },    /* kuu */
    { 'N', T_TEXT, { 0x0B9A, TA_SIGN_UU } },    /* cuu */
    { '_', T_TEXT, { TA_SA, TA_PULLI, TA_RA, TA_SIGN_II } },  /* sri */
    /* Punctuation moved by the layout. */
    { '~', T_TEXT, { '*' } },
    { '@', T_TEXT, { '"' } },
    { '#', T_TEXT, { '%' } },
    { '*', T_TEXT, { '\'' } },
    { '.', T_TEXT, { ',' } },
    { '/', T_TEXT, { '.' } },
    { '>', T_TEXT, { '?' } },
    { '?', T_TEXT, { '-' } },
    { '|', T_TEXT, { '!' } },
    /* No character on the typewriter. */
    { 'A', T_NONE, { 0 } },
    { 'Z', T_NONE, { 0 } },
};

static const tw_key_t *find_key(char c)
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

static bool is_pre_sign(uint32_t cp)
{
    return cp == TA_SIGN_E || cp == TA_SIGN_EE || cp == TA_SIGN_AI;
}

static bool tw_key(uint8_t keycode, uint8_t modifier, const char *text)
{
    (void)keycode;
    (void)modifier;
    char c = ta_key_char(text);
    const tw_key_t *k = c ? find_key(c) : NULL;
    if (!k) return false;
    if (k->kind == T_NONE) return true;

    uint32_t prev[3] = { 0, 0, 0 };
    uint8_t plen[3] = { 0, 0, 0 };
    size_t pn = ta_peek(prev, plen, 3);
    /* p1 = the code point right before the cursor, p2 the one before. */
    uint32_t p1 = pn >= 1 ? prev[pn - 1] : 0;
    uint32_t p2 = pn >= 2 ? prev[pn - 2] : 0;
    size_t b1 = pn >= 1 ? plen[pn - 1] : 0;
    size_t n = cps_len(k->cps);

    switch (k->kind) {
    case T_CONS:
    case T_LLA:
        if (k->kind == T_LLA) {
            /* e sign + lla piece = au sign; o + lla = au. */
            if (p1 == TA_SIGN_E && ta_is_consonant(p2)) {
                uint32_t cp = TA_SIGN_AU;
                ta_put(b1, &cp, 1);
                return true;
            }
            if (p1 == 0x0B92) {
                uint32_t cp = 0x0B94;
                ta_put(b1, &cp, 1);
                return true;
            }
        }
        if (is_pre_sign(p1) && !ta_is_consonant(p2)) {
            /* The sign typed first goes after the consonant. */
            uint32_t buf[5];
            memcpy(buf, k->cps, n * sizeof(uint32_t));
            buf[n] = p1;
            ta_put(b1, buf, n + 1);
            return true;
        }
        ta_put(0, k->cps, n);
        return true;

    case T_AA:
        if ((p1 == TA_SIGN_E || p1 == TA_SIGN_EE) && ta_is_consonant(p2)) {
            uint32_t cp = p1 == TA_SIGN_E ? TA_SIGN_O : TA_SIGN_OO;
            ta_put(b1, &cp, 1);
            return true;
        }
        /* Otherwise a sign like the others below. */
        __attribute__((fallthrough));
    case T_SIGN:
        if (p1 == TA_SIGN_AU && ta_is_consonant(p2)) {
            /* That was e sign + lla, not au. */
            uint32_t buf[3] = { TA_SIGN_E, 0x0BB3, k->cps[0] };
            ta_put(b1, buf, 3);
            return true;
        }
        if (p1 == 0x0B94) {
            uint32_t buf[3] = { 0x0B92, 0x0BB3, k->cps[0] };
            ta_put(b1, buf, 3);
            return true;
        }
        ta_put(0, k->cps, n);
        return true;

    case T_UU_HOOK:
        if (p1 == TA_SIGN_U) {
            uint32_t cp = TA_SIGN_UU;
            ta_put(b1, &cp, 1);
            return true;
        }
        ta_put(0, k->cps, n);
        return true;

    case T_PRE:
    case T_TEXT:
    default:
        ta_put(0, k->cps, n);
        return true;
    }
}

static void tw_commit(void)
{
}

static bool tw_composing(void)
{
    return false;
}

static const char *tw_status(void)
{
    return NULL;
}

const ime_engine_t ime_tamil_tw_engine = {
    tw_key, tw_commit, tw_composing, tw_status,
};
