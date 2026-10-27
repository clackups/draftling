/*
 * Chinese input: full pinyin -> hanzi (Simplified Chinese).
 *
 * Pinyin letters are typed into the text and split into syllables as
 * they come ("v" stands for u-umlaut: lv, nv; an apostrophe forces a
 * syllable break: xi'an). Candidates for the longest run of complete
 * syllables that has dictionary words are listed in the status bar,
 * followed by the words of shorter runs:
 *
 *   Space        accept the selected candidate
 *   1..9         accept a candidate of the current page
 *   Left/Right   select the previous / next candidate
 *   Up/Down, PageUp/PageDown, - and =   previous / next page
 *   Enter        keep the letters as typed
 *   Esc          drop the letters
 *   Backspace    delete the last letter
 *
 * Accepting a word replaces its syllables; any letters left over stay
 * in composition with fresh candidates. Outside a composition, , . ? !
 * : ; and \ type full-width Chinese punctuation.
 */

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "ime_internal.h"

/* Every syllable of the dictionary, sorted. */
static const char *const SYLLABLES[] = {
    "a", "ai", "an", "ang", "ao", "ba", "bai", "ban", "bang", "bao", "bei",
    "ben", "beng", "bi", "bian", "biao", "bie", "bin", "bing", "bo", "bu",
    "ca", "cai", "can", "cang", "cao", "ce", "cen", "ceng", "cha", "chai",
    "chan", "chang", "chao", "che", "chen", "cheng", "chi", "chong", "chou",
    "chu", "chua", "chuai", "chuan", "chuang", "chui", "chun", "chuo", "ci",
    "cong", "cou", "cu", "cuan", "cui", "cun", "cuo", "da", "dai", "dan",
    "dang", "dao", "de", "dei", "den", "deng", "di", "dia", "dian", "diao",
    "die", "ding", "diu", "dong", "dou", "du", "duan", "dui", "dun", "duo",
    "e", "ei", "en", "eng", "er", "fa", "fan", "fang", "fei", "fen", "feng",
    "fiao", "fo", "fou", "fu", "ga", "gai", "gan", "gang", "gao", "ge",
    "gei", "gen", "geng", "gong", "gou", "gu", "gua", "guai", "guan",
    "guang", "gui", "gun", "guo", "ha", "hai", "han", "hang", "hao", "he",
    "hei", "hen", "heng", "hm", "hng", "hong", "hou", "hu", "hua", "huai",
    "huan", "huang", "hui", "hun", "huo", "ji", "jia", "jian", "jiang",
    "jiao", "jie", "jin", "jing", "jiong", "jiu", "ju", "juan", "jue",
    "jun", "ka", "kai", "kan", "kang", "kao", "ke", "kei", "ken", "keng",
    "kong", "kou", "ku", "kua", "kuai", "kuan", "kuang", "kui", "kun",
    "kuo", "la", "lai", "lan", "lang", "lao", "le", "lei", "leng", "li",
    "lia", "lian", "liang", "liao", "lie", "lin", "ling", "liu", "lo",
    "long", "lou", "lu", "luan", "lue", "lun", "luo", "lv", "m", "ma",
    "mai", "man", "mang", "mao", "me", "mei", "men", "meng", "mi", "mian",
    "miao", "mie", "min", "ming", "miu", "mo", "mou", "mu", "n", "na",
    "nai", "nan", "nang", "nao", "ne", "nei", "nen", "neng", "ng", "ni",
    "nian", "niang", "niao", "nie", "nin", "ning", "niu", "nong", "nou",
    "nu", "nuan", "nue", "nuo", "nv", "o", "ou", "pa", "pai", "pan", "pang",
    "pao", "pei", "pen", "peng", "pi", "pian", "piao", "pie", "pin", "ping",
    "po", "pou", "pu", "qi", "qia", "qian", "qiang", "qiao", "qie", "qin",
    "qing", "qiong", "qiu", "qu", "quan", "que", "qun", "ran", "rang",
    "rao", "re", "ren", "reng", "ri", "rong", "rou", "ru", "ruan", "rui",
    "run", "ruo", "sa", "sai", "san", "sang", "sao", "se", "sen", "seng",
    "sha", "shai", "shan", "shang", "shao", "she", "shei", "shen", "sheng",
    "shi", "shou", "shu", "shua", "shuai", "shuan", "shuang", "shui",
    "shun", "shuo", "si", "song", "sou", "su", "suan", "sui", "sun", "suo",
    "ta", "tai", "tan", "tang", "tao", "te", "tei", "teng", "ti", "tian",
    "tiao", "tie", "ting", "tong", "tou", "tu", "tuan", "tui", "tun", "tuo",
    "wa", "wai", "wan", "wang", "wei", "wen", "weng", "wo", "wu", "xi",
    "xia", "xian", "xiang", "xiao", "xie", "xin", "xing", "xiong", "xiu",
    "xu", "xuan", "xue", "xun", "ya", "yan", "yang", "yao", "ye", "yi",
    "yin", "ying", "yo", "yong", "you", "yu", "yuan", "yue", "yun", "za",
    "zai", "zan", "zang", "zao", "ze", "zei", "zen", "zeng", "zha", "zhai",
    "zhan", "zhang", "zhao", "zhe", "zhei", "zhen", "zheng", "zhi", "zhong",
    "zhou", "zhu", "zhua", "zhuai", "zhuan", "zhuang", "zhui", "zhun",
    "zhuo", "zi", "zong", "zou", "zu", "zuan", "zui", "zun", "zuo",
};
#define SYL_COUNT (sizeof(SYLLABLES) / sizeof(SYLLABLES[0]))
#define SYL_MAX_LEN 6

#define PY_MAX 60         /* letters in a composition */
#define SEG_MAX 30

static char s_py[PY_MAX + 1];
static bool s_active;
static ime_cands_t s_cands;
static char s_status[256];

/* Syllable split of s_py: piece i is s_py[seg_start[i] .. seg_end[i]). */
static int  s_nseg;
static int  s_seg_start[SEG_MAX], s_seg_end[SEG_MAX];
static bool s_last_partial;

/* 1 = a syllable, 2 = a prefix of a longer one, 3 = both, 0 = neither. */
static int syl_match(const char *p, size_t len)
{
    int r = 0;
    for (size_t i = 0; i < SYL_COUNT; i++) {
        const char *s = SYLLABLES[i];
        if (strncmp(s, p, len) != 0) continue;
        if (s[len] == '\0') r |= 1;
        else r |= 2;
        if (r == 3) break;
    }
    return r;
}

/* Split s_py into syllables: fewest pieces, longest first; only the
 * last piece may be an incomplete syllable. */
static void segment(void)
{
    int len = (int)strlen(s_py);
    /* cost[i] = best cost of parsing s_py[i..]; next[i] = piece end. */
    int cost[PY_MAX + 2], next[PY_MAX + 2];
    cost[len] = 0;
    next[len] = len;
    for (int i = len - 1; i >= 0; i--) {
        cost[i] = 1 << 20;
        next[i] = i + 1;
        if (s_py[i] == '\'') { cost[i] = cost[i + 1]; next[i] = i + 1; continue; }
        for (int j = i + 1; j <= len && j - i <= SYL_MAX_LEN; j++) {
            if (s_py[j - 1] == '\'') break;
            int m = syl_match(s_py + i, (size_t)(j - i));
            if (m == 0) break;
            int c;
            if (m & 1) c = 10 + cost[j];
            else if (j == len) c = 11;            /* incomplete, at the end */
            else continue;
            if (c <= cost[i]) { cost[i] = c; next[i] = j; }
        }
        if (cost[i] >= (1 << 20)) { cost[i] = 1000 + cost[i + 1]; next[i] = i + 1; }
    }
    s_nseg = 0;
    s_last_partial = false;
    int i = 0;
    while (i < len && s_nseg < SEG_MAX) {
        if (s_py[i] == '\'') { i++; continue; }
        int j = next[i];
        s_seg_start[s_nseg] = i;
        s_seg_end[s_nseg] = j;
        s_nseg++;
        i = j;
    }
    if (s_nseg > 0) {
        int k = s_nseg - 1;
        int m = syl_match(s_py + s_seg_start[k], (size_t)(s_seg_end[k] - s_seg_start[k]));
        s_last_partial = !(m & 1);
    }
}

/* Number of leading pieces that are complete syllables. */
static int complete_pieces(void)
{
    int n = 0;
    while (n < s_nseg) {
        int m = syl_match(s_py + s_seg_start[n], (size_t)(s_seg_end[n] - s_seg_start[n]));
        if (!(m & 1)) break;
        n++;
    }
    return n;
}

static void build_candidates(void)
{
    cands_clear(&s_cands);
    segment();
    int full = complete_pieces();
    for (int k = full; k >= 1 && s_cands.count < IME_CAND_MAX; k--) {
        char key[PY_MAX * 2];
        size_t kl = 0;
        for (int i = 0; i < k; i++) {
            if (i) key[kl++] = ' ';
            int a = s_seg_start[i], b = s_seg_end[i];
            memcpy(key + kl, s_py + a, (size_t)(b - a));
            kl += (size_t)(b - a);
        }
        uint8_t n = 0;
        const char *c = NULL;
        if (!dict_lookup(&g_zh_dict, key, kl, NULL, &n, &c)) continue;
        /* Consume through the k-th piece and a following apostrophe. */
        int span = s_seg_end[k - 1];
        if (s_py[span] == '\'') span++;
        for (uint8_t i = 0; i < n; i++) {
            size_t l = strlen(c);
            if (!cands_add(&s_cands, c, l, (uint8_t)span)) break;
            c += l + 1;
        }
    }
}

static void status_prefix(char *out, size_t size)
{
    size_t pos = 0;
    for (int i = 0; i < s_nseg && pos + 8 < size; i++) {
        if (i) out[pos++] = '\'';
        int a = s_seg_start[i], b = s_seg_end[i];
        size_t l = (size_t)(b - a);
        if (pos + l + 1 >= size) break;
        memcpy(out + pos, s_py + a, l);
        pos += l;
    }
    out[pos] = '\0';
}

static void refresh(void)
{
    build_candidates();
    char prefix[PY_MAX * 2];
    status_prefix(prefix, sizeof(prefix));
    cands_paginate(&s_cands, ime_text_cells(prefix, strlen(prefix)));
}

static void reset(void)
{
    s_py[0] = '\0';
    s_active = false;
    cands_clear(&s_cands);
    s_nseg = 0;
}

static void finish(void)
{
    reset();
    comp_forget();
}

/* Accept candidate idx: its hanzi replace the letters it spans. */
static bool accept(int idx)
{
    const ime_cand_t *c = &s_cands.c[idx];
    int span = c->span;
    char rest[PY_MAX + 1];
    ime_copy(rest, sizeof(rest), s_py + span);
    char buf[IME_COMP_MAX];
    ime_join(buf, sizeof(buf), c->text, rest);
    if (!comp_show(buf)) return false;
    comp_drop_front(strlen(c->text));
    ime_copy(s_py, sizeof(s_py), rest);
    if (!s_py[0]) {
        finish();
    } else {
        refresh();
    }
    return true;
}

/* Full-width punctuation typed outside a composition. */
static const char *zh_punct(char c)
{
    switch (c) {
    case ',':  return "\xEF\xBC\x8C";   /* fullwidth comma */
    case '.':  return "\xE3\x80\x82";   /* ideographic full stop */
    case '?':  return "\xEF\xBC\x9F";
    case '!':  return "\xEF\xBC\x81";
    case ':':  return "\xEF\xBC\x9A";
    case ';':  return "\xEF\xBC\x9B";
    case '\\': return "\xE3\x80\x81";   /* ideographic comma */
    default:   return NULL;
    }
}

static void page_move(int dir)
{
    if (s_cands.count == 0) return;
    int p = cands_page_of(&s_cands, s_cands.sel) + dir;
    if (p < 0) p = s_cands.pages - 1;
    if (p >= s_cands.pages) p = 0;
    s_cands.sel = s_cands.page_start[p];
}

static bool pinyin_key(uint8_t keycode, uint8_t modifier, const char *text)
{
    (void)modifier;
    char c = (text && text[0] && !text[1]) ? text[0] : 0;

    if (!s_active) {
        if (c >= 'a' && c <= 'z') {
            reset();
            s_active = true;
            comp_forget();
            s_py[0] = c;
            s_py[1] = '\0';
            refresh();
            return comp_show(s_py);
        }
        const char *p = zh_punct(c);
        if (p) return ime_replace(0, p);
        return false;
    }

    if ((c >= 'a' && c <= 'z') || (c == '\'' && s_py[0])) {
        size_t l = strlen(s_py);
        if (l < PY_MAX) {
            s_py[l] = c;
            s_py[l + 1] = '\0';
            refresh();
            comp_show(s_py);
        }
        return true;
    }
    if (c >= '1' && c <= '9') {
        int idx = cands_digit(&s_cands, c - '0');
        if (idx >= 0) accept(idx);
        return true;
    }
    if (c == '-' || c == '=') {
        page_move(c == '-' ? -1 : 1);
        return true;
    }

    switch (keycode) {
    case IME_KEY_SPACE:
        if (s_cands.count > 0) accept(s_cands.sel);
        else { comp_show(s_py); finish(); }
        return true;
    case IME_KEY_ENTER:
    case IME_KEY_KP_ENTER:
        finish();                      /* letters stay as typed */
        return true;
    case IME_KEY_ESCAPE:
        comp_show("");
        finish();
        return true;
    case IME_KEY_BACKSPACE: {
        size_t l = strlen(s_py);
        if (l > 0) s_py[l - 1] = '\0';
        comp_show(s_py);
        if (!s_py[0]) finish();
        else refresh();
        return true;
    }
    case IME_KEY_LEFT:
        if (s_cands.count) s_cands.sel = (s_cands.sel + s_cands.count - 1) % s_cands.count;
        return true;
    case IME_KEY_RIGHT:
        if (s_cands.count) s_cands.sel = (s_cands.sel + 1) % s_cands.count;
        return true;
    case IME_KEY_UP:
    case IME_KEY_PAGEUP:
        page_move(-1);
        return true;
    case IME_KEY_DOWN:
    case IME_KEY_PAGEDOWN:
        page_move(1);
        return true;
    default:
        break;
    }

    /* Punctuation accepts the selected word first, then is typed. */
    const char *p = zh_punct(c);
    if (p) {
        if (s_cands.count > 0) {
            while (s_active && s_cands.count > 0) accept(s_cands.sel);
        }
        finish();
        return ime_replace(0, p);
    }
    /* Anything else: keep the letters, let the editor handle the key. */
    finish();
    return false;
}

static void pinyin_commit(void)
{
    reset();
}

static bool pinyin_composing(void)
{
    return s_active;
}

static const char *pinyin_status(void)
{
    if (!s_active) return NULL;
    char prefix[PY_MAX * 2];
    status_prefix(prefix, sizeof(prefix));
    cands_format(&s_cands, prefix, s_status, sizeof(s_status));
    return s_status;
}

const ime_engine_t ime_pinyin_engine = {
    pinyin_key, pinyin_commit, pinyin_composing, pinyin_status,
};
