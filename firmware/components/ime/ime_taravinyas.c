/*
 * Taravinyas Tamil input, after Bandhan Pramanik's Taravinyas input
 * engine (github.com/BandhanPramanik/tamil-input-engine-checkbox and
 * the preprint "Taravinyas: Mapping Paninian Phonology to Digital
 * Logic for Continuous Touchscreen Input",
 * doi:10.5281/zenodo.20467634). This is a separate implementation of
 * the method with the same keys as the engine's web page; no code of
 * that project (AGPL-3.0) is used.
 *
 * A letter is picked by where and how it is pronounced. V switches
 * between the consonant and the vowel mode (the mode stays until V
 * is pressed again).
 *
 * Consonants: the place of articulation, then the manner.
 *   F velar   D palatal   S retroflex   R dental   E labial   W alveolar
 *   J hard (vallinam)   K nasal (mellinam)   L medial (idaiyinam;
 *   the rhotic one where there are two)   U medial, the lateral one
 *   I sibilant (Grantha)
 *     F: J ka  K nga
 *     D: J ca  K nya  L/U ya  I sha
 *     S: J tta K nna  L llla U lla  I ssa
 *     R: J ta  K na
 *     E: J pa  K ma   L/U va
 *     W: J rra K nnna L ra  U la  I sa
 *   A then J / K / L: the Grantha ja, ha, k.ssa.
 *
 * Vowels: the vowel, its length, then sign or letter.
 *   F a   D i   S u   R e   E o
 *   J short   K long   L diphthong (e -> ai, o -> au only)
 *   X vowel sign (joins the consonant before; short a has none)
 *   C independent vowel
 *   A then J / K / L: pulli, aytham, zero-width non-joiner.
 *
 * The status bar shows the choices of the next key. An impossible
 * pick (e.g. a sibilant at the velar place) is refused there. Esc
 * drops a half-made pick, Backspace undoes its last step. Other
 * letter keys do nothing; digits, punctuation and Space type as
 * usual.
 */

#include <stdio.h>
#include <string.h>
#include "ime_tamil.h"

#define NO 0

/* Consonant of [place][manner]; manner: hard, nasal, medial rhotic,
 * medial lateral, sibilant. 0 = none. */
static const uint16_t CONS[6][5] = {
    { 0x0B95, 0x0B99, NO,     NO,     NO     },   /* F velar */
    { 0x0B9A, 0x0B9E, 0x0BAF, 0x0BAF, 0x0BB6 },   /* D palatal */
    { 0x0B9F, 0x0BA3, 0x0BB4, 0x0BB3, 0x0BB7 },   /* S retroflex */
    { 0x0BA4, 0x0BA8, NO,     NO,     NO     },   /* R dental */
    { 0x0BAA, 0x0BAE, 0x0BB5, 0x0BB5, NO     },   /* E labial */
    { 0x0BB1, 0x0BA9, 0x0BB0, 0x0BB2, 0x0BB8 },   /* W alveolar */
};
static const char PLACE_KEYS[] = "fdsrew";
static const char MANNER_KEYS[] = "jklui";
static const char *const PLACE_NAMES[] = {
    "velar", "palatal", "retroflex", "dental", "labial", "alveolar",
};

/* Vowel index (TA_VOWEL_*) of [vowel][length]; length: short, long,
 * diphthong. -1 = none. */
static const int8_t VOWEL[5][3] = {
    { 0, 1, -1 },     /* F a */
    { 2, 3, -1 },     /* D i */
    { 4, 5, -1 },     /* S u */
    { 6, 7, 8 },      /* R e */
    { 9, 10, 11 },    /* E o */
};
static const char VOWEL_KEYS[] = "fdsre";
static const char LENGTH_KEYS[] = "jkl";

enum { MODE_CONS = 0, MODE_VOWEL };

static uint8_t s_mode = MODE_CONS;
static bool    s_extended;          /* A pressed */
static int8_t  s_place = -1;        /* consonant place / vowel */
static int8_t  s_length = -1;       /* vowel length */
static const char *s_error;
static bool    s_show;              /* status shown (see tv_composing()) */
static char    s_status[160];

static int key_index(const char *keys, char c)
{
    const char *p = strchr(keys, c);
    return (c && p) ? (int)(p - keys) : -1;
}

static void clear_pick(void)
{
    s_extended = false;
    s_place = -1;
    s_length = -1;
}

static bool picking(void)
{
    return s_extended || s_place >= 0;
}

static void emit(const uint32_t *cps, size_t n)
{
    if (n) ta_put(0, cps, n);
    clear_pick();
    s_show = false;
}

/* Append a Tamil code point to the status text. */
static void st_cp(size_t *pos, uint32_t cp)
{
    char tmp[8];
    ta_encode(&cp, 1, tmp, sizeof(tmp));
    size_t l = strlen(tmp);
    if (*pos + l + 1 < sizeof(s_status)) {
        memcpy(s_status + *pos, tmp, l);
        *pos += l;
        s_status[*pos] = '\0';
    }
}

static void st_str(size_t *pos, const char *s)
{
    size_t l = strlen(s);
    if (*pos + l + 1 < sizeof(s_status)) {
        memcpy(s_status + *pos, s, l);
        *pos += l;
        s_status[*pos] = '\0';
    }
}

static void build_status(void)
{
    size_t pos = 0;
    s_status[0] = '\0';
    if (s_error) {
        st_str(&pos, s_error);
        st_str(&pos, " | ");
    }
    if (s_mode == MODE_CONS) {
        if (s_extended) {
            st_str(&pos, "Grantha: J ");
            st_cp(&pos, 0x0B9C);
            st_str(&pos, " K ");
            st_cp(&pos, 0x0BB9);
            st_str(&pos, " L ");
            st_cp(&pos, TA_KA);
            st_cp(&pos, TA_PULLI);
            st_cp(&pos, TA_SSA);
        } else if (s_place >= 0) {
            st_str(&pos, PLACE_NAMES[s_place]);
            st_str(&pos, ":");
            static const char *const MK[] = { " J ", " K ", " L ", " U ", " I " };
            for (int m = 0; m < 5; m++) {
                if (!CONS[s_place][m]) continue;
                if (m == 3 && CONS[s_place][3] == CONS[s_place][2]) continue;
                st_str(&pos, MK[m]);
                st_cp(&pos, CONS[s_place][m]);
            }
        } else {
            st_str(&pos, "Consonant: F velar D palatal S retroflex R dental "
                         "E labial W alveolar, A Grantha, V vowels");
        }
    } else {
        if (s_extended) {
            st_str(&pos, "J pulli K aytham L ZWNJ");
        } else if (s_place >= 0 && s_length >= 0) {
            int v = VOWEL[s_place][s_length];
            st_str(&pos, "X sign");
            if (TA_VOWEL_SIGN[v]) {
                st_str(&pos, " ");
                st_cp(&pos, TA_DOTTED_CIRCLE);
                st_cp(&pos, TA_VOWEL_SIGN[v]);
            }
            st_str(&pos, ", C letter ");
            st_cp(&pos, TA_VOWEL_LETTER[v]);
        } else if (s_place >= 0) {
            st_str(&pos, "J short ");
            st_cp(&pos, TA_VOWEL_LETTER[VOWEL[s_place][0]]);
            st_str(&pos, " K long ");
            st_cp(&pos, TA_VOWEL_LETTER[VOWEL[s_place][1]]);
            if (VOWEL[s_place][2] >= 0) {
                st_str(&pos, " L ");
                st_cp(&pos, TA_VOWEL_LETTER[VOWEL[s_place][2]]);
            }
        } else {
            st_str(&pos, "Vowel: F ");
            st_cp(&pos, 0x0B85);
            st_str(&pos, " D ");
            st_cp(&pos, 0x0B87);
            st_str(&pos, " S ");
            st_cp(&pos, 0x0B89);
            st_str(&pos, " R ");
            st_cp(&pos, 0x0B8E);
            st_str(&pos, " E ");
            st_cp(&pos, 0x0B92);
            st_str(&pos, ", A other, V consonants");
        }
    }
}

static void key_consonant(char c)
{
    if (s_extended) {
        int i = key_index("jkl", c);
        if (i == 0) { uint32_t cp = 0x0B9C; emit(&cp, 1); }
        else if (i == 1) { uint32_t cp = 0x0BB9; emit(&cp, 1); }
        else if (i == 2) {
            uint32_t cps[3] = { TA_KA, TA_PULLI, TA_SSA };
            emit(cps, 3);
        }
        return;
    }
    int p = key_index(PLACE_KEYS, c);
    if (p >= 0) {
        s_place = (int8_t)p;
        return;
    }
    int m = key_index(MANNER_KEYS, c);
    if (m >= 0 && s_place >= 0) {
        uint32_t cp = CONS[s_place][m];
        if (!cp) {
            s_error = "not at this place";
            return;
        }
        emit(&cp, 1);
    }
}

static void key_vowel(char c)
{
    if (s_extended) {
        static const uint32_t OTHER[3] = { TA_PULLI, TA_AYTHAM, TA_ZWNJ };
        int i = key_index("jkl", c);
        if (i >= 0) emit(&OTHER[i], 1);
        return;
    }
    int v = key_index(VOWEL_KEYS, c);
    if (v >= 0) {
        s_place = (int8_t)v;
        s_length = -1;
        return;
    }
    if (s_place < 0) return;
    int l = key_index(LENGTH_KEYS, c);
    if (l >= 0) {
        if (VOWEL[s_place][l] < 0) {
            s_error = "no diphthong of this vowel";
            return;
        }
        s_length = (int8_t)l;
        return;
    }
    if (s_length < 0) return;
    int idx = VOWEL[s_place][s_length];
    if (c == 'x') {
        uint32_t cp = TA_VOWEL_SIGN[idx];
        emit(&cp, cp ? 1 : 0);     /* short a: the consonant has it */
    } else if (c == 'c') {
        uint32_t cp = TA_VOWEL_LETTER[idx];
        emit(&cp, 1);
    }
}

static bool tv_key(uint8_t keycode, uint8_t modifier, const char *text)
{
    (void)modifier;
    s_error = NULL;

    if (keycode == IME_KEY_ESCAPE) {
        if (!picking()) {
            s_show = false;
            return false;
        }
        clear_pick();
        build_status();
        return true;
    }
    if (keycode == IME_KEY_BACKSPACE) {
        if (!picking()) {
            s_show = false;
            return false;
        }
        if (s_length >= 0) s_length = -1;
        else if (s_place >= 0) s_place = -1;
        else s_extended = false;
        build_status();
        return true;
    }

    char c = ta_key_char(text);
    if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
    if (!(c >= 'a' && c <= 'z')) {
        /* Digits, punctuation, Space, Enter, arrows: as usual. */
        clear_pick();
        s_show = false;
        return false;
    }

    s_show = true;

    if (c == 'v') {
        s_mode = s_mode == MODE_CONS ? MODE_VOWEL : MODE_CONS;
        clear_pick();
    } else if (c == 'a') {
        bool ext = !s_extended;
        clear_pick();
        s_extended = ext;
    } else if (s_mode == MODE_CONS) {
        key_consonant(c);
    } else {
        key_vowel(c);
    }
    build_status();
    return true;
}

static void tv_commit(void)
{
    clear_pick();
    s_error = NULL;
    s_show = false;
}

/* "Composing" from a method key until a letter is typed, so the status
 * bar shows the mode and the choices of the next key meanwhile. */
static bool tv_composing(void)
{
    return s_show;
}

static const char *tv_status(void)
{
    if (!s_status[0]) build_status();
    return s_status;
}

const ime_engine_t ime_taravinyas_engine = {
    tv_key, tv_commit, tv_composing, tv_status,
};
