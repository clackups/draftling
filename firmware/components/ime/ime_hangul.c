/*
 * Korean 2-set (Dubeolsik) Hangul composition.
 *
 * The Korean layout (kb_layout.cpp) produces Hangul Compatibility Jamo;
 * this engine composes them into syllables (U+AC00 + (L*21 + V)*28 + T)
 * in place, one syllable at a time:
 *
 *   consonant           starts a syllable, or becomes / extends its final
 *   vowel               completes the syllable, or combines (o + a -> wa)
 *   vowel after a final the final moves on to start the next syllable
 *                       (gag + a -> ga ga), a double final is split
 *   Backspace           removes the last jamo typed, not the syllable
 *
 * Anything that is not a jamo ends the composition.
 */

#include <string.h>
#include "ime_internal.h"

/* Compatibility jamo U+3131..U+3163 -> initial index (0..18), or -1. */
static const int8_t JAMO_TO_L[51] = {
    0, 1, -1, 2, -1, -1, 3, 4, 5, -1, -1, -1, -1, -1, -1, -1,
    6, 7, 8, -1, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
    /* vowels */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};
/* Compatibility jamo -> final index (1..27), or 0. */
static const int8_t JAMO_TO_T[51] = {
    1, 2, 3, 4, 5, 6, 7, 0, 8, 9, 10, 11, 12, 13, 14, 15,
    16, 17, 0, 18, 19, 20, 21, 22, 0, 23, 24, 25, 26, 27,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
/* Initial index -> compatibility jamo. */
static const uint16_t L_TO_JAMO[19] = {
    0x3131, 0x3132, 0x3134, 0x3137, 0x3138, 0x3139, 0x3141, 0x3142,
    0x3143, 0x3145, 0x3146, 0x3147, 0x3148, 0x3149, 0x314A, 0x314B,
    0x314C, 0x314D, 0x314E,
};
/* Final index -> initial index when it moves to the next syllable
 * (single finals only; -1 for doubles, handled by split). */
static const int8_t T_TO_L[28] = {
    -1, 0, 1, -1, 2, -1, -1, 3, 5, -1, -1, -1, -1, -1, -1, -1,
    6, 7, -1, 9, 10, 11, 12, 14, 15, 16, 17, 18,
};
/* Double finals: (first, second) as final indexes -> combined. */
static const int8_t T_COMBINE[][3] = {
    { 1, 19, 3 },    /* g + s  */
    { 4, 22, 5 },    /* n + j  */
    { 4, 27, 6 },    /* n + h  */
    { 8, 1, 9 },     /* r + g  */
    { 8, 16, 10 },   /* r + m  */
    { 8, 17, 11 },   /* r + b  */
    { 8, 19, 12 },   /* r + s  */
    { 8, 25, 13 },   /* r + t  */
    { 8, 26, 14 },   /* r + p  */
    { 8, 27, 15 },   /* r + h  */
    { 17, 19, 18 },  /* b + s  */
};
/* Vowel index (0..20) combinations. */
static const int8_t V_COMBINE[][3] = {
    { 8, 0, 9 },     /* o + a  -> wa  */
    { 8, 1, 10 },    /* o + ae -> wae */
    { 8, 20, 11 },   /* o + i  -> oe  */
    { 13, 4, 14 },   /* u + eo -> wo  */
    { 13, 5, 15 },   /* u + e  -> we  */
    { 13, 20, 16 },  /* u + i  -> wi  */
    { 18, 20, 19 },  /* eu + i -> ui  */
};

static int t_combine(int a, int b)
{
    for (size_t i = 0; i < sizeof(T_COMBINE) / sizeof(T_COMBINE[0]); i++) {
        if (T_COMBINE[i][0] == a && T_COMBINE[i][1] == b) return T_COMBINE[i][2];
    }
    return -1;
}

static bool t_split(int t, int *first, int *second)
{
    for (size_t i = 0; i < sizeof(T_COMBINE) / sizeof(T_COMBINE[0]); i++) {
        if (T_COMBINE[i][2] == t) {
            *first = T_COMBINE[i][0];
            *second = T_COMBINE[i][1];
            return true;
        }
    }
    return false;
}

static int v_combine(int a, int b)
{
    for (size_t i = 0; i < sizeof(V_COMBINE) / sizeof(V_COMBINE[0]); i++) {
        if (V_COMBINE[i][0] == a && V_COMBINE[i][1] == b) return V_COMBINE[i][2];
    }
    return -1;
}

/* Final index -> initial index of its jamo (for a final shown alone). */
static int t_to_l_any(int t)
{
    return (t >= 0 && t < 28) ? T_TO_L[t] : -1;
}

typedef struct {
    int8_t l;    /* initial, -1 none */
    int8_t v;    /* vowel, -1 none */
    int8_t t;    /* final, 0 none */
} syl_t;

#define HIST_MAX 8
static syl_t s_hist[HIST_MAX];   /* states before each jamo of the syllable */
static int   s_hist_n;
static syl_t s_cur;
static bool  s_active;

static size_t syl_render(const syl_t *s, char *out)
{
    uint32_t cp;
    if (s->l >= 0 && s->v >= 0) {
        cp = 0xAC00 + ((uint32_t)s->l * 21 + (uint32_t)s->v) * 28 + (uint32_t)s->t;
    } else if (s->l >= 0) {
        cp = L_TO_JAMO[s->l];
    } else if (s->v >= 0) {
        cp = 0x314F + (uint32_t)s->v;
    } else {
        out[0] = '\0';
        return 0;
    }
    size_t n = utf8_encode(cp, out);
    out[n] = '\0';
    return n;
}

static void reset(void)
{
    s_cur.l = -1;
    s_cur.v = -1;
    s_cur.t = 0;
    s_hist_n = 0;
    s_active = false;
}

static void push_hist(void)
{
    if (s_hist_n < HIST_MAX) s_hist[s_hist_n++] = s_cur;
}

/* Show `done` (finished syllable text) followed by the current one. */
static bool show(const char *done)
{
    char cur[8];
    syl_render(&s_cur, cur);
    if (done && done[0]) {
        /* The finished part leaves the composition. */
        char both[24];
        size_t dl = strlen(done);
        memcpy(both, done, dl);
        strcpy(both + dl, cur);
        if (!comp_show(both)) return false;
        /* Keep only the current syllable as the composition. */
        comp_drop_front(dl);
        return true;
    }
    return comp_show(cur);
}

static bool start_new(const syl_t *next, const char *done_text)
{
    s_cur = *next;
    s_hist_n = 0;
    syl_t empty = { -1, -1, 0 };
    s_hist[s_hist_n++] = empty;
    return show(done_text);
}

static bool hangul_key(uint8_t keycode, uint8_t modifier, const char *text)
{
    (void)modifier;
    if (keycode == IME_KEY_BACKSPACE && s_active) {
        if (s_hist_n > 0) {
            s_cur = s_hist[--s_hist_n];
            char cur[8];
            syl_render(&s_cur, cur);
            comp_show(cur);
            if (s_cur.l < 0 && s_cur.v < 0) reset();
            return true;
        }
        reset();
        return false;
    }

    uint32_t cp = 0;
    if (text) {
        size_t n;
        cp = utf8_decode(text, strlen(text), &n);
        if (n != strlen(text)) cp = 0;
    }
    if (cp < 0x3131 || cp > 0x3163) {
        reset();          /* not a jamo: finish the syllable */
        return false;
    }
    int idx = (int)(cp - 0x3131);
    bool vowel = cp >= 0x314F;

    if (!s_active) {
        reset();
        s_active = true;
        syl_t next = { -1, -1, 0 };
        if (vowel) next.v = (int8_t)(cp - 0x314F);
        else next.l = JAMO_TO_L[idx];
        if (!vowel && next.l < 0) {
            /* A double-consonant jamo that cannot start a syllable
             * (only typed via Shift on some layouts): insert as is. */
            reset();
            return false;
        }
        return start_new(&next, NULL);
    }

    char done[8];
    if (!vowel) {
        int l = JAMO_TO_L[idx];
        int t = JAMO_TO_T[idx];
        if (s_cur.v < 0 || s_cur.l < 0) {
            /* Lone consonant or lone vowel: finish it, start anew. */
            syl_render(&s_cur, done);
            syl_t next = { (int8_t)l, -1, 0 };
            if (l < 0) { reset(); comp_forget(); return false; }
            return start_new(&next, done);
        }
        if (s_cur.t == 0 && t > 0) {
            push_hist();
            s_cur.t = (int8_t)t;
            return show(NULL);
        }
        if (s_cur.t > 0) {
            int c = t > 0 ? t_combine(s_cur.t, t) : -1;
            if (c > 0) {
                push_hist();
                s_cur.t = (int8_t)c;
                return show(NULL);
            }
        }
        syl_render(&s_cur, done);
        if (l < 0) { reset(); comp_forget(); return false; }
        syl_t next = { (int8_t)l, -1, 0 };
        return start_new(&next, done);
    }

    int v = (int)(cp - 0x314F);
    if (s_cur.l >= 0 && s_cur.v < 0) {
        push_hist();
        s_cur.v = (int8_t)v;
        return show(NULL);
    }
    if (s_cur.v >= 0 && s_cur.t == 0) {
        int c = v_combine(s_cur.v, v);
        if (c >= 0) {
            push_hist();
            s_cur.v = (int8_t)c;
            return show(NULL);
        }
        syl_render(&s_cur, done);
        syl_t next = { -1, (int8_t)v, 0 };
        return start_new(&next, done);
    }
    if (s_cur.t > 0) {
        /* The final moves to the next syllable. */
        int first, second;
        syl_t prev = s_cur;
        int moved;
        if (t_split(s_cur.t, &first, &second)) {
            prev.t = (int8_t)first;
            moved = t_to_l_any(second);
        } else {
            prev.t = 0;
            moved = t_to_l_any(s_cur.t);
        }
        syl_render(&prev, done);
        syl_t next = { (int8_t)moved, (int8_t)v, 0 };
        if (moved < 0) next.l = -1;
        bool ok = start_new(&next, done);
        /* Backspace from here restores the consonant-only state. */
        s_hist_n = 0;
        syl_t lonely = { (int8_t)moved, -1, 0 };
        s_hist[s_hist_n++] = (syl_t){ -1, -1, 0 };
        if (moved >= 0) s_hist[s_hist_n++] = lonely;
        return ok;
    }
    /* Lone vowel followed by a vowel that does not combine. */
    syl_render(&s_cur, done);
    syl_t next = { -1, (int8_t)v, 0 };
    return start_new(&next, done);
}

static void hangul_commit(void)
{
    reset();
}

static bool hangul_composing(void)
{
    return s_active;
}

static const char *hangul_status(void)
{
    return NULL;
}

const ime_engine_t ime_hangul_engine = {
    hangul_key, hangul_commit, hangul_composing, hangul_status,
};
