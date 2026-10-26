#pragma once

/* Shared helpers of the input-method engines (not public API). */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "ime.h"

#ifdef __cplusplus
extern "C" {
#endif

/* HID keycodes used by the engines (same values as ble_keyboard.h). */
#define IME_KEY_ENTER     0x28
#define IME_KEY_ESCAPE    0x29
#define IME_KEY_BACKSPACE 0x2A
#define IME_KEY_TAB       0x2B
#define IME_KEY_SPACE     0x2C
#define IME_KEY_RIGHT     0x4F
#define IME_KEY_LEFT      0x50
#define IME_KEY_DOWN      0x51
#define IME_KEY_UP        0x52
#define IME_KEY_PAGEUP    0x4B
#define IME_KEY_PAGEDOWN  0x4E
#define IME_KEY_KP_ENTER  0x58
#define IME_MOD_SHIFT     0x22

/* ---- Host access ---- */
size_t ime_peek(char *buf, size_t max);
bool   ime_replace(size_t del, const char *text);

/* ---- Composition tracking ----
 * The composition is the text the engine inserted right before the
 * cursor. comp_valid() checks that it is still there. */
#define IME_COMP_MAX 256
extern char   g_comp[IME_COMP_MAX];
extern size_t g_comp_len;
bool comp_valid(void);
/* Replace the composition shown in the document with `text` (which
 * becomes the new composition). */
bool comp_show(const char *text);
/* Forget the composition; the text stays in the document. */
void comp_forget(void);

/* ---- Strings ----
 * Copy / concatenate into a buffer of `size` bytes, truncating (at a
 * byte, callers keep their inputs short enough). */
void ime_copy(char *out, size_t size, const char *src);
void ime_join(char *out, size_t size, const char *a, const char *b);
/* Drop the first `n` bytes of the composition (text that has been
 * accepted and no longer belongs to it). */
void comp_drop_front(size_t n);

/* ---- UTF-8 ---- */
/* Decode one code point at s (len bytes available); *n = bytes used. */
uint32_t utf8_decode(const char *s, size_t len, size_t *n);
/* Encode cp, return bytes written (out needs 4). */
size_t utf8_encode(uint32_t cp, char *out);
/* Byte length of the last code point of s[0..len). */
size_t utf8_last_len(const char *s, size_t len);
bool is_wide_cp(uint32_t cp);

/* ---- Candidate list ---- */
#define IME_CAND_MAX     64
#define IME_CAND_TEXT    40
typedef struct {
    char text[IME_CAND_TEXT];
    uint8_t span;          /* engine-specific: input consumed */
} ime_cand_t;

typedef struct {
    ime_cand_t c[IME_CAND_MAX];
    int count;
    int sel;               /* selected candidate */
    int page_start[IME_CAND_MAX + 1];
    int pages;
} ime_cands_t;

void cands_clear(ime_cands_t *cl);
/* Append unless already present. Returns false when full. */
bool cands_add(ime_cands_t *cl, const char *text, size_t len, uint8_t span);
/* Split into pages fitting `cells` with the prefix shown first. */
void cands_paginate(ime_cands_t *cl, int prefix_cells);
int  cands_page_of(const ime_cands_t *cl, int idx);
/* Format "prefix 1.xx 2.yy ..." for the page holding sel into out. */
void cands_format(const ime_cands_t *cl, const char *prefix, char *out, size_t out_size);
/* Index of the candidate picked with digit d (1..9) on the current
 * page, or -1. */
int  cands_digit(const ime_cands_t *cl, int d);
extern int g_status_cells;

/* ---- Dictionaries ---- */
typedef struct {
    const uint8_t *offs;   /* u32 offsets, count + 1 */
    const uint8_t *data;
    uint32_t count;
} ime_dict_t;

bool dict_open(ime_dict_t *d, const uint8_t *blob);
/* Look up an exact key. Returns false when absent. On success *cost
 * is the best candidate's cost, *ncand the candidate count and
 * *cands the first candidate (NUL-separated). */
bool dict_lookup(const ime_dict_t *d, const char *key, size_t key_len,
                 uint16_t *cost, uint8_t *ncand, const char **cands);

/* ---- Engines ---- */
typedef struct {
    bool (*key)(uint8_t keycode, uint8_t modifier, const char *text);
    void (*commit)(void);
    bool (*composing)(void);
    const char *(*status)(void);
} ime_engine_t;

extern const ime_engine_t ime_telex_engine;
extern const ime_engine_t ime_hangul_engine;
extern const ime_engine_t ime_kana_engine;
extern const ime_engine_t ime_pinyin_engine;

extern ime_dict_t g_ja_dict;
extern ime_dict_t g_zh_dict;

#ifdef __cplusplus
}
#endif
