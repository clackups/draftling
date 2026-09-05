/*
 * Keyboard layout translation module.
 *
 * Translates HID keycodes + modifier flags into UTF-8 strings
 * for US-English, Ukrainian, German, and French layouts.
 *
 * Non-ASCII characters are encoded as UTF-8 byte sequences using
 * hex escapes to keep the source file ASCII-only.
 */

#include <cstring>
#include "sdkconfig.h"
#include "kb_layout.h"

/* Modifier flags (same as ble_keyboard.h) */
#define MOD_LSHIFT 0x02
#define MOD_RSHIFT 0x20
#define MOD_RALT   0x40  /* AltGr on international keyboards */

static kb_layout_id_t s_layout = (kb_layout_id_t)0;

/* HID keycodes for letter/number/symbol keys: 0x04..0x38 (53 keys) */
#define KC_A     0x04
#define KC_Z     0x1D
#define KC_1     0x1E
#define KC_0     0x27
#define KC_ENTER 0x28
#define KC_SPACE 0x2C
#define KC_MINUS 0x2D
#define KC_EQUAL 0x2E
#define KC_LBRAC 0x2F
#define KC_RBRAC 0x30
#define KC_BSLSH 0x31
#define KC_HASH  0x32  /* non-US # */
#define KC_SCOLN 0x33
#define KC_QUOTE 0x34
#define KC_GRAVE 0x35
#define KC_COMMA 0x36
#define KC_DOT   0x37
#define KC_SLASH 0x38

#ifdef CONFIG_KB_LAYOUT_ENABLE_US
/* ------------------------------------------------------------------ */
/* US-English layout                                                  */
/* ------------------------------------------------------------------ */

/* unshifted: keycodes 0x04..0x38 */
static const char *US_NORMAL[] = {
    "a","b","c","d","e","f","g","h","i","j","k","l",  /* 04-0F */
    "m","n","o","p","q","r","s","t","u","v","w","x",  /* 10-1B */
    "y","z",                                          /* 1C-1D */
    "1","2","3","4","5","6","7","8","9","0",           /* 1E-27 */
    NULL,NULL,NULL,NULL," ",                           /* 28-2C */
    "-","=","[","]","\\",                              /* 2D-31 */
    NULL,";","'","`",",",".","/",                      /* 32-38 */
};

/* shifted */
static const char *US_SHIFT[] = {
    "A","B","C","D","E","F","G","H","I","J","K","L",
    "M","N","O","P","Q","R","S","T","U","V","W","X",
    "Y","Z",
    "!","@","#","$","%","^","&","*","(",")",
    NULL,NULL,NULL,NULL," ",
    "_","+","{","}","|",
    NULL,":","\x22","~","<",">","?",
};
#endif /* CONFIG_KB_LAYOUT_ENABLE_US */

#ifdef CONFIG_KB_LAYOUT_ENABLE_DE
/* ------------------------------------------------------------------ */
/* German layout                                                      */
/* ------------------------------------------------------------------ */

/* German remaps Y<->Z, and several symbol keys differ.
 * Umlauts use UTF-8 encoding:
 *   ae = \xC3\xA4,  AE = \xC3\x84
 *   oe = \xC3\xB6,  OE = \xC3\x96
 *   ue = \xC3\xBC,  UE = \xC3\x9C
 *   ss = \xC3\x9F
 */

static const char *DE_NORMAL[] = {
    "a","b","c","d","e","f","g","h","i","j","k","l",  /* 04-0F */
    "m","n","o","p","q","r","s","t","u","v","w","x",  /* 10-1B */
    "z","y",                                          /* 1C-1D: Y and Z swapped */
    "1","2","3","4","5","6","7","8","9","0",           /* 1E-27 */
    NULL,NULL,NULL,NULL," ",                           /* 28-2C */
    "\xC3\x9F",                                       /* 2D: ss (Eszett) */
    "\xC2\xB4",                                       /* 2E: acute accent */
    "\xC3\xBC",                                       /* 2F: ue */
    "+",                                              /* 30 */
    "#",                                              /* 31 */
    NULL,                                             /* 32 */
    "\xC3\xB6",                                       /* 33: oe */
    "\xC3\xA4",                                       /* 34: ae */
    "^",                                              /* 35 */
    ",",".","-",                                       /* 36-38 */
};

static const char *DE_SHIFT[] = {
    "A","B","C","D","E","F","G","H","I","J","K","L",
    "M","N","O","P","Q","R","S","T","U","V","W","X",
    "Z","Y",
    "!","\x22",                                       /* 1E-1F */
    "\xC2\xA7",                                       /* 20: section sign */
    "$","%","&","/","(",")",                           /* 21-27 */
    "=",                                              /* (shifted 0) - stored at idx 9 */
    NULL,NULL,NULL,NULL," ",
    "?","`",
    "\xC3\x9C",                                       /* 2F: Ue */
    "*","'",NULL,
    "\xC3\x96",                                       /* 33: Oe */
    "\xC3\x84",                                       /* 34: Ae */
    "\xC2\xB0",                                       /* 35: degree sign */
    ";",":",                                          /* 36-37 */
    "_",                                              /* 38 */
};

/* German AltGr layer (Right Alt) */
static const char *DE_ALTGR[] = {
    NULL,NULL,NULL,NULL,                              /* 04-07 */
    "\xe2\x82\xac",                                   /* 08 (e) -> Euro sign */
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,                /* 09-0F */
    NULL,NULL,NULL,NULL,                              /* 10-13 */
    "@",                                              /* 14 (q) -> @ */
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,                /* 15-1B */
    NULL,NULL,                                        /* 1C-1D */
    NULL,                                             /* 1E */
    "\xC2\xB2",                                       /* 1F (2) -> superscript 2 */
    "\xC2\xB3",                                       /* 20 (3) -> superscript 3 */
    NULL,NULL,NULL,                                   /* 21-23 */
    "{",                                              /* 24 (7) -> { */
    "[",                                              /* 25 (8) -> [ */
    "]",                                              /* 26 (9) -> ] */
    "}",                                              /* 27 (0) -> } */
    NULL,NULL,NULL,NULL,NULL,                          /* 28-2C */
    "\\",                                             /* 2D -> backslash */
    NULL,NULL,                                        /* 2E-2F */
    "~",                                              /* 30 -> ~ */
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,           /* 31-38 */
};
#endif /* CONFIG_KB_LAYOUT_ENABLE_DE */

#ifdef CONFIG_KB_LAYOUT_ENABLE_FR
/* ------------------------------------------------------------------ */
/* French (AZERTY) layout                                             */
/* ------------------------------------------------------------------ */
/* French remaps many keys: A<->Q, Z<->W, M moves, numbers are shifted.
 * Accented chars use UTF-8:
 *   e-grave = \xC3\xA8,  E-grave = \xC3\x88
 *   e-acute = \xC3\xA9
 *   a-grave = \xC3\xA0
 *   u-grave = \xC3\xB9
 *   c-cedilla = \xC3\xA7, C-cedilla = \xC3\x87
 */

static const char *FR_NORMAL[] = {
    "q","b","c","d","e","f","g","h","i","j","k","l",  /* 04-0F: a->q */
    ",","n","o","p","a","r","s","t","u","v","z","x",  /* 10-1B: m->,q->a,w->z */
    "y","w",                                          /* 1C-1D: z->w (AZERTY) */
    "&",                                              /* 1E: 1 -> & */
    "\xC3\xA9",                                       /* 1F: 2 -> e-acute */
    "\x22",                                           /* 20: 3 -> " */
    "'",                                              /* 21: 4 -> ' */
    "(",                                              /* 22: 5 -> ( */
    "-",                                              /* 23: 6 -> - */
    "\xC3\xA8",                                       /* 24: 7 -> e-grave */
    "_",                                              /* 25: 8 -> _ */
    "\xC3\xA7",                                       /* 26: 9 -> c-cedilla */
    "\xC3\xA0",                                       /* 27: 0 -> a-grave */
    NULL,NULL,NULL,NULL," ",                           /* 28-2C */
    ")",                                              /* 2D -> ) */
    "=",                                              /* 2E -> = */
    "^",                                              /* 2F */
    "$",                                              /* 30 */
    "*",                                              /* 31 */
    "<",                                              /* 32: non-US key */
    "m",                                              /* 33: semicolon -> m */
    "\xC3\xB9",                                       /* 34: quote -> u-grave */
    "\xC2\xB2",                                       /* 35: grave -> superscript 2 */
    ";",":",                                          /* 36-37 */
    "!",                                              /* 38 */
};

static const char *FR_SHIFT[] = {
    "Q","B","C","D","E","F","G","H","I","J","K","L",
    "?","N","O","P","A","R","S","T","U","V","Z","X",
    "Y","W",
    "1","2","3","4","5","6","7","8","9","0",           /* 1E-27: digits */
    NULL,NULL,NULL,NULL," ",
    "\xC2\xB0",                                       /* 2D: ) shifted -> degree */
    "+",                                              /* 2E */
    "\xC2\xA8",                                       /* 2F: diaeresis */
    "\xC2\xA3",                                       /* 30: pound sign */
    "\xC2\xB5",                                       /* 31: micro sign */
    ">",                                              /* 32: non-US key shifted */
    "M",                                              /* 33 */
    "%",                                              /* 34 */
    NULL,                                             /* 35 */
    ".","/",                                          /* 36-37 */
    "\xC2\xA7",                                       /* 38: section sign */
};

/* French AltGr layer */
static const char *FR_ALTGR[] = {
    NULL,NULL,                                        /* 04-05 */
    "\xC3\xA7",                                       /* 06 (c) -> c-cedilla */
    NULL,                                             /* 07 */
    "\xe2\x82\xac",                                   /* 08 (e) -> Euro sign */
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,                /* 09-0F */
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,           /* 10-17 */
    NULL,NULL,NULL,NULL,                              /* 18-1B */
    NULL,NULL,                                        /* 1C-1D */
    NULL,                                             /* 1E */
    "~",                                              /* 1F (2) -> ~ */
    "#",                                              /* 20 (3) -> # */
    "{",                                              /* 21 (4) -> { */
    "[",                                              /* 22 (5) -> [ */
    "|",                                              /* 23 (6) -> | */
    "`",                                              /* 24 (7) -> ` */
    "\\",                                             /* 25 (8) -> backslash */
    "\xC3\x87",                                       /* 26 (9) -> C-cedilla (upper) */
    "@",                                              /* 27 (0) -> @ */
    NULL,NULL,NULL,NULL,NULL,                          /* 28-2C */
    "]",                                              /* 2D -> ] */
    "}",                                              /* 2E -> } */
    NULL,                                             /* 2F */
    "\xC2\xA4",                                       /* 30 ($) -> currency sign */
    NULL,                                             /* 31 */
    NULL,                                             /* 32 */
    NULL,NULL,NULL,NULL,NULL,NULL,                     /* 33-38 */
};
#endif /* CONFIG_KB_LAYOUT_ENABLE_FR */


#ifdef CONFIG_KB_LAYOUT_ENABLE_UA
/* ------------------------------------------------------------------ */
/* Ukrainian layout                                                   */
/* ------------------------------------------------------------------ */
/* Ukrainian Cyrillic layout mapped on a US physical keyboard.
 * Each entry is a UTF-8 encoded Ukrainian letter.
 *
 * Physical key -> Ukrainian letter mapping (unshifted):
 * q->j  w->ts  e->u  r->k  t->e  y->n  u->g  i->sh  o->shch  p->z
 * a->f  s->i   d->v  f->a  g->p  h->r  j->o  k->l   l->d
 * z->ya x->ch  c->s  v->m  b->y  n->t  [->kh ]->yi   ;->zh    '->ye
 * `->apostrophe  \->ghe
 */

/* Lowercase Ukrainian letters as UTF-8 hex escapes.
 * Cyrillic block: U+0400..U+04FF -> UTF-8: 0xD0 0x80..0xBF, 0xD1 0x80..0xBF
 */

static const char *UA_NORMAL[] = {
    /* 04 a */ "\xD1\x84",   /* f (Cyrillic) */
    /* 05 b */ "\xD0\xB8",   /* i (short) */
    /* 06 c */ "\xD1\x81",   /* s */
    /* 07 d */ "\xD0\xB2",   /* v */
    /* 08 e */ "\xD1\x83",   /* u */
    /* 09 f */ "\xD0\xB0",   /* a */
    /* 0A g */ "\xD0\xBF",   /* p */
    /* 0B h */ "\xD1\x80",   /* r */
    /* 0C i */ "\xD1\x88",   /* sh */
    /* 0D j */ "\xD0\xBE",   /* o */
    /* 0E k */ "\xD0\xBB",   /* l */
    /* 0F l */ "\xD0\xB4",   /* d */
    /* 10 m */ "\xD1\x8C",   /* soft sign */
    /* 11 n */ "\xD1\x82",   /* t */
    /* 12 o */ "\xD1\x89",   /* shch */
    /* 13 p */ "\xD0\xB7",   /* z */
    /* 14 q */ "\xD0\xB9",   /* j (short i) */
    /* 15 r */ "\xD0\xBA",   /* k */
    /* 16 s */ "\xD1\x96",   /* i (Ukrainian) */
    /* 17 t */ "\xD0\xB5",   /* e */
    /* 18 u */ "\xD0\xB3",   /* g */
    /* 19 v */ "\xD0\xBC",   /* m */
    /* 1A w */ "\xD1\x86",   /* ts */
    /* 1B x */ "\xD1\x87",   /* ch */
    /* 1C y */ "\xD0\xBD",   /* n */
    /* 1D z */ "\xD1\x8F",   /* ya */
    /* 1E 1 */ "1",
    /* 1F 2 */ "2",
    /* 20 3 */ "3",
    /* 21 4 */ "4",
    /* 22 5 */ "5",
    /* 23 6 */ "6",
    /* 24 7 */ "7",
    /* 25 8 */ "8",
    /* 26 9 */ "9",
    /* 27 0 */ "0",
    /* 28-2C */ NULL, NULL, NULL, NULL, " ",
    /* 2D - */ "-",
    /* 2E = */ "=",
    /* 2F [ */ "\xD1\x85",   /* kh */
    /* 30 ] */ "\xD1\x97",   /* yi */
    /* 31 \ */ "\xD2\x91",   /* ghe with upturn (lowercase) */
    /* 32   */ NULL,
    /* 33 ; */ "\xD0\xB6",   /* zh */
    /* 34 ' */ "\xD1\x94",   /* ye (Ukrainian) */
    /* 35 ` */ "'",           /* apostrophe (used in Ukrainian orthography) */
    /* 36 , */ "\xD0\xB1",   /* b (Ukrainian) */
    /* 37 . */ "\xD1\x8E",   /* yu */
    /* 38 / */ ".",
};

/* Uppercase Ukrainian */
static const char *UA_SHIFT[] = {
    /* 04 A */ "\xD0\xA4",   /* F */
    /* 05 B */ "\xD0\x98",   /* I (short) */
    /* 06 C */ "\xD0\xA1",   /* S */
    /* 07 D */ "\xD0\x92",   /* V */
    /* 08 E */ "\xD0\xA3",   /* U */
    /* 09 F */ "\xD0\x90",   /* A */
    /* 0A G */ "\xD0\x9F",   /* P */
    /* 0B H */ "\xD0\xA0",   /* R */
    /* 0C I */ "\xD0\xA8",   /* Sh */
    /* 0D J */ "\xD0\x9E",   /* O */
    /* 0E K */ "\xD0\x9B",   /* L */
    /* 0F L */ "\xD0\x94",   /* D */
    /* 10 M */ "\xD0\xAC",   /* soft sign (upper) */
    /* 11 N */ "\xD0\xA2",   /* T */
    /* 12 O */ "\xD0\xA9",   /* Shch */
    /* 13 P */ "\xD0\x97",   /* Z */
    /* 14 Q */ "\xD0\x99",   /* J */
    /* 15 R */ "\xD0\x9A",   /* K */
    /* 16 S */ "\xD0\x86",   /* I (Ukrainian, upper) */
    /* 17 T */ "\xD0\x95",   /* E */
    /* 18 U */ "\xD0\x93",   /* G */
    /* 19 V */ "\xD0\x9C",   /* M */
    /* 1A W */ "\xD0\xA6",   /* Ts */
    /* 1B X */ "\xD0\xA7",   /* Ch */
    /* 1C Y */ "\xD0\x9D",   /* N */
    /* 1D Z */ "\xD0\xAF",   /* Ya */
    /* 1E-27 */ "!","\"",
                "\xE2\x84\x96",          /* Shift+3: numero sign (U+2116) */
                ";","%",":",             /* Shift+4: ;  Shift+5: %  Shift+6: : */
                "?","*","(",")",         /* Shift+7: ? */
    /* 28-2C */ NULL, NULL, NULL, NULL, " ",
    /* 2D */ "_",
    /* 2E */ "+",
    /* 2F */ "\xD0\xA5",     /* Kh */
    /* 30 */ "\xD0\x87",     /* Yi (upper) */
    /* 31 */ "\xD2\x90",     /* Ghe with upturn (upper) */
    /* 32 */ NULL,
    /* 33 */ "\xD0\x96",     /* Zh */
    /* 34 */ "\xD0\x84",     /* Ye (upper, Ukrainian) */
    /* 35 */ "\xE2\x82\xB4", /* hryvnia sign (U+20B4) */
    /* 36 */ "\xD0\x91",     /* B (Ukrainian, upper) */
    /* 37 */ "\xD0\xAE",     /* Yu (upper) */
    /* 38 */ ",",
};
#endif /* CONFIG_KB_LAYOUT_ENABLE_UA */

#ifdef CONFIG_KB_LAYOUT_ENABLE_HE
/* ------------------------------------------------------------------ */
/* Hebrew (Israeli standard, SI 1452-2)                               */
/* ------------------------------------------------------------------ */
/* Hebrew has no case, so all 22 base letters appear in the unshifted
 * layer only. Shifted letter keys produce Latin uppercase letters in
 * the SI 1452-2 standard, which is convenient when mixing English
 * acronyms into Hebrew prose.
 *
 * Hebrew text is stored in logical (typing) order in our buffer; the
 * visual right-to-left ordering is handled by LVGL's bidirectional
 * text engine (LV_USE_BIDI). The bracket / parenthesis pairs are
 * visually swapped on the keytop in the standard so that opening
 * and closing characters read "naturally" inside an RTL run:
 *   [ key -> ]   ] key -> [
 *   shift-9 -> )    shift-0 -> (
 *
 * Hebrew letter UTF-8 encoding: U+05D0..U+05EA -> 0xD7 0x90..0xAA
 */

static const char *HE_NORMAL[] = {
    /* 04 a */ "\xD7\xA9",   /* shin     U+05E9 */
    /* 05 b */ "\xD7\xA0",   /* nun      U+05E0 */
    /* 06 c */ "\xD7\x91",   /* bet      U+05D1 */
    /* 07 d */ "\xD7\x92",   /* gimel    U+05D2 */
    /* 08 e */ "\xD7\xA7",   /* qof      U+05E7 */
    /* 09 f */ "\xD7\x9B",   /* kaf      U+05DB */
    /* 0A g */ "\xD7\xA2",   /* ayin     U+05E2 */
    /* 0B h */ "\xD7\x99",   /* yod      U+05D9 */
    /* 0C i */ "\xD7\x9F",   /* final nun U+05DF */
    /* 0D j */ "\xD7\x97",   /* het      U+05D7 */
    /* 0E k */ "\xD7\x9C",   /* lamed    U+05DC */
    /* 0F l */ "\xD7\x9A",   /* final kaf U+05DA */
    /* 10 m */ "\xD7\xA6",   /* tsadi    U+05E6 */
    /* 11 n */ "\xD7\x9E",   /* mem      U+05DE */
    /* 12 o */ "\xD7\x9D",   /* final mem U+05DD */
    /* 13 p */ "\xD7\xA4",   /* pe       U+05E4 */
    /* 14 q */ "/",
    /* 15 r */ "\xD7\xA8",   /* resh     U+05E8 */
    /* 16 s */ "\xD7\x93",   /* dalet    U+05D3 */
    /* 17 t */ "\xD7\x90",   /* alef     U+05D0 */
    /* 18 u */ "\xD7\x95",   /* vav      U+05D5 */
    /* 19 v */ "\xD7\x94",   /* he       U+05D4 */
    /* 1A w */ "'",
    /* 1B x */ "\xD7\xA1",   /* samekh   U+05E1 */
    /* 1C y */ "\xD7\x98",   /* tet      U+05D8 */
    /* 1D z */ "\xD7\x96",   /* zayin    U+05D6 */
    /* 1E 1 */ "1",
    /* 1F 2 */ "2",
    /* 20 3 */ "3",
    /* 21 4 */ "4",
    /* 22 5 */ "5",
    /* 23 6 */ "6",
    /* 24 7 */ "7",
    /* 25 8 */ "8",
    /* 26 9 */ "9",
    /* 27 0 */ "0",
    /* 28-2C */ NULL, NULL, NULL, NULL, " ",
    /* 2D - */ "-",
    /* 2E = */ "=",
    /* 2F [ */ "]",          /* visual swap for RTL */
    /* 30 ] */ "[",          /* visual swap for RTL */
    /* 31 \ */ "\\",
    /* 32   */ NULL,
    /* 33 ; */ "\xD7\xA3",   /* final pe U+05E3 */
    /* 34 ' */ ",",
    /* 35 ` */ ";",
    /* 36 , */ "\xD7\xAA",   /* tav      U+05EA */
    /* 37 . */ "\xD7\xA5",   /* final tsadi U+05E5 */
    /* 38 / */ ".",
};

static const char *HE_SHIFT[] = {
    /* Shifted letter keys produce Latin uppercase per SI 1452-2 */
    /* 04 A */ "A",
    /* 05 B */ "B",
    /* 06 C */ "C",
    /* 07 D */ "D",
    /* 08 E */ "E",
    /* 09 F */ "F",
    /* 0A G */ "G",
    /* 0B H */ "H",
    /* 0C I */ "I",
    /* 0D J */ "J",
    /* 0E K */ "K",
    /* 0F L */ "L",
    /* 10 M */ "M",
    /* 11 N */ "N",
    /* 12 O */ "O",
    /* 13 P */ "P",
    /* 14 Q */ "Q",
    /* 15 R */ "R",
    /* 16 S */ "S",
    /* 17 T */ "T",
    /* 18 U */ "U",
    /* 19 V */ "V",
    /* 1A W */ "W",
    /* 1B X */ "X",
    /* 1C Y */ "Y",
    /* 1D Z */ "Z",
    /* 1E 1 */ "!",
    /* 1F 2 */ "@",
    /* 20 3 */ "#",
    /* 21 4 */ "$",
    /* 22 5 */ "%",
    /* 23 6 */ "^",
    /* 24 7 */ "&",
    /* 25 8 */ "*",
    /* 26 9 */ ")",          /* swapped vs US for RTL */
    /* 27 0 */ "(",          /* swapped vs US for RTL */
    /* 28-2C */ NULL, NULL, NULL, NULL, " ",
    /* 2D */ "_",
    /* 2E */ "+",
    /* 2F */ "}",
    /* 30 */ "{",
    /* 31 */ "|",
    /* 32 */ NULL,
    /* 33 */ ":",
    /* 34 */ "\x22",
    /* 35 */ "~",
    /* 36 */ ">",
    /* 37 */ "<",
    /* 38 */ "?",
};
#endif /* CONFIG_KB_LAYOUT_ENABLE_HE */

/* ------------------------------------------------------------------ */
/* Layout table structure                                             */
/* ------------------------------------------------------------------ */

typedef struct {
    const char **normal;
    const char **shift;
    const char **altgr;  /* NULL if layout has no AltGr layer */
    int table_size;      /* number of entries (keycodes 0x04..0x04+size-1) */
} layout_table_t;

#define ARRAY_SIZE(a) ((int)(sizeof(a) / sizeof((a)[0])))

static const layout_table_t s_layouts[KB_LAYOUT_COUNT] = {
#ifdef CONFIG_KB_LAYOUT_ENABLE_US
    [KB_LAYOUT_US] = { US_NORMAL, US_SHIFT, NULL,      ARRAY_SIZE(US_NORMAL) },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_UA
    [KB_LAYOUT_UA] = { UA_NORMAL, UA_SHIFT, NULL,      ARRAY_SIZE(UA_NORMAL) },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_DE
    [KB_LAYOUT_DE] = { DE_NORMAL, DE_SHIFT, DE_ALTGR,  ARRAY_SIZE(DE_NORMAL) },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_FR
    [KB_LAYOUT_FR] = { FR_NORMAL, FR_SHIFT, FR_ALTGR,  ARRAY_SIZE(FR_NORMAL) },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_HE
    [KB_LAYOUT_HE] = { HE_NORMAL, HE_SHIFT, NULL,      ARRAY_SIZE(HE_NORMAL) },
#endif
};

static const char *s_layout_names[KB_LAYOUT_COUNT] = {
#ifdef CONFIG_KB_LAYOUT_ENABLE_US
    [KB_LAYOUT_US] = "US",
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_UA
    [KB_LAYOUT_UA] = "UA",
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_DE
    [KB_LAYOUT_DE] = "DE",
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_FR
    [KB_LAYOUT_FR] = "FR",
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_HE
    [KB_LAYOUT_HE] = "HE",
#endif
};

/* ------------------------------------------------------------------ */
/* Public API                                                         */
/* ------------------------------------------------------------------ */

extern "C" void kb_layout_set(kb_layout_id_t layout)
{
    if (layout >= KB_LAYOUT_COUNT) layout = (kb_layout_id_t)0;
    s_layout = layout;
}

extern "C" kb_layout_id_t kb_layout_get(void)
{
    return s_layout;
}

extern "C" const char *kb_layout_name(kb_layout_id_t layout)
{
    if (layout >= KB_LAYOUT_COUNT) return "??";
    return s_layout_names[layout];
}

extern "C" kb_layout_id_t kb_layout_next(void)
{
    s_layout = (kb_layout_id_t)((s_layout + 1) % KB_LAYOUT_COUNT);
    return s_layout;
}

extern "C" bool kb_layout_is_rtl(void)
{
#ifdef CONFIG_KB_LAYOUT_ENABLE_HE
    return s_layout == KB_LAYOUT_HE;
#else
    return false;
#endif
}

extern "C" const char *kb_layout_translate(uint8_t keycode, uint8_t modifier)
{
    if (keycode < 0x04) return NULL;

    const layout_table_t *lt = &s_layouts[s_layout];
    int idx = keycode - 0x04;
    if (idx >= lt->table_size) return NULL;

    bool shift = (modifier & (MOD_LSHIFT | MOD_RSHIFT)) != 0;
    bool altgr = (modifier & MOD_RALT) != 0;

    const char *result = NULL;

    if (altgr && lt->altgr && idx < lt->table_size) {
        result = lt->altgr[idx];
    }
    if (!result) {
        result = shift ? lt->shift[idx] : lt->normal[idx];
    }

    return result;
}
