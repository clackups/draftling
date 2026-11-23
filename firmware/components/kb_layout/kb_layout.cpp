/*
 * Keyboard layout translation module.
 *
 * Translates HID keycodes + modifier flags into UTF-8 strings
 * for US-English, Ukrainian, German, French, Hebrew, Swiss German,
 * Swiss French, Italian, Portuguese (Portugal and Brazil), Spanish, United Kingdom and the
 * other European layouts, and the Korean, Tamil InScript, Devanagari,
 * Bengali and Telugu layouts. Vietnamese, Japanese, Chinese and the other
 * Tamil layouts (Tamil 99, Typewriter, Taravinyas) type on the US
 * layout and are composed by an input method (components/ime/), and
 * so is the Bengali National (Jatiya) layout's automatic vowel
 * forming.
 *
 * Non-ASCII characters are encoded as UTF-8 byte sequences using
 * hex escapes to keep the source file ASCII-only.
 */

#include <cstdio>
#include <cstring>
#include "sdkconfig.h"
#include "kb_layout.h"

/* Modifier flags (same as ble_keyboard.h) */
#define MOD_LSHIFT 0x02
#define MOD_RSHIFT 0x20
#define MOD_RALT   0x40  /* AltGr on international keyboards */

static kb_layout_id_t s_layout = (kb_layout_id_t)0;

/* Default "Active language layouts" set: US when compiled in, plus the
 * main layouts of a pack (VI, KO, JA, ZH; TA99 and HI-IN), else the first
 * compiled-in layout so the set is never empty. The "all" pack, which
 * has every one of them, starts with US only. */
static uint64_t default_active_mask(void)
{
    uint64_t mask = 0;
#if !defined(CONFIG_DRAFTLING_LANG_PACK_ALL)
#ifdef CONFIG_KB_LAYOUT_ENABLE_VI
    mask |= (1ull << KB_LAYOUT_VI);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_KO
    mask |= (1ull << KB_LAYOUT_KO);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_JA
    mask |= (1ull << KB_LAYOUT_JA);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_ZH
    mask |= (1ull << KB_LAYOUT_ZH);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_TA99
    mask |= (1ull << KB_LAYOUT_TA99);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_HI_IN
    mask |= (1ull << KB_LAYOUT_HI_IN);
#endif
#endif /* !CONFIG_DRAFTLING_LANG_PACK_ALL */
#ifdef CONFIG_KB_LAYOUT_ENABLE_US
    mask |= (1ull << KB_LAYOUT_US);
#endif
    if (mask == 0) mask = 1ull;
    return mask;
}

static uint64_t s_active_mask = default_active_mask();

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
#define KC_ISO   0x64  /* non-US \ and | : the ISO key left of Z (<>) */

/* The normal / shift tables below cover keycodes 0x04..0x38. */
#define TABLE_KEYS (KC_SLASH - KC_A + 1)

/* Everything else -- AltGr layers and keys past 0x38 such as the ISO
 * key -- is a sparse list of key_entry_t, ended by a zero key. */
/* L_CAPS: output with Caps Lock on and Shift off, for keys whose Shift
 * layer is not the capital of the letter (the Czech number row, the
 * Swiss accented letters). Other letter keys need no entry; see
 * is_upper_letter(). */
enum { L_NORMAL, L_SHIFT, L_ALTGR, L_ALTGR_SHIFT, L_CAPS };

typedef struct {
    uint8_t key;      /* HID keycode */
    uint8_t layer;    /* L_* */
    const char *str;  /* UTF-8 output */
} key_entry_t;

/* Dead keys: an entry starting with \x01 does not type anything by
 * itself; the accent applies to the next letter (see translate()).
 * The bytes after \x01 are the accent's spacing form, typed when the
 * accent does not combine with the next key. */
#define DK_GRAVE "\x01`"
#define DK_ACUTE "\x01\xC2\xB4"
#define DK_CIRC  "\x01^"
#define DK_TILDE "\x01~"
#define DK_DIAER "\x01\xC2\xA8"
#define DK_CEDIL  "\x01\xC2\xB8"  /* U+00B8 cedilla */
#define DK_CARON  "\x01\xCB\x87"  /* U+02C7 caron */
#define DK_BREVE  "\x01\xCB\x98"  /* U+02D8 breve */
#define DK_DOT    "\x01\xCB\x99"  /* U+02D9 dot above */
#define DK_RING   "\x01\xCB\x9A"  /* U+02DA ring above */
#define DK_OGONEK "\x01\xCB\x9B"  /* U+02DB ogonek */
#define DK_DACUTE "\x01\xCB\x9D"  /* U+02DD double acute */
#define DK_COMMA  "\x01,"        /* comma below (no spacing form: a comma) */

/* Latin-1 / punctuation characters used by the European layouts. */
#define CH_SECT   "\xC2\xA7"  /* section sign */
#define CH_DEG    "\xC2\xB0"  /* degree sign */
#define CH_POUND  "\xC2\xA3"  /* pound sign */
#define CH_BROKEN "\xC2\xA6"  /* broken bar */
#define CH_NOT    "\xC2\xAC"  /* not sign */
#define CH_CENT   "\xC2\xA2"  /* cent sign */
#define CH_EURO   "\xE2\x82\xAC"
#define CH_ORDM   "\xC2\xBA"  /* masculine ordinal indicator */
#define CH_ORDF   "\xC2\xAA"  /* feminine ordinal indicator */
#define CH_MIDDOT "\xC2\xB7"  /* middle dot */
#define CH_IEXCL  "\xC2\xA1"  /* inverted exclamation mark */
#define CH_IQUES  "\xC2\xBF"  /* inverted question mark */
#define CH_LAQUO  "\xC2\xAB"  /* left guillemet */
#define CH_RAQUO  "\xC2\xBB"  /* right guillemet */
#define CH_MICRO  "\xC2\xB5"  /* micro sign */
#define CH_a_GRV  "\xC3\xA0"
#define CH_a_ACU  "\xC3\xA1"
#define CH_a_UML  "\xC3\xA4"
#define CH_c_CED  "\xC3\xA7"
#define CH_e_GRV  "\xC3\xA8"
#define CH_e_ACU  "\xC3\xA9"
#define CH_i_GRV  "\xC3\xAC"
#define CH_i_ACU  "\xC3\xAD"
#define CH_n_TIL  "\xC3\xB1"
#define CH_o_GRV  "\xC3\xB2"
#define CH_o_ACU  "\xC3\xB3"
#define CH_o_UML  "\xC3\xB6"
#define CH_u_GRV  "\xC3\xB9"
#define CH_u_ACU  "\xC3\xBA"
#define CH_u_UML  "\xC3\xBC"
#define CH_A_ACU  "\xC3\x81"
#define CH_C_CED  "\xC3\x87"
#define CH_E_ACU  "\xC3\x89"
#define CH_I_ACU  "\xC3\x8D"
#define CH_N_TIL  "\xC3\x91"
#define CH_O_ACU  "\xC3\x93"
#define CH_U_ACU  "\xC3\x9A"

/* Lowercase / uppercase letter rows, QWERTY and QWERTZ (Y and Z
 * swapped), keycodes 0x04..0x1D. */
#define LETTERS_QWERTY \
    "a","b","c","d","e","f","g","h","i","j","k","l", \
    "m","n","o","p","q","r","s","t","u","v","w","x","y","z"
#define LETTERS_QWERTY_UP \
    "A","B","C","D","E","F","G","H","I","J","K","L", \
    "M","N","O","P","Q","R","S","T","U","V","W","X","Y","Z"
#define LETTERS_QWERTZ \
    "a","b","c","d","e","f","g","h","i","j","k","l", \
    "m","n","o","p","q","r","s","t","u","v","w","x","z","y"
#define LETTERS_QWERTZ_UP \
    "A","B","C","D","E","F","G","H","I","J","K","L", \
    "M","N","O","P","Q","R","S","T","U","V","W","X","Z","Y"
#define DIGITS "1","2","3","4","5","6","7","8","9","0"
/* 0x28..0x2C: Enter, Esc, Backspace, Tab produce nothing; Space. */
#define CTRL_KEYS NULL,NULL,NULL,NULL," "

/* ------------------------------------------------------------------ */
/* US-English layout                                                  */
/* Always compiled: Vietnamese, Japanese and Chinese type on it too.  */
/* ------------------------------------------------------------------ */

/* unshifted: keycodes 0x04..0x38 */
static const char *const US_NORMAL[] = {
    "a","b","c","d","e","f","g","h","i","j","k","l",  /* 04-0F */
    "m","n","o","p","q","r","s","t","u","v","w","x",  /* 10-1B */
    "y","z",                                          /* 1C-1D */
    "1","2","3","4","5","6","7","8","9","0",           /* 1E-27 */
    NULL,NULL,NULL,NULL," ",                           /* 28-2C */
    "-","=","[","]","\\",                              /* 2D-31 */
    NULL,";","'","`",",",".","/",                      /* 32-38 */
};

/* shifted */
static const char *const US_SHIFT[] = {
    "A","B","C","D","E","F","G","H","I","J","K","L",
    "M","N","O","P","Q","R","S","T","U","V","W","X",
    "Y","Z",
    "!","@","#","$","%","^","&","*","(",")",
    NULL,NULL,NULL,NULL," ",
    "_","+","{","}","|",
    NULL,":","\x22","~","<",">","?",
};

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

static const char *const DE_NORMAL[] = {
    "a","b","c","d","e","f","g","h","i","j","k","l",  /* 04-0F */
    "m","n","o","p","q","r","s","t","u","v","w","x",  /* 10-1B */
    "z","y",                                          /* 1C-1D: Y and Z swapped */
    "1","2","3","4","5","6","7","8","9","0",           /* 1E-27 */
    NULL,NULL,NULL,NULL," ",                           /* 28-2C */
    "\xC3\x9F",                                       /* 2D: ss (Eszett) */
    DK_ACUTE,                                         /* 2E: acute accent (dead) */
    "\xC3\xBC",                                       /* 2F: ue */
    "+",                                              /* 30 */
    "#",                                              /* 31 */
    "#",                                              /* 32: ISO key left of Enter */
    "\xC3\xB6",                                       /* 33: oe */
    "\xC3\xA4",                                       /* 34: ae */
    DK_CIRC,                                          /* 35: circumflex (dead) */
    ",",".","-",                                       /* 36-38 */
};

static const char *const DE_SHIFT[] = {
    "A","B","C","D","E","F","G","H","I","J","K","L",
    "M","N","O","P","Q","R","S","T","U","V","W","X",
    "Z","Y",
    "!","\x22",                                       /* 1E-1F */
    "\xC2\xA7",                                       /* 20: section sign */
    "$","%","&","/","(",")",                           /* 21-27 */
    "=",                                              /* (shifted 0) - stored at idx 9 */
    NULL,NULL,NULL,NULL," ",
    "?",DK_GRAVE,                                     /* 2D, 2E: grave (dead) */
    "\xC3\x9C",                                       /* 2F: Ue */
    "*","'","'",                                      /* 30-32 */
    "\xC3\x96",                                       /* 33: Oe */
    "\xC3\x84",                                       /* 34: Ae */
    "\xC2\xB0",                                       /* 35: degree sign */
    ";",":",                                          /* 36-37 */
    "_",                                              /* 38 */
};

/* AltGr layer and the ISO key */
static const key_entry_t DE_EXTRA[] = {
    { 0x08,   L_ALTGR,  CH_EURO },      /* e */
    { 0x14,   L_ALTGR,  "@" },          /* q */
    { 0x1F,   L_ALTGR,  "\xC2\xB2" },   /* 2: superscript 2 */
    { 0x20,   L_ALTGR,  "\xC2\xB3" },   /* 3: superscript 3 */
    { 0x24,   L_ALTGR,  "{" },          /* 7 */
    { 0x25,   L_ALTGR,  "[" },          /* 8 */
    { 0x26,   L_ALTGR,  "]" },          /* 9 */
    { 0x27,   L_ALTGR,  "}" },          /* 0 */
    { 0x2D,   L_ALTGR,  "\\" },         /* sharp s key */
    { 0x30,   L_ALTGR,  "~" },          /* + key */
    { KC_ISO, L_NORMAL, "<" },
    { KC_ISO, L_SHIFT,  ">" },
    { KC_ISO, L_ALTGR,  "|" },
    { 0, 0, NULL },
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

static const char *const FR_NORMAL[] = {
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
    DK_CIRC,                                          /* 2F: circumflex (dead) */
    "$",                                              /* 30 */
    "*",                                              /* 31 */
    "*",                                              /* 32: ISO key left of Enter */
    "m",                                              /* 33: semicolon -> m */
    "\xC3\xB9",                                       /* 34: quote -> u-grave */
    "\xC2\xB2",                                       /* 35: grave -> superscript 2 */
    ";",":",                                          /* 36-37 */
    "!",                                              /* 38 */
};

static const char *const FR_SHIFT[] = {
    "Q","B","C","D","E","F","G","H","I","J","K","L",
    "?","N","O","P","A","R","S","T","U","V","Z","X",
    "Y","W",
    "1","2","3","4","5","6","7","8","9","0",           /* 1E-27: digits */
    NULL,NULL,NULL,NULL," ",
    "\xC2\xB0",                                       /* 2D: ) shifted -> degree */
    "+",                                              /* 2E */
    DK_DIAER,                                         /* 2F: diaeresis (dead) */
    "\xC2\xA3",                                       /* 30: pound sign */
    "\xC2\xB5",                                       /* 31: micro sign */
    CH_MICRO,                                         /* 32: ISO key left of Enter */
    "M",                                              /* 33 */
    "%",                                              /* 34 */
    NULL,                                             /* 35 */
    ".","/",                                          /* 36-37 */
    "\xC2\xA7",                                       /* 38: section sign */
};

/* AltGr layer and the ISO key */
static const key_entry_t FR_EXTRA[] = {
    { 0x06,   L_ALTGR,  CH_c_CED },     /* c */
    { 0x08,   L_ALTGR,  CH_EURO },      /* e */
    { 0x1F,   L_ALTGR,  "~" },          /* 2 */
    { 0x20,   L_ALTGR,  "#" },          /* 3 */
    { 0x21,   L_ALTGR,  "{" },          /* 4 */
    { 0x22,   L_ALTGR,  "[" },          /* 5 */
    { 0x23,   L_ALTGR,  "|" },          /* 6 */
    { 0x24,   L_ALTGR,  "`" },          /* 7 */
    { 0x25,   L_ALTGR,  "\\" },         /* 8 */
    { 0x26,   L_ALTGR,  CH_C_CED },     /* 9 */
    { 0x27,   L_ALTGR,  "@" },          /* 0 */
    { 0x2D,   L_ALTGR,  "]" },          /* ) key */
    { 0x2E,   L_ALTGR,  "}" },          /* = key */
    { 0x30,   L_ALTGR,  "\xC2\xA4" },   /* $ key: currency sign */
    { KC_ISO, L_NORMAL, "<" },
    { KC_ISO, L_SHIFT,  ">" },
    { 0, 0, NULL },
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

static const char *const UA_NORMAL[] = {
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
static const char *const UA_SHIFT[] = {
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

static const char *const HE_NORMAL[] = {
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

static const char *const HE_SHIFT[] = {
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

#ifdef CONFIG_KB_LAYOUT_ENABLE_KO
/* ------------------------------------------------------------------ */
/* Korean 2-set (Dubeolsik) layout                                    */
/* ------------------------------------------------------------------ */

/* Letter keys produce Hangul Compatibility Jamo (U+3131..U+3163); the
 * Hangul input method composes them into syllables. Shift only
 * changes the five keys with a tense / extra form (Q W E R T -> the
 * double consonants, O P -> yae / ye); digits and symbols are US. */
static const char *const KO_NORMAL[] = {
    "\xE3\x85\x81", /* 04 a: mieum */
    "\xE3\x85\xA0", /* 05 b: yu */
    "\xE3\x85\x8A", /* 06 c: chieuch */
    "\xE3\x85\x87", /* 07 d: ieung */
    "\xE3\x84\xB7", /* 08 e: tikeut */
    "\xE3\x84\xB9", /* 09 f: rieul */
    "\xE3\x85\x8E", /* 0A g: hieuh */
    "\xE3\x85\x97", /* 0B h: o */
    "\xE3\x85\x91", /* 0C i: ya */
    "\xE3\x85\x93", /* 0D j: eo */
    "\xE3\x85\x8F", /* 0E k: a */
    "\xE3\x85\xA3", /* 0F l: i */
    "\xE3\x85\xA1", /* 10 m: eu */
    "\xE3\x85\x9C", /* 11 n: u */
    "\xE3\x85\x90", /* 12 o: ae */
    "\xE3\x85\x94", /* 13 p: e */
    "\xE3\x85\x82", /* 14 q: pieup */
    "\xE3\x84\xB1", /* 15 r: kiyeok */
    "\xE3\x84\xB4", /* 16 s: nieun */
    "\xE3\x85\x85", /* 17 t: sios */
    "\xE3\x85\x95", /* 18 u: yeo */
    "\xE3\x85\x8D", /* 19 v: phieuph */
    "\xE3\x85\x88", /* 1A w: cieuc */
    "\xE3\x85\x8C", /* 1B x: thieuth */
    "\xE3\x85\x9B", /* 1C y: yo */
    "\xE3\x85\x8B", /* 1D z: khieukh */
    "1","2","3","4","5","6","7","8","9","0",           /* 1E-27 */
    NULL,NULL,NULL,NULL," ",                           /* 28-2C */
    "-","=","[","]","\\",                              /* 2D-31 */
    NULL,";","'","`",",",".","/",                      /* 32-38 */
};

static const char *const KO_SHIFT[] = {
    "\xE3\x85\x81", /* 04 A: mieum */
    "\xE3\x85\xA0", /* 05 B: yu */
    "\xE3\x85\x8A", /* 06 C: chieuch */
    "\xE3\x85\x87", /* 07 D: ieung */
    "\xE3\x84\xB8", /* 08 E: ssangtikeut */
    "\xE3\x84\xB9", /* 09 F: rieul */
    "\xE3\x85\x8E", /* 0A G: hieuh */
    "\xE3\x85\x97", /* 0B H: o */
    "\xE3\x85\x91", /* 0C I: ya */
    "\xE3\x85\x93", /* 0D J: eo */
    "\xE3\x85\x8F", /* 0E K: a */
    "\xE3\x85\xA3", /* 0F L: i */
    "\xE3\x85\xA1", /* 10 M: eu */
    "\xE3\x85\x9C", /* 11 N: u */
    "\xE3\x85\x92", /* 12 O: yae */
    "\xE3\x85\x96", /* 13 P: ye */
    "\xE3\x85\x83", /* 14 Q: ssangpieup */
    "\xE3\x84\xB2", /* 15 R: ssangkiyeok */
    "\xE3\x84\xB4", /* 16 S: nieun */
    "\xE3\x85\x86", /* 17 T: ssangsios */
    "\xE3\x85\x95", /* 18 U: yeo */
    "\xE3\x85\x8D", /* 19 V: phieuph */
    "\xE3\x85\x89", /* 1A W: ssangcieuc */
    "\xE3\x85\x8C", /* 1B X: thieuth */
    "\xE3\x85\x9B", /* 1C Y: yo */
    "\xE3\x85\x8B", /* 1D Z: khieukh */
    "!","@","#","$","%","^","&","*","(",")",
    NULL,NULL,NULL,NULL," ",
    "_","+","{","}","|",
    NULL,":","\x22","~","<",">","?",
};
#endif /* CONFIG_KB_LAYOUT_ENABLE_KO */

#ifdef CONFIG_KB_LAYOUT_ENABLE_TA_IN
/* ------------------------------------------------------------------ */
/* Tamil InScript                                                     */
/* ------------------------------------------------------------------ */

/* The Government of India InScript layout for Tamil, as in Windows
 * "Tamil" and XKB "in(tam)": vowel signs on the left hand (Shift: the
 * independent vowels), consonants on the right, pulli on D. Text is
 * typed in logical order (consonant, then vowel sign); the Tamil font
 * reorders the signs drawn left of the consonant. Keys XKB leaves
 * empty (Y, I, O, [) type nothing. AltGr + digit types the Tamil
 * digits. */
static const char *const TA_IN_NORMAL[] = {
    /* 04 */ "\xE0\xAF\x8B",  /* U+0BCB tamil vowel sign oo */
    /* 05 */ "\xE0\xAE\xB5",  /* U+0BB5 tamil letter va */
    /* 06 */ "\xE0\xAE\xAE",  /* U+0BAE tamil letter ma */
    /* 07 */ "\xE0\xAF\x8D",  /* U+0BCD tamil sign virama */
    /* 08 */ "\xE0\xAE\xBE",  /* U+0BBE tamil vowel sign aa */
    /* 09 */ "\xE0\xAE\xBF",  /* U+0BBF tamil vowel sign i */
    /* 0A */ "\xE0\xAF\x81",  /* U+0BC1 tamil vowel sign u */
    /* 0B */ "\xE0\xAE\xAA",  /* U+0BAA tamil letter pa */
    /* 0C */ NULL,
    /* 0D */ "\xE0\xAE\xB0",  /* U+0BB0 tamil letter ra */
    /* 0E */ "\xE0\xAE\x95",  /* U+0B95 tamil letter ka */
    /* 0F */ "\xE0\xAE\xA4",  /* U+0BA4 tamil letter ta */
    /* 10 */ "\xE0\xAE\xB8",  /* U+0BB8 tamil letter sa */
    /* 11 */ "\xE0\xAE\xB2",  /* U+0BB2 tamil letter la */
    /* 12 */ NULL,
    /* 13 */ "\xE0\xAE\x9C",  /* U+0B9C tamil letter ja */
    /* 14 */ "\xE0\xAF\x8C",  /* U+0BCC tamil vowel sign au */
    /* 15 */ "\xE0\xAF\x80",  /* U+0BC0 tamil vowel sign ii */
    /* 16 */ "\xE0\xAF\x87",  /* U+0BC7 tamil vowel sign ee */
    /* 17 */ "\xE0\xAF\x82",  /* U+0BC2 tamil vowel sign uu */
    /* 18 */ "\xE0\xAE\xB9",  /* U+0BB9 tamil letter ha */
    /* 19 */ "\xE0\xAE\xA8",  /* U+0BA8 tamil letter na */
    /* 1A */ "\xE0\xAF\x88",  /* U+0BC8 tamil vowel sign ai */
    /* 1B */ "\xE0\xAE\x82",  /* U+0B82 tamil sign anusvara */
    /* 1C */ NULL,
    /* 1D */ "\xE0\xAF\x86",  /* U+0BC6 tamil vowel sign e */
    /* 1E */ "1",
    /* 1F */ "2",
    /* 20 */ "3",
    /* 21 */ "4",
    /* 22 */ "5",
    /* 23 */ "6",
    /* 24 */ "7",
    /* 25 */ "8",
    /* 26 */ "9",
    /* 27 */ "0",
    /* 28 */ NULL,
    /* 29 */ NULL,
    /* 2A */ NULL,
    /* 2B */ NULL,
    /* 2C */ " ",
    /* 2D */ "-",
    /* 2E */ "=",
    /* 2F */ NULL,
    /* 30 */ "\xE0\xAE\x9E",  /* U+0B9E tamil letter nya */
    /* 31 */ "\\",
    /* 32 */ NULL,
    /* 33 */ "\xE0\xAE\x9A",  /* U+0B9A tamil letter ca */
    /* 34 */ "\xE0\xAE\x9F",  /* U+0B9F tamil letter tta */
    /* 35 */ "\xE0\xAF\x8A",  /* U+0BCA tamil vowel sign o */
    /* 36 */ ",",
    /* 37 */ ".",
    /* 38 */ "\xE0\xAE\xAF",  /* U+0BAF tamil letter ya */
};

static const char *const TA_IN_SHIFT[] = {
    /* 04 */ "\xE0\xAE\x93",  /* U+0B93 tamil letter oo */
    /* 05 */ "\xE0\xAE\xB4",  /* U+0BB4 tamil letter llla */
    /* 06 */ "\xE0\xAE\xA3",  /* U+0BA3 tamil letter nna */
    /* 07 */ "\xE0\xAE\x85",  /* U+0B85 tamil letter a */
    /* 08 */ "\xE0\xAE\x86",  /* U+0B86 tamil letter aa */
    /* 09 */ "\xE0\xAE\x87",  /* U+0B87 tamil letter i */
    /* 0A */ "\xE0\xAE\x89",  /* U+0B89 tamil letter u */
    /* 0B */ NULL,
    /* 0C */ NULL,
    /* 0D */ "\xE0\xAE\xB1",  /* U+0BB1 tamil letter rra */
    /* 0E */ NULL,
    /* 0F */ NULL,
    /* 10 */ "\xE0\xAE\xB6",  /* U+0BB6 tamil letter sha */
    /* 11 */ "\xE0\xAE\xB3",  /* U+0BB3 tamil letter lla */
    /* 12 */ NULL,
    /* 13 */ NULL,
    /* 14 */ "\xE0\xAE\x94",  /* U+0B94 tamil letter au */
    /* 15 */ "\xE0\xAE\x88",  /* U+0B88 tamil letter ii */
    /* 16 */ "\xE0\xAE\x8F",  /* U+0B8F tamil letter ee */
    /* 17 */ "\xE0\xAE\x8A",  /* U+0B8A tamil letter uu */
    /* 18 */ "\xE0\xAE\x99",  /* U+0B99 tamil letter nga */
    /* 19 */ "\xE0\xAE\xA9",  /* U+0BA9 tamil letter nnna */
    /* 1A */ "\xE0\xAE\x90",  /* U+0B90 tamil letter ai */
    /* 1B */ NULL,
    /* 1C */ NULL,
    /* 1D */ "\xE0\xAE\x8E",  /* U+0B8E tamil letter e */
    /* 1E */ "!",
    /* 1F */ "@",
    /* 20 */ "#",
    /* 21 */ "$",
    /* 22 */ "%",
    /* 23 */ "^",
    /* 24 */ "&",
    /* 25 */ "*",
    /* 26 */ "(",
    /* 27 */ ")",
    /* 28 */ NULL,
    /* 29 */ NULL,
    /* 2A */ NULL,
    /* 2B */ NULL,
    /* 2C */ " ",
    /* 2D */ "\xE0\xAE\x83",  /* U+0B83 tamil sign visarga */
    /* 2E */ "+",
    /* 2F */ NULL,
    /* 30 */ NULL,
    /* 31 */ "|",
    /* 32 */ NULL,
    /* 33 */ NULL,
    /* 34 */ NULL,
    /* 35 */ "\xE0\xAE\x92",  /* U+0B92 tamil letter o */
    /* 36 */ "\xE0\xAE\xB7",  /* U+0BB7 tamil letter ssa */
    /* 37 */ ">",
    /* 38 */ "?",
};

static const key_entry_t TA_IN_EXTRA[] = {
    { 0x1E, L_ALTGR, "\xE0\xAF\xA7" },  /* U+0BE7 tamil digit one */
    { 0x1F, L_ALTGR, "\xE0\xAF\xA8" },  /* U+0BE8 tamil digit two */
    { 0x20, L_ALTGR, "\xE0\xAF\xA9" },  /* U+0BE9 tamil digit three */
    { 0x21, L_ALTGR, "\xE0\xAF\xAA" },  /* U+0BEA tamil digit four */
    { 0x22, L_ALTGR, "\xE0\xAF\xAB" },  /* U+0BEB tamil digit five */
    { 0x23, L_ALTGR, "\xE0\xAF\xAC" },  /* U+0BEC tamil digit six */
    { 0x24, L_ALTGR, "\xE0\xAF\xAD" },  /* U+0BED tamil digit seven */
    { 0x25, L_ALTGR, "\xE0\xAF\xAE" },  /* U+0BEE tamil digit eight */
    { 0x26, L_ALTGR, "\xE0\xAF\xAF" },  /* U+0BEF tamil digit nine */
    { 0x27, L_ALTGR, "\xE0\xAF\xA6" },  /* U+0BE6 tamil digit zero */
    { 0x2D, L_ALTGR, "\xE0\xAF\xB1" },  /* U+0BF1 tamil number one hundred */
    { 0x2E, L_ALTGR, "\xE0\xAF\xB2" },  /* U+0BF2 tamil number one thousand */
    { 0x1B, L_ALTGR, "\xE0\xAF\x90" },  /* U+0BD0 tamil om */
    { 0, 0, NULL },
};
#endif /* CONFIG_KB_LAYOUT_ENABLE_TA_IN */

#if defined(CONFIG_KB_LAYOUT_ENABLE_HI_IN) || defined(CONFIG_KB_LAYOUT_ENABLE_MR_IN) || \
    defined(CONFIG_KB_LAYOUT_ENABLE_NE_TR) || defined(CONFIG_KB_LAYOUT_ENABLE_NE_RO)
/* ------------------------------------------------------------------ */
/* Devanagari: Hindi and Marathi InScript, Nepali Traditional and     */
/* Romanized                                                          */
/* ------------------------------------------------------------------ */

/* Generated from the m17n-db input methods "hi-inscript2" (Enhanced
 * InScript, BIS 2016), "mr-inscript2", "ne-trad" and "ne-rom" (keys
 * typed on a US keyboard -> output), with the InScript keys m17n
 * leaves empty taken from XKB "in(deva)" (marked). Text is typed in
 * Unicode order (consonant, then vowel sign); HarfBuzz shapes it for
 * display. Keys without an m17n entry keep their US character, except
 * letters, which type nothing. */

#ifdef CONFIG_KB_LAYOUT_ENABLE_HI_IN
static const char *const HI_IN_NORMAL[] = {
    /* 04 'a' */ "\xE0\xA5\x8B",  /* U+094B devanagari vowel sign o */
    /* 05 'b' */ "\xE0\xA4\xB5",  /* U+0935 devanagari letter va */
    /* 06 'c' */ "\xE0\xA4\xAE",  /* U+092E devanagari letter ma */
    /* 07 'd' */ "\xE0\xA5\x8D",  /* U+094D devanagari sign virama */
    /* 08 'e' */ "\xE0\xA4\xBE",  /* U+093E devanagari vowel sign aa */
    /* 09 'f' */ "\xE0\xA4\xBF",  /* U+093F devanagari vowel sign i */
    /* 0A 'g' */ "\xE0\xA5\x81",  /* U+0941 devanagari vowel sign u */
    /* 0B 'h' */ "\xE0\xA4\xAA",  /* U+092A devanagari letter pa */
    /* 0C 'i' */ "\xE0\xA4\x97",  /* U+0917 devanagari letter ga */
    /* 0D 'j' */ "\xE0\xA4\xB0",  /* U+0930 devanagari letter ra */
    /* 0E 'k' */ "\xE0\xA4\x95",  /* U+0915 devanagari letter ka */
    /* 0F 'l' */ "\xE0\xA4\xA4",  /* U+0924 devanagari letter ta */
    /* 10 'm' */ "\xE0\xA4\xB8",  /* U+0938 devanagari letter sa */
    /* 11 'n' */ "\xE0\xA4\xB2",  /* U+0932 devanagari letter la */
    /* 12 'o' */ "\xE0\xA4\xA6",  /* U+0926 devanagari letter da */
    /* 13 'p' */ "\xE0\xA4\x9C",  /* U+091C devanagari letter ja */
    /* 14 'q' */ "\xE0\xA5\x8C",  /* U+094C devanagari vowel sign au */
    /* 15 'r' */ "\xE0\xA5\x80",  /* U+0940 devanagari vowel sign ii */
    /* 16 's' */ "\xE0\xA5\x87",  /* U+0947 devanagari vowel sign e */
    /* 17 't' */ "\xE0\xA5\x82",  /* U+0942 devanagari vowel sign uu */
    /* 18 'u' */ "\xE0\xA4\xB9",  /* U+0939 devanagari letter ha */
    /* 19 'v' */ "\xE0\xA4\xA8",  /* U+0928 devanagari letter na */
    /* 1A 'w' */ "\xE0\xA5\x88",  /* U+0948 devanagari vowel sign ai */
    /* 1B 'x' */ "\xE0\xA4\x82",  /* U+0902 devanagari sign anusvara */
    /* 1C 'y' */ "\xE0\xA4\xAC",  /* U+092C devanagari letter ba */
    /* 1D 'z' */ "\xE0\xA5\x86",  /* U+0946 devanagari vowel sign short e (XKB in(deva)) */
    /* 1E '1' */ "\xE0\xA5\xA7",  /* U+0967 devanagari digit one */
    /* 1F '2' */ "\xE0\xA5\xA8",  /* U+0968 devanagari digit two */
    /* 20 '3' */ "\xE0\xA5\xA9",  /* U+0969 devanagari digit three */
    /* 21 '4' */ "\xE0\xA5\xAA",  /* U+096A devanagari digit four */
    /* 22 '5' */ "\xE0\xA5\xAB",  /* U+096B devanagari digit five */
    /* 23 '6' */ "\xE0\xA5\xAC",  /* U+096C devanagari digit six */
    /* 24 '7' */ "\xE0\xA5\xAD",  /* U+096D devanagari digit seven */
    /* 25 '8' */ "\xE0\xA5\xAE",  /* U+096E devanagari digit eight */
    /* 26 '9' */ "\xE0\xA5\xAF",  /* U+096F devanagari digit nine */
    /* 27 '0' */ "\xE0\xA5\xA6",  /* U+0966 devanagari digit zero */
    /* 28  */ NULL,
    /* 29  */ NULL,
    /* 2A  */ NULL,
    /* 2B  */ NULL,
    /* 2C ' ' */ " ",
    /* 2D '-' */ "-",
    /* 2E '=' */ "\xE0\xA5\x83",  /* U+0943 devanagari vowel sign vocalic r */
    /* 2F '[' */ "\xE0\xA4\xA1",  /* U+0921 devanagari letter dda */
    /* 30 ']' */ "\xE0\xA4\xBC",  /* U+093C devanagari sign nukta */
    /* 31 '\\' */ "\xE0\xA5\x89",  /* U+0949 devanagari vowel sign candra o */
    /* 32  */ NULL,
    /* 33 ';' */ "\xE0\xA4\x9A",  /* U+091A devanagari letter ca */
    /* 34 "'" */ "\xE0\xA4\x9F",  /* U+091F devanagari letter tta */
    /* 35 '`' */ "\xE0\xA5\x8A",  /* U+094A devanagari vowel sign short o (XKB in(deva)) */
    /* 36 ',' */ ",",
    /* 37 '.' */ ".",
    /* 38 '/' */ "\xE0\xA4\xAF",  /* U+092F devanagari letter ya */
};

static const char *const HI_IN_SHIFT[] = {
    /* 04 'A' */ "\xE0\xA4\x93",  /* U+0913 devanagari letter o */
    /* 05 'B' */ "\xE0\xA4\xB4",  /* U+0934 devanagari letter llla (XKB in(deva)) */
    /* 06 'C' */ "\xE0\xA4\xA3",  /* U+0923 devanagari letter nna */
    /* 07 'D' */ "\xE0\xA4\x85",  /* U+0905 devanagari letter a */
    /* 08 'E' */ "\xE0\xA4\x86",  /* U+0906 devanagari letter aa */
    /* 09 'F' */ "\xE0\xA4\x87",  /* U+0907 devanagari letter i */
    /* 0A 'G' */ "\xE0\xA4\x89",  /* U+0909 devanagari letter u */
    /* 0B 'H' */ "\xE0\xA4\xAB",  /* U+092B devanagari letter pha */
    /* 0C 'I' */ "\xE0\xA4\x98",  /* U+0918 devanagari letter gha */
    /* 0D 'J' */ "\xE0\xA4\xB1",  /* U+0931 devanagari letter rra (XKB in(deva)) */
    /* 0E 'K' */ "\xE0\xA4\x96",  /* U+0916 devanagari letter kha */
    /* 0F 'L' */ "\xE0\xA4\xA5",  /* U+0925 devanagari letter tha */
    /* 10 'M' */ "\xE0\xA4\xB6",  /* U+0936 devanagari letter sha */
    /* 11 'N' */ "\xE0\xA4\xB3",  /* U+0933 devanagari letter lla (XKB in(deva)) */
    /* 12 'O' */ "\xE0\xA4\xA7",  /* U+0927 devanagari letter dha */
    /* 13 'P' */ "\xE0\xA4\x9D",  /* U+091D devanagari letter jha */
    /* 14 'Q' */ "\xE0\xA4\x94",  /* U+0914 devanagari letter au */
    /* 15 'R' */ "\xE0\xA4\x88",  /* U+0908 devanagari letter ii */
    /* 16 'S' */ "\xE0\xA4\x8F",  /* U+090F devanagari letter e */
    /* 17 'T' */ "\xE0\xA4\x8A",  /* U+090A devanagari letter uu */
    /* 18 'U' */ "\xE0\xA4\x99",  /* U+0919 devanagari letter nga */
    /* 19 'V' */ "\xE0\xA4\xA9",  /* U+0929 devanagari letter nnna (XKB in(deva)) */
    /* 1A 'W' */ "\xE0\xA4\x90",  /* U+0910 devanagari letter ai */
    /* 1B 'X' */ "\xE0\xA4\x81",  /* U+0901 devanagari sign candrabindu */
    /* 1C 'Y' */ "\xE0\xA4\xAD",  /* U+092D devanagari letter bha */
    /* 1D 'Z' */ "\xE0\xA4\x8E",  /* U+090E devanagari letter short e (XKB in(deva)) */
    /* 1E '!' */ "\xE0\xA4\x8D",  /* U+090D devanagari letter candra e */
    /* 1F '@' */ "\xE0\xA5\x85",  /* U+0945 devanagari vowel sign candra e */
    /* 20 '#' */ "\xE0\xA5\x8D\xE0\xA4\xB0",  /* U+094D devanagari sign virama U+0930 devanagari letter ra */
    /* 21 '$' */ "\xE0\xA4\xB0\xE0\xA5\x8D",  /* U+0930 devanagari letter ra U+094D devanagari sign virama */
    /* 22 '%' */ "\xE0\xA4\x9C\xE0\xA5\x8D\xE0\xA4\x9E",  /* U+091C devanagari letter ja U+094D devanagari sign virama U+091E devanagari letter nya */
    /* 23 '^' */ "\xE0\xA4\xA4\xE0\xA5\x8D\xE0\xA4\xB0",  /* U+0924 devanagari letter ta U+094D devanagari sign virama U+0930 devanagari letter ra */
    /* 24 '&' */ "\xE0\xA4\x95\xE0\xA5\x8D\xE0\xA4\xB7",  /* U+0915 devanagari letter ka U+094D devanagari sign virama U+0937 devanagari letter ssa */
    /* 25 '*' */ "\xE0\xA4\xB6\xE0\xA5\x8D\xE0\xA4\xB0",  /* U+0936 devanagari letter sha U+094D devanagari sign virama U+0930 devanagari letter ra */
    /* 26 '(' */ "(",
    /* 27 ')' */ ")",
    /* 28  */ NULL,
    /* 29  */ NULL,
    /* 2A  */ NULL,
    /* 2B  */ NULL,
    /* 2C ' ' */ " ",
    /* 2D '_' */ "\xE0\xA4\x83",  /* U+0903 devanagari sign visarga */
    /* 2E '+' */ "\xE0\xA4\x8B",  /* U+090B devanagari letter vocalic r */
    /* 2F '{' */ "\xE0\xA4\xA2",  /* U+0922 devanagari letter ddha */
    /* 30 '}' */ "\xE0\xA4\x9E",  /* U+091E devanagari letter nya */
    /* 31 '|' */ "\xE0\xA4\x91",  /* U+0911 devanagari letter candra o */
    /* 32  */ NULL,
    /* 33 ':' */ "\xE0\xA4\x9B",  /* U+091B devanagari letter cha */
    /* 34 '"' */ "\xE0\xA4\xA0",  /* U+0920 devanagari letter ttha */
    /* 35 '~' */ "\xE0\xA4\x92",  /* U+0912 devanagari letter short o (XKB in(deva)) */
    /* 36 '<' */ "\xE0\xA4\xB7",  /* U+0937 devanagari letter ssa */
    /* 37 '>' */ "\xE0\xA5\xA4",  /* U+0964 devanagari danda */
    /* 38 '?' */ "?",
};

static const key_entry_t HI_IN_EXTRA[] = {
    { 0x2E, L_ALTGR_SHIFT, "\xE0\xA5\xA0" },  /* U+0960 devanagari letter vocalic rr */
    { 0x36, L_ALTGR, "\xE0\xA5\xB0" },  /* U+0970 devanagari abbreviation sign */
    { 0x37, L_ALTGR, "\xE0\xA5\xA5" },  /* U+0965 devanagari double danda */
    { 0x1E, L_ALTGR, "\xE2\x80\x8D" },  /* U+200D zero width joiner */
    { 0x1F, L_ALTGR, "\xE2\x80\x8C" },  /* U+200C zero width non-joiner */
    { 0x21, L_ALTGR, "\xE2\x82\xB9" },  /* U+20B9 indian rupee sign */
    { 0x2E, L_ALTGR, "\xE0\xA5\x84" },  /* U+0944 devanagari vowel sign vocalic rr */
    { 0x37, L_ALTGR_SHIFT, "\xE0\xA4\xBD" },  /* U+093D devanagari sign avagraha */
    { 0x09, L_ALTGR_SHIFT, "\xE0\xA4\x8C" },  /* U+090C devanagari letter vocalic l */
    { 0x0B, L_ALTGR_SHIFT, "\xE0\xA5\x9E" },  /* U+095E devanagari letter fa */
    { 0x0E, L_ALTGR_SHIFT, "\xE0\xA5\x99" },  /* U+0959 devanagari letter khha */
    { 0x11, L_ALTGR_SHIFT, "\xE0\xA4\xB3" },  /* U+0933 devanagari letter lla */
    { 0x15, L_ALTGR_SHIFT, "\xE0\xA5\xA1" },  /* U+0961 devanagari letter vocalic ll */
    { 0x1B, L_ALTGR_SHIFT, "\xE0\xA5\x90" },  /* U+0950 devanagari om */
    { 0x2F, L_ALTGR, "\xE0\xA5\x9C" },  /* U+095C devanagari letter dddha */
    { 0x09, L_ALTGR, "\xE0\xA5\xA2" },  /* U+0962 devanagari vowel sign vocalic l */
    { 0x0C, L_ALTGR, "\xE0\xA5\x9A" },  /* U+095A devanagari letter ghha */
    { 0x0E, L_ALTGR, "\xE0\xA5\x98" },  /* U+0958 devanagari letter qa */
    { 0x13, L_ALTGR, "\xE0\xA5\x9B" },  /* U+095B devanagari letter za */
    { 0x15, L_ALTGR, "\xE0\xA5\xA3" },  /* U+0963 devanagari vowel sign vocalic ll */
    { 0x2F, L_ALTGR_SHIFT, "\xE0\xA5\x9D" },  /* U+095D devanagari letter rha */
    { 0, 0, NULL },
};
#endif

#ifdef CONFIG_KB_LAYOUT_ENABLE_MR_IN
static const char *const MR_IN_NORMAL[] = {
    /* 04 'a' */ "\xE0\xA5\x8B",  /* U+094B devanagari vowel sign o */
    /* 05 'b' */ "\xE0\xA4\xB5",  /* U+0935 devanagari letter va */
    /* 06 'c' */ "\xE0\xA4\xAE",  /* U+092E devanagari letter ma */
    /* 07 'd' */ "\xE0\xA5\x8D",  /* U+094D devanagari sign virama */
    /* 08 'e' */ "\xE0\xA4\xBE",  /* U+093E devanagari vowel sign aa */
    /* 09 'f' */ "\xE0\xA4\xBF",  /* U+093F devanagari vowel sign i */
    /* 0A 'g' */ "\xE0\xA5\x81",  /* U+0941 devanagari vowel sign u */
    /* 0B 'h' */ "\xE0\xA4\xAA",  /* U+092A devanagari letter pa */
    /* 0C 'i' */ "\xE0\xA4\x97",  /* U+0917 devanagari letter ga */
    /* 0D 'j' */ "\xE0\xA4\xB0",  /* U+0930 devanagari letter ra */
    /* 0E 'k' */ "\xE0\xA4\x95",  /* U+0915 devanagari letter ka */
    /* 0F 'l' */ "\xE0\xA4\xA4",  /* U+0924 devanagari letter ta */
    /* 10 'm' */ "\xE0\xA4\xB8",  /* U+0938 devanagari letter sa */
    /* 11 'n' */ "\xE0\xA4\xB2",  /* U+0932 devanagari letter la */
    /* 12 'o' */ "\xE0\xA4\xA6",  /* U+0926 devanagari letter da */
    /* 13 'p' */ "\xE0\xA4\x9C",  /* U+091C devanagari letter ja */
    /* 14 'q' */ "\xE0\xA5\x8C",  /* U+094C devanagari vowel sign au */
    /* 15 'r' */ "\xE0\xA5\x80",  /* U+0940 devanagari vowel sign ii */
    /* 16 's' */ "\xE0\xA5\x87",  /* U+0947 devanagari vowel sign e */
    /* 17 't' */ "\xE0\xA5\x82",  /* U+0942 devanagari vowel sign uu */
    /* 18 'u' */ "\xE0\xA4\xB9",  /* U+0939 devanagari letter ha */
    /* 19 'v' */ "\xE0\xA4\xA8",  /* U+0928 devanagari letter na */
    /* 1A 'w' */ "\xE0\xA5\x88",  /* U+0948 devanagari vowel sign ai */
    /* 1B 'x' */ "\xE0\xA4\x82",  /* U+0902 devanagari sign anusvara */
    /* 1C 'y' */ "\xE0\xA4\xAC",  /* U+092C devanagari letter ba */
    /* 1D 'z' */ "\xE0\xA5\x86",  /* U+0946 devanagari vowel sign short e (XKB in(deva)) */
    /* 1E '1' */ "\xE0\xA5\xA7",  /* U+0967 devanagari digit one */
    /* 1F '2' */ "\xE0\xA5\xA8",  /* U+0968 devanagari digit two */
    /* 20 '3' */ "\xE0\xA5\xA9",  /* U+0969 devanagari digit three */
    /* 21 '4' */ "\xE0\xA5\xAA",  /* U+096A devanagari digit four */
    /* 22 '5' */ "\xE0\xA5\xAB",  /* U+096B devanagari digit five */
    /* 23 '6' */ "\xE0\xA5\xAC",  /* U+096C devanagari digit six */
    /* 24 '7' */ "\xE0\xA5\xAD",  /* U+096D devanagari digit seven */
    /* 25 '8' */ "\xE0\xA5\xAE",  /* U+096E devanagari digit eight */
    /* 26 '9' */ "\xE0\xA5\xAF",  /* U+096F devanagari digit nine */
    /* 27 '0' */ "\xE0\xA5\xA6",  /* U+0966 devanagari digit zero */
    /* 28  */ NULL,
    /* 29  */ NULL,
    /* 2A  */ NULL,
    /* 2B  */ NULL,
    /* 2C ' ' */ " ",
    /* 2D '-' */ "-",
    /* 2E '=' */ "\xE0\xA5\x83",  /* U+0943 devanagari vowel sign vocalic r */
    /* 2F '[' */ "\xE0\xA4\xA1",  /* U+0921 devanagari letter dda */
    /* 30 ']' */ "\xE0\xA4\xBC",  /* U+093C devanagari sign nukta */
    /* 31 '\\' */ "\xE0\xA5\x89",  /* U+0949 devanagari vowel sign candra o */
    /* 32  */ NULL,
    /* 33 ';' */ "\xE0\xA4\x9A",  /* U+091A devanagari letter ca */
    /* 34 "'" */ "\xE0\xA4\x9F",  /* U+091F devanagari letter tta */
    /* 35 '`' */ "\xE0\xA5\x8A",  /* U+094A devanagari vowel sign short o */
    /* 36 ',' */ ",",
    /* 37 '.' */ ".",
    /* 38 '/' */ "\xE0\xA4\xAF",  /* U+092F devanagari letter ya */
};

static const char *const MR_IN_SHIFT[] = {
    /* 04 'A' */ "\xE0\xA4\x93",  /* U+0913 devanagari letter o */
    /* 05 'B' */ "\xE0\xA4\xB4",  /* U+0934 devanagari letter llla (XKB in(deva)) */
    /* 06 'C' */ "\xE0\xA4\xA3",  /* U+0923 devanagari letter nna */
    /* 07 'D' */ "\xE0\xA4\x85",  /* U+0905 devanagari letter a */
    /* 08 'E' */ "\xE0\xA4\x86",  /* U+0906 devanagari letter aa */
    /* 09 'F' */ "\xE0\xA4\x87",  /* U+0907 devanagari letter i */
    /* 0A 'G' */ "\xE0\xA4\x89",  /* U+0909 devanagari letter u */
    /* 0B 'H' */ "\xE0\xA4\xAB",  /* U+092B devanagari letter pha */
    /* 0C 'I' */ "\xE0\xA4\x98",  /* U+0918 devanagari letter gha */
    /* 0D 'J' */ "\xE0\xA4\xB1",  /* U+0931 devanagari letter rra */
    /* 0E 'K' */ "\xE0\xA4\x96",  /* U+0916 devanagari letter kha */
    /* 0F 'L' */ "\xE0\xA4\xA5",  /* U+0925 devanagari letter tha */
    /* 10 'M' */ "\xE0\xA4\xB6",  /* U+0936 devanagari letter sha */
    /* 11 'N' */ "\xE0\xA4\xB3",  /* U+0933 devanagari letter lla */
    /* 12 'O' */ "\xE0\xA4\xA7",  /* U+0927 devanagari letter dha */
    /* 13 'P' */ "\xE0\xA4\x9D",  /* U+091D devanagari letter jha */
    /* 14 'Q' */ "\xE0\xA4\x94",  /* U+0914 devanagari letter au */
    /* 15 'R' */ "\xE0\xA4\x88",  /* U+0908 devanagari letter ii */
    /* 16 'S' */ "\xE0\xA4\x8F",  /* U+090F devanagari letter e */
    /* 17 'T' */ "\xE0\xA4\x8A",  /* U+090A devanagari letter uu */
    /* 18 'U' */ "\xE0\xA4\x99",  /* U+0919 devanagari letter nga */
    /* 19 'V' */ "\xE0\xA4\xA9",  /* U+0929 devanagari letter nnna (XKB in(deva)) */
    /* 1A 'W' */ "\xE0\xA4\x90",  /* U+0910 devanagari letter ai */
    /* 1B 'X' */ "\xE0\xA4\x81",  /* U+0901 devanagari sign candrabindu */
    /* 1C 'Y' */ "\xE0\xA4\xAD",  /* U+092D devanagari letter bha */
    /* 1D 'Z' */ "\xE0\xA4\x8E",  /* U+090E devanagari letter short e (XKB in(deva)) */
    /* 1E '!' */ "\xE0\xA5\xB2",  /* U+0972 devanagari letter candra a */
    /* 1F '@' */ "\xE0\xA5\x85",  /* U+0945 devanagari vowel sign candra e */
    /* 20 '#' */ "\xE0\xA5\x8D\xE0\xA4\xB0",  /* U+094D devanagari sign virama U+0930 devanagari letter ra */
    /* 21 '$' */ "\xE0\xA4\xB0\xE0\xA5\x8D",  /* U+0930 devanagari letter ra U+094D devanagari sign virama */
    /* 22 '%' */ "\xE0\xA4\x9C\xE0\xA5\x8D\xE0\xA4\x9E",  /* U+091C devanagari letter ja U+094D devanagari sign virama U+091E devanagari letter nya */
    /* 23 '^' */ "\xE0\xA4\xA4\xE0\xA5\x8D\xE0\xA4\xB0",  /* U+0924 devanagari letter ta U+094D devanagari sign virama U+0930 devanagari letter ra */
    /* 24 '&' */ "\xE0\xA4\x95\xE0\xA5\x8D\xE0\xA4\xB7",  /* U+0915 devanagari letter ka U+094D devanagari sign virama U+0937 devanagari letter ssa */
    /* 25 '*' */ "\xE0\xA4\xB6\xE0\xA5\x8D\xE0\xA4\xB0",  /* U+0936 devanagari letter sha U+094D devanagari sign virama U+0930 devanagari letter ra */
    /* 26 '(' */ "(",
    /* 27 ')' */ ")",
    /* 28  */ NULL,
    /* 29  */ NULL,
    /* 2A  */ NULL,
    /* 2B  */ NULL,
    /* 2C ' ' */ " ",
    /* 2D '_' */ "\xE0\xA4\x83",  /* U+0903 devanagari sign visarga */
    /* 2E '+' */ "\xE0\xA4\x8B",  /* U+090B devanagari letter vocalic r */
    /* 2F '{' */ "\xE0\xA4\xA2",  /* U+0922 devanagari letter ddha */
    /* 30 '}' */ "\xE0\xA4\x9E",  /* U+091E devanagari letter nya */
    /* 31 '|' */ "\xE0\xA4\x91",  /* U+0911 devanagari letter candra o */
    /* 32  */ NULL,
    /* 33 ':' */ "\xE0\xA4\x9B",  /* U+091B devanagari letter cha */
    /* 34 '"' */ "\xE0\xA4\xA0",  /* U+0920 devanagari letter ttha */
    /* 35 '~' */ "\xE0\xA4\x92",  /* U+0912 devanagari letter short o */
    /* 36 '<' */ "\xE0\xA4\xB7",  /* U+0937 devanagari letter ssa */
    /* 37 '>' */ "\xE0\xA5\xA4",  /* U+0964 devanagari danda */
    /* 38 '?' */ "?",
};

static const key_entry_t MR_IN_EXTRA[] = {
    { 0x2E, L_ALTGR_SHIFT, "\xE0\xA5\xA0" },  /* U+0960 devanagari letter vocalic rr */
    { 0x36, L_ALTGR, "\xE0\xA5\xB0" },  /* U+0970 devanagari abbreviation sign */
    { 0x37, L_ALTGR, "\xE0\xA5\xA5" },  /* U+0965 devanagari double danda */
    { 0x1E, L_ALTGR, "\xE2\x80\x8D" },  /* U+200D zero width joiner */
    { 0x1F, L_ALTGR, "\xE2\x80\x8C" },  /* U+200C zero width non-joiner */
    { 0x21, L_ALTGR, "\xE2\x82\xB9" },  /* U+20B9 indian rupee sign */
    { 0x2E, L_ALTGR, "\xE0\xA5\x84" },  /* U+0944 devanagari vowel sign vocalic rr */
    { 0x37, L_ALTGR_SHIFT, "\xE0\xA4\xBD" },  /* U+093D devanagari sign avagraha */
    { 0x09, L_ALTGR_SHIFT, "\xE0\xA4\x8C" },  /* U+090C devanagari letter vocalic l */
    { 0x15, L_ALTGR_SHIFT, "\xE0\xA5\xA1" },  /* U+0961 devanagari letter vocalic ll */
    { 0x1B, L_ALTGR_SHIFT, "\xE0\xA5\x90" },  /* U+0950 devanagari om */
    { 0x09, L_ALTGR, "\xE0\xA5\xA2" },  /* U+0962 devanagari vowel sign vocalic l */
    { 0x15, L_ALTGR, "\xE0\xA5\xA3" },  /* U+0963 devanagari vowel sign vocalic ll */
    { 0, 0, NULL },
};
#endif

#ifdef CONFIG_KB_LAYOUT_ENABLE_NE_TR
static const char *const NE_TR_NORMAL[] = {
    /* 04 'a' */ "\xE0\xA4\xAC",  /* U+092C devanagari letter ba */
    /* 05 'b' */ "\xE0\xA4\xA6",  /* U+0926 devanagari letter da */
    /* 06 'c' */ "\xE0\xA4\x85",  /* U+0905 devanagari letter a */
    /* 07 'd' */ "\xE0\xA4\xAE",  /* U+092E devanagari letter ma */
    /* 08 'e' */ "\xE0\xA4\xAD",  /* U+092D devanagari letter bha */
    /* 09 'f' */ "\xE0\xA4\xBE",  /* U+093E devanagari vowel sign aa */
    /* 0A 'g' */ "\xE0\xA4\xA8",  /* U+0928 devanagari letter na */
    /* 0B 'h' */ "\xE0\xA4\x9C",  /* U+091C devanagari letter ja */
    /* 0C 'i' */ "\xE0\xA4\xB7",  /* U+0937 devanagari letter ssa */
    /* 0D 'j' */ "\xE0\xA4\xB5",  /* U+0935 devanagari letter va */
    /* 0E 'k' */ "\xE0\xA4\xAA",  /* U+092A devanagari letter pa */
    /* 0F 'l' */ "\xE0\xA4\xBF",  /* U+093F devanagari vowel sign i */
    /* 10 'm' */ "\xE0\xA4\x83",  /* U+0903 devanagari sign visarga */
    /* 11 'n' */ "\xE0\xA4\xB2",  /* U+0932 devanagari letter la */
    /* 12 'o' */ "\xE0\xA4\xAF",  /* U+092F devanagari letter ya */
    /* 13 'p' */ "\xE0\xA4\x89",  /* U+0909 devanagari letter u */
    /* 14 'q' */ "\xE0\xA4\xA4\xE0\xA5\x8D\xE0\xA4\xB0",  /* U+0924 devanagari letter ta U+094D devanagari sign virama U+0930 devanagari letter ra */
    /* 15 'r' */ "\xE0\xA4\x9A",  /* U+091A devanagari letter ca */
    /* 16 's' */ "\xE0\xA4\x95",  /* U+0915 devanagari letter ka */
    /* 17 't' */ "\xE0\xA4\xA4",  /* U+0924 devanagari letter ta */
    /* 18 'u' */ "\xE0\xA4\x97",  /* U+0917 devanagari letter ga */
    /* 19 'v' */ "\xE0\xA4\x96",  /* U+0916 devanagari letter kha */
    /* 1A 'w' */ "\xE0\xA4\xA7",  /* U+0927 devanagari letter dha */
    /* 1B 'x' */ "\xE0\xA4\xB9",  /* U+0939 devanagari letter ha */
    /* 1C 'y' */ "\xE0\xA4\xA5",  /* U+0925 devanagari letter tha */
    /* 1D 'z' */ "\xE0\xA4\xB6",  /* U+0936 devanagari letter sha */
    /* 1E '1' */ "\xE0\xA5\xA7",  /* U+0967 devanagari digit one */
    /* 1F '2' */ "\xE0\xA5\xA8",  /* U+0968 devanagari digit two */
    /* 20 '3' */ "\xE0\xA5\xA9",  /* U+0969 devanagari digit three */
    /* 21 '4' */ "\xE0\xA5\xAA",  /* U+096A devanagari digit four */
    /* 22 '5' */ "\xE0\xA5\xAB",  /* U+096B devanagari digit five */
    /* 23 '6' */ "\xE0\xA5\xAC",  /* U+096C devanagari digit six */
    /* 24 '7' */ "\xE0\xA5\xAD",  /* U+096D devanagari digit seven */
    /* 25 '8' */ "\xE0\xA5\xAE",  /* U+096E devanagari digit eight */
    /* 26 '9' */ "\xE0\xA5\xAF",  /* U+096F devanagari digit nine */
    /* 27 '0' */ "\xE0\xA5\xA6",  /* U+0966 devanagari digit zero */
    /* 28  */ NULL,
    /* 29  */ NULL,
    /* 2A  */ NULL,
    /* 2B  */ NULL,
    /* 2C ' ' */ " ",
    /* 2D '-' */ "\xE0\xA4\x94",  /* U+0914 devanagari letter au */
    /* 2E '=' */ "\xE2\x80\x8D",  /* U+200D zero width joiner */
    /* 2F '[' */ "\xE0\xA4\xB0\xE0\xA5\x8D",  /* U+0930 devanagari letter ra U+094D devanagari sign virama */
    /* 30 ']' */ "\xE0\xA5\x87",  /* U+0947 devanagari vowel sign e */
    /* 31 '\\' */ "\xE0\xA5\x8D",  /* U+094D devanagari sign virama */
    /* 32  */ NULL,
    /* 33 ';' */ "\xE0\xA4\xB8",  /* U+0938 devanagari letter sa */
    /* 34 "'" */ "\xE0\xA5\x81",  /* U+0941 devanagari vowel sign u */
    /* 35 '`' */ "\xE0\xA4\x9E",  /* U+091E devanagari letter nya */
    /* 36 ',' */ "\xE0\xA4\xBD",  /* U+093D devanagari sign avagraha */
    /* 37 '.' */ "\xE0\xA5\xA4",  /* U+0964 devanagari danda */
    /* 38 '/' */ "\xE0\xA4\xB0",  /* U+0930 devanagari letter ra */
};

static const char *const NE_TR_SHIFT[] = {
    /* 04 'A' */ "\xE0\xA4\x86",  /* U+0906 devanagari letter aa */
    /* 05 'B' */ "\xE0\xA5\x8C",  /* U+094C devanagari vowel sign au */
    /* 06 'C' */ "\xE0\xA4\x8B",  /* U+090B devanagari letter vocalic r */
    /* 07 'D' */ "\xE0\xA4\x99\xE0\xA5\x8D\xE0\xA4\x97",  /* U+0919 devanagari letter nga U+094D devanagari sign virama U+0917 devanagari letter ga */
    /* 08 'E' */ "\xE0\xA4\x90",  /* U+0910 devanagari letter ai */
    /* 09 'F' */ "\xE0\xA4\x81",  /* U+0901 devanagari sign candrabindu */
    /* 0A 'G' */ "\xE0\xA4\xA6\xE0\xA5\x8D\xE0\xA4\xA6",  /* U+0926 devanagari letter da U+094D devanagari sign virama U+0926 devanagari letter da */
    /* 0B 'H' */ "\xE0\xA4\x9D",  /* U+091D devanagari letter jha */
    /* 0C 'I' */ "\xE0\xA4\x95\xE0\xA5\x8D\xE0\xA4\xB7",  /* U+0915 devanagari letter ka U+094D devanagari sign virama U+0937 devanagari letter ssa */
    /* 0D 'J' */ "\xE0\xA5\x8B",  /* U+094B devanagari vowel sign o */
    /* 0E 'K' */ "\xE0\xA4\xAB",  /* U+092B devanagari letter pha */
    /* 0F 'L' */ "\xE0\xA5\x80",  /* U+0940 devanagari vowel sign ii */
    /* 10 'M' */ "\xE0\xA4\xA1\xE0\xA5\x8D\xE0\xA4\xA1",  /* U+0921 devanagari letter dda U+094D devanagari sign virama U+0921 devanagari letter dda */
    /* 11 'N' */ "\xE0\xA4\xA6\xE0\xA5\x8D\xE0\xA4\xAF",  /* U+0926 devanagari letter da U+094D devanagari sign virama U+092F devanagari letter ya */
    /* 12 'O' */ "\xE0\xA4\x87",  /* U+0907 devanagari letter i */
    /* 13 'P' */ "\xE0\xA4\x8F",  /* U+090F devanagari letter e */
    /* 14 'Q' */ "\xE0\xA4\xA4\xE0\xA5\x8D\xE0\xA4\xA4",  /* U+0924 devanagari letter ta U+094D devanagari sign virama U+0924 devanagari letter ta */
    /* 15 'R' */ "\xE0\xA4\xA6\xE0\xA5\x8D\xE0\xA4\xAC",  /* U+0926 devanagari letter da U+094D devanagari sign virama U+092C devanagari letter ba */
    /* 16 'S' */ "\xE0\xA4\x99\xE0\xA5\x8D\xE0\xA4\x95",  /* U+0919 devanagari letter nga U+094D devanagari sign virama U+0915 devanagari letter ka */
    /* 17 'T' */ "\xE0\xA4\x9F\xE0\xA5\x8D\xE0\xA4\x9F",  /* U+091F devanagari letter tta U+094D devanagari sign virama U+091F devanagari letter tta */
    /* 18 'U' */ "\xE0\xA4\x8A",  /* U+090A devanagari letter uu */
    /* 19 'V' */ "\xE0\xA5\x90",  /* U+0950 devanagari om */
    /* 1A 'W' */ "\xE0\xA4\xA1\xE0\xA5\x8D\xE0\xA4\xA2",  /* U+0921 devanagari letter dda U+094D devanagari sign virama U+0922 devanagari letter ddha */
    /* 1B 'X' */ "\xE0\xA4\xB9\xE0\xA5\x8D\xE0\xA4\xAF",  /* U+0939 devanagari letter ha U+094D devanagari sign virama U+092F devanagari letter ya */
    /* 1C 'Y' */ "\xE0\xA4\xA0\xE0\xA5\x8D\xE0\xA4\xA0",  /* U+0920 devanagari letter ttha U+094D devanagari sign virama U+0920 devanagari letter ttha */
    /* 1D 'Z' */ "\xE0\xA4\x95\xE0\xA5\x8D\xE0\xA4\x95",  /* U+0915 devanagari letter ka U+094D devanagari sign virama U+0915 devanagari letter ka */
    /* 1E '!' */ "\xE0\xA4\x9C\xE0\xA5\x8D\xE0\xA4\x9E",  /* U+091C devanagari letter ja U+094D devanagari sign virama U+091E devanagari letter nya */
    /* 1F '@' */ "\xE0\xA4\x88",  /* U+0908 devanagari letter ii */
    /* 20 '#' */ "\xE0\xA4\x98",  /* U+0918 devanagari letter gha */
    /* 21 '$' */ "\xE0\xA4\xA6\xE0\xA5\x8D\xE0\xA4\xA7",  /* U+0926 devanagari letter da U+094D devanagari sign virama U+0927 devanagari letter dha */
    /* 22 '%' */ "\xE0\xA4\x9B",  /* U+091B devanagari letter cha */
    /* 23 '^' */ "\xE0\xA4\x9F",  /* U+091F devanagari letter tta */
    /* 24 '&' */ "\xE0\xA4\xA0",  /* U+0920 devanagari letter ttha */
    /* 25 '*' */ "\xE0\xA4\xA1",  /* U+0921 devanagari letter dda */
    /* 26 '(' */ "\xE0\xA4\xA2",  /* U+0922 devanagari letter ddha */
    /* 27 ')' */ "\xE0\xA4\xA3",  /* U+0923 devanagari letter nna */
    /* 28  */ NULL,
    /* 29  */ NULL,
    /* 2A  */ NULL,
    /* 2B  */ NULL,
    /* 2C ' ' */ " ",
    /* 2D '_' */ "\xE0\xA4\x93",  /* U+0913 devanagari letter o */
    /* 2E '+' */ "\xE2\x80\x8C",  /* U+200C zero width non-joiner */
    /* 2F '{' */ "\xE0\xA5\x83",  /* U+0943 devanagari vowel sign vocalic r */
    /* 30 '}' */ "\xE0\xA5\x88",  /* U+0948 devanagari vowel sign ai */
    /* 31 '|' */ "\xE0\xA4\x82",  /* U+0902 devanagari sign anusvara */
    /* 32  */ NULL,
    /* 33 ':' */ "\xE0\xA4\x9F\xE0\xA5\x8D\xE0\xA4\xA0",  /* U+091F devanagari letter tta U+094D devanagari sign virama U+0920 devanagari letter ttha */
    /* 34 '"' */ "\xE0\xA5\x82",  /* U+0942 devanagari vowel sign uu */
    /* 35 '~' */ "\xE0\xA5\xA5",  /* U+0965 devanagari double danda */
    /* 36 '<' */ "\xE0\xA4\x99",  /* U+0919 devanagari letter nga */
    /* 37 '>' */ "\xE0\xA4\xB6\xE0\xA5\x8D\xE0\xA4\xB0",  /* U+0936 devanagari letter sha U+094D devanagari sign virama U+0930 devanagari letter ra */
    /* 38 '?' */ "\xE0\xA4\xB0\xE0\xA5\x81",  /* U+0930 devanagari letter ra U+0941 devanagari vowel sign u */
};
#endif

#ifdef CONFIG_KB_LAYOUT_ENABLE_NE_RO
static const char *const NE_RO_NORMAL[] = {
    /* 04 'a' */ "\xE0\xA4\xBE",  /* U+093E devanagari vowel sign aa */
    /* 05 'b' */ "\xE0\xA4\xAC",  /* U+092C devanagari letter ba */
    /* 06 'c' */ "\xE0\xA4\x9B",  /* U+091B devanagari letter cha */
    /* 07 'd' */ "\xE0\xA4\xA6",  /* U+0926 devanagari letter da */
    /* 08 'e' */ "\xE0\xA5\x87",  /* U+0947 devanagari vowel sign e */
    /* 09 'f' */ "\xE0\xA4\x89",  /* U+0909 devanagari letter u */
    /* 0A 'g' */ "\xE0\xA4\x97",  /* U+0917 devanagari letter ga */
    /* 0B 'h' */ "\xE0\xA4\xB9",  /* U+0939 devanagari letter ha */
    /* 0C 'i' */ "\xE0\xA4\xBF",  /* U+093F devanagari vowel sign i */
    /* 0D 'j' */ "\xE0\xA4\x9C",  /* U+091C devanagari letter ja */
    /* 0E 'k' */ "\xE0\xA4\x95",  /* U+0915 devanagari letter ka */
    /* 0F 'l' */ "\xE0\xA4\xB2",  /* U+0932 devanagari letter la */
    /* 10 'm' */ "\xE0\xA4\xAE",  /* U+092E devanagari letter ma */
    /* 11 'n' */ "\xE0\xA4\xA8",  /* U+0928 devanagari letter na */
    /* 12 'o' */ "\xE0\xA5\x8B",  /* U+094B devanagari vowel sign o */
    /* 13 'p' */ "\xE0\xA4\xAA",  /* U+092A devanagari letter pa */
    /* 14 'q' */ "\xE0\xA4\x9F",  /* U+091F devanagari letter tta */
    /* 15 'r' */ "\xE0\xA4\xB0",  /* U+0930 devanagari letter ra */
    /* 16 's' */ "\xE0\xA4\xB8",  /* U+0938 devanagari letter sa */
    /* 17 't' */ "\xE0\xA4\xA4",  /* U+0924 devanagari letter ta */
    /* 18 'u' */ "\xE0\xA5\x81",  /* U+0941 devanagari vowel sign u */
    /* 19 'v' */ "\xE0\xA4\xB5",  /* U+0935 devanagari letter va */
    /* 1A 'w' */ "\xE0\xA5\x8C",  /* U+094C devanagari vowel sign au */
    /* 1B 'x' */ "\xE0\xA4\xA1",  /* U+0921 devanagari letter dda */
    /* 1C 'y' */ "\xE0\xA4\xAF",  /* U+092F devanagari letter ya */
    /* 1D 'z' */ "\xE0\xA4\xB7",  /* U+0937 devanagari letter ssa */
    /* 1E '1' */ "\xE0\xA5\xA7",  /* U+0967 devanagari digit one */
    /* 1F '2' */ "\xE0\xA5\xA8",  /* U+0968 devanagari digit two */
    /* 20 '3' */ "\xE0\xA5\xA9",  /* U+0969 devanagari digit three */
    /* 21 '4' */ "\xE0\xA5\xAA",  /* U+096A devanagari digit four */
    /* 22 '5' */ "\xE0\xA5\xAB",  /* U+096B devanagari digit five */
    /* 23 '6' */ "\xE0\xA5\xAC",  /* U+096C devanagari digit six */
    /* 24 '7' */ "\xE0\xA5\xAD",  /* U+096D devanagari digit seven */
    /* 25 '8' */ "\xE0\xA5\xAE",  /* U+096E devanagari digit eight */
    /* 26 '9' */ "\xE0\xA5\xAF",  /* U+096F devanagari digit nine */
    /* 27 '0' */ "\xE0\xA5\xA6",  /* U+0966 devanagari digit zero */
    /* 28  */ NULL,
    /* 29  */ NULL,
    /* 2A  */ NULL,
    /* 2B  */ NULL,
    /* 2C ' ' */ " ",
    /* 2D '-' */ "-",
    /* 2E '=' */ "\xE2\x80\x8D",  /* U+200D zero width joiner */
    /* 2F '[' */ "\xE0\xA4\x87",  /* U+0907 devanagari letter i */
    /* 30 ']' */ "\xE0\xA4\x8F",  /* U+090F devanagari letter e */
    /* 31 '\\' */ "\xE0\xA5\x90",  /* U+0950 devanagari om */
    /* 32  */ NULL,
    /* 33 ';' */ ";",
    /* 34 "'" */ "'",
    /* 35 '`' */ "\xE0\xA4\xBC",  /* U+093C devanagari sign nukta */
    /* 36 ',' */ ",",
    /* 37 '.' */ "\xE0\xA5\xA4",  /* U+0964 devanagari danda */
    /* 38 '/' */ "\xE0\xA5\x8D",  /* U+094D devanagari sign virama */
};

static const char *const NE_RO_SHIFT[] = {
    /* 04 'A' */ "\xE0\xA4\x86",  /* U+0906 devanagari letter aa */
    /* 05 'B' */ "\xE0\xA4\xAD",  /* U+092D devanagari letter bha */
    /* 06 'C' */ "\xE0\xA4\x9A",  /* U+091A devanagari letter ca */
    /* 07 'D' */ "\xE0\xA4\xA7",  /* U+0927 devanagari letter dha */
    /* 08 'E' */ "\xE0\xA5\x88",  /* U+0948 devanagari vowel sign ai */
    /* 09 'F' */ "\xE0\xA4\x8A",  /* U+090A devanagari letter uu */
    /* 0A 'G' */ "\xE0\xA4\x98",  /* U+0918 devanagari letter gha */
    /* 0B 'H' */ "\xE0\xA4\x85",  /* U+0905 devanagari letter a */
    /* 0C 'I' */ "\xE0\xA5\x80",  /* U+0940 devanagari vowel sign ii */
    /* 0D 'J' */ "\xE0\xA4\x9D",  /* U+091D devanagari letter jha */
    /* 0E 'K' */ "\xE0\xA4\x96",  /* U+0916 devanagari letter kha */
    /* 0F 'L' */ "\xE0\xA5\xA5",  /* U+0965 devanagari double danda */
    /* 10 'M' */ "\xE0\xA4\x82",  /* U+0902 devanagari sign anusvara */
    /* 11 'N' */ "\xE0\xA4\xA3",  /* U+0923 devanagari letter nna */
    /* 12 'O' */ "\xE0\xA4\x93",  /* U+0913 devanagari letter o */
    /* 13 'P' */ "\xE0\xA4\xAB",  /* U+092B devanagari letter pha */
    /* 14 'Q' */ "\xE0\xA4\xA0",  /* U+0920 devanagari letter ttha */
    /* 15 'R' */ "\xE0\xA5\x83",  /* U+0943 devanagari vowel sign vocalic r */
    /* 16 'S' */ "\xE0\xA4\xB6",  /* U+0936 devanagari letter sha */
    /* 17 'T' */ "\xE0\xA4\xA5",  /* U+0925 devanagari letter tha */
    /* 18 'U' */ "\xE0\xA5\x82",  /* U+0942 devanagari vowel sign uu */
    /* 19 'V' */ "\xE0\xA4\x81",  /* U+0901 devanagari sign candrabindu */
    /* 1A 'W' */ "\xE0\xA4\x94",  /* U+0914 devanagari letter au */
    /* 1B 'X' */ "\xE0\xA4\xA2",  /* U+0922 devanagari letter ddha */
    /* 1C 'Y' */ "\xE0\xA4\x9E",  /* U+091E devanagari letter nya */
    /* 1D 'Z' */ "\xE0\xA4\x8B",  /* U+090B devanagari letter vocalic r */
    /* 1E '!' */ "!",
    /* 1F '@' */ "@",
    /* 20 '#' */ "#",
    /* 21 '$' */ "\xE0\xA4\xB0\xE0\xA5\x81",  /* U+0930 devanagari letter ra U+0941 devanagari vowel sign u */
    /* 22 '%' */ "%",
    /* 23 '^' */ "^",
    /* 24 '&' */ "&",
    /* 25 '*' */ "*",
    /* 26 '(' */ "(",
    /* 27 ')' */ ")",
    /* 28  */ NULL,
    /* 29  */ NULL,
    /* 2A  */ NULL,
    /* 2B  */ NULL,
    /* 2C ' ' */ " ",
    /* 2D '_' */ "_",
    /* 2E '+' */ "\xE2\x80\x8C",  /* U+200C zero width non-joiner */
    /* 2F '{' */ "\xE0\xA4\x88",  /* U+0908 devanagari letter ii */
    /* 30 '}' */ "\xE0\xA4\x90",  /* U+0910 devanagari letter ai */
    /* 31 '|' */ "\xE0\xA4\x83",  /* U+0903 devanagari sign visarga */
    /* 32  */ NULL,
    /* 33 ':' */ ":",
    /* 34 '"' */ "\"",
    /* 35 '~' */ "\xE0\xA4\xBD",  /* U+093D devanagari sign avagraha */
    /* 36 '<' */ "\xE0\xA4\x99",  /* U+0919 devanagari letter nga */
    /* 37 '>' */ "\xE0\xA5\xA5",  /* U+0965 devanagari double danda */
    /* 38 '?' */ "?",
};
#endif
#endif /* Devanagari */

#if defined(CONFIG_KB_LAYOUT_ENABLE_BN_IN) || defined(CONFIG_KB_LAYOUT_ENABLE_BN_PR) || \
    defined(CONFIG_KB_LAYOUT_ENABLE_BN_NJ)
/* ------------------------------------------------------------------ */
/* Bengali: InScript, Probhat and National (Jatiya)                   */
/* ------------------------------------------------------------------ */

/* Generated from the m17n-db input methods "bn-inscript2" (Enhanced
 * InScript), "bn-probhat" and "bn-national-jatiya" (keys typed on a US
 * keyboard -> output), with the InScript keys m17n leaves empty taken
 * from XKB "in(ben_inscript)" (marked; m17n's z, U+02BC modifier
 * apostrophe, which the fonts lack, too). Text is typed in Unicode
 * order (consonant, then vowel sign); HarfBuzz shapes it for display.
 * Keys without an m17n entry keep their US character, except letters,
 * which type nothing. The National (Jatiya) table gives the vowel
 * signs; its input method (components/ime/ime_bengali.c) types the
 * independent vowel instead where no consonant comes before, as m17n's
 * "automatic vowel forming" does. */

#ifdef CONFIG_KB_LAYOUT_ENABLE_BN_IN
static const char *const BN_IN_NORMAL[] = {
    /* 04 'a' */ "\xE0\xA7\x8B",  /* U+09CB bengali vowel sign o */
    /* 05 'b' */ "\xE0\xA6\xAC",  /* U+09AC bengali letter ba (XKB in(ben_inscript)) */
    /* 06 'c' */ "\xE0\xA6\xAE",  /* U+09AE bengali letter ma */
    /* 07 'd' */ "\xE0\xA7\x8D",  /* U+09CD bengali sign virama */
    /* 08 'e' */ "\xE0\xA6\xBE",  /* U+09BE bengali vowel sign aa */
    /* 09 'f' */ "\xE0\xA6\xBF",  /* U+09BF bengali vowel sign i */
    /* 0A 'g' */ "\xE0\xA7\x81",  /* U+09C1 bengali vowel sign u */
    /* 0B 'h' */ "\xE0\xA6\xAA",  /* U+09AA bengali letter pa */
    /* 0C 'i' */ "\xE0\xA6\x97",  /* U+0997 bengali letter ga */
    /* 0D 'j' */ "\xE0\xA6\xB0",  /* U+09B0 bengali letter ra */
    /* 0E 'k' */ "\xE0\xA6\x95",  /* U+0995 bengali letter ka */
    /* 0F 'l' */ "\xE0\xA6\xA4",  /* U+09A4 bengali letter ta */
    /* 10 'm' */ "\xE0\xA6\xB8",  /* U+09B8 bengali letter sa */
    /* 11 'n' */ "\xE0\xA6\xB2",  /* U+09B2 bengali letter la */
    /* 12 'o' */ "\xE0\xA6\xA6",  /* U+09A6 bengali letter da */
    /* 13 'p' */ "\xE0\xA6\x9C",  /* U+099C bengali letter ja */
    /* 14 'q' */ "\xE0\xA7\x8C",  /* U+09CC bengali vowel sign au */
    /* 15 'r' */ "\xE0\xA7\x80",  /* U+09C0 bengali vowel sign ii */
    /* 16 's' */ "\xE0\xA7\x87",  /* U+09C7 bengali vowel sign e */
    /* 17 't' */ "\xE0\xA7\x82",  /* U+09C2 bengali vowel sign uu */
    /* 18 'u' */ "\xE0\xA6\xB9",  /* U+09B9 bengali letter ha */
    /* 19 'v' */ "\xE0\xA6\xA8",  /* U+09A8 bengali letter na */
    /* 1A 'w' */ "\xE0\xA7\x88",  /* U+09C8 bengali vowel sign ai */
    /* 1B 'x' */ "\xE0\xA6\x82",  /* U+0982 bengali sign anusvara */
    /* 1C 'y' */ "\xE0\xA6\xAC",  /* U+09AC bengali letter ba */
    /* 1D 'z' */ "\xE0\xA7\x8E",  /* U+09CE bengali letter khanda ta (XKB in(ben_inscript)) */
    /* 1E '1' */ "\xE0\xA7\xA7",  /* U+09E7 bengali digit one */
    /* 1F '2' */ "\xE0\xA7\xA8",  /* U+09E8 bengali digit two */
    /* 20 '3' */ "\xE0\xA7\xA9",  /* U+09E9 bengali digit three */
    /* 21 '4' */ "\xE0\xA7\xAA",  /* U+09EA bengali digit four */
    /* 22 '5' */ "\xE0\xA7\xAB",  /* U+09EB bengali digit five */
    /* 23 '6' */ "\xE0\xA7\xAC",  /* U+09EC bengali digit six */
    /* 24 '7' */ "\xE0\xA7\xAD",  /* U+09ED bengali digit seven */
    /* 25 '8' */ "\xE0\xA7\xAE",  /* U+09EE bengali digit eight */
    /* 26 '9' */ "\xE0\xA7\xAF",  /* U+09EF bengali digit nine */
    /* 27 '0' */ "\xE0\xA7\xA6",  /* U+09E6 bengali digit zero */
    /* 28  */ NULL,
    /* 29  */ NULL,
    /* 2A  */ NULL,
    /* 2B  */ NULL,
    /* 2C ' ' */ " ",
    /* 2D '-' */ "-",
    /* 2E '=' */ "\xE0\xA7\x83",  /* U+09C3 bengali vowel sign vocalic r */
    /* 2F '[' */ "\xE0\xA6\xA1",  /* U+09A1 bengali letter dda */
    /* 30 ']' */ "\xE0\xA6\xBC",  /* U+09BC bengali sign nukta */
    /* 31 '\\' */ "\\",
    /* 32  */ NULL,
    /* 33 ';' */ "\xE0\xA6\x9A",  /* U+099A bengali letter ca */
    /* 34 "'" */ "\xE0\xA6\x9F",  /* U+099F bengali letter tta */
    /* 35 '`' */ "\xE2\x80\x8D",  /* U+200D zero width joiner (XKB in(ben_inscript)) */
    /* 36 ',' */ ",",
    /* 37 '.' */ ".",
    /* 38 '/' */ "\xE0\xA7\x9F",  /* U+09DF bengali letter yya */
};

static const char *const BN_IN_SHIFT[] = {
    /* 04 'A' */ "\xE0\xA6\x93",  /* U+0993 bengali letter o */
    /* 05 'B' */ "\xE0\xA6\xAC",  /* U+09AC bengali letter ba (XKB in(ben_inscript)) */
    /* 06 'C' */ "\xE0\xA6\xA3",  /* U+09A3 bengali letter nna */
    /* 07 'D' */ "\xE0\xA6\x85",  /* U+0985 bengali letter a */
    /* 08 'E' */ "\xE0\xA6\x86",  /* U+0986 bengali letter aa */
    /* 09 'F' */ "\xE0\xA6\x87",  /* U+0987 bengali letter i */
    /* 0A 'G' */ "\xE0\xA6\x89",  /* U+0989 bengali letter u */
    /* 0B 'H' */ "\xE0\xA6\xAB",  /* U+09AB bengali letter pha */
    /* 0C 'I' */ "\xE0\xA6\x98",  /* U+0998 bengali letter gha */
    /* 0D 'J' */ "\xE0\xA7\x9D",  /* U+09DD bengali letter rha (XKB in(ben_inscript)) */
    /* 0E 'K' */ "\xE0\xA6\x96",  /* U+0996 bengali letter kha */
    /* 0F 'L' */ "\xE0\xA6\xA5",  /* U+09A5 bengali letter tha */
    /* 10 'M' */ "\xE0\xA6\xB6",  /* U+09B6 bengali letter sha */
    /* 11 'N' */ "\xE0\xA6\xB2",  /* U+09B2 bengali letter la (XKB in(ben_inscript)) */
    /* 12 'O' */ "\xE0\xA6\xA7",  /* U+09A7 bengali letter dha */
    /* 13 'P' */ "\xE0\xA6\x9D",  /* U+099D bengali letter jha */
    /* 14 'Q' */ "\xE0\xA6\x94",  /* U+0994 bengali letter au */
    /* 15 'R' */ "\xE0\xA6\x88",  /* U+0988 bengali letter ii */
    /* 16 'S' */ "\xE0\xA6\x8F",  /* U+098F bengali letter e */
    /* 17 'T' */ "\xE0\xA6\x8A",  /* U+098A bengali letter uu */
    /* 18 'U' */ "\xE0\xA6\x99",  /* U+0999 bengali letter nga */
    /* 19 'V' */ "\xE0\xA6\xA8",  /* U+09A8 bengali letter na (XKB in(ben_inscript)) */
    /* 1A 'W' */ "\xE0\xA6\x90",  /* U+0990 bengali letter ai */
    /* 1B 'X' */ "\xE0\xA6\x81",  /* U+0981 bengali sign candrabindu */
    /* 1C 'Y' */ "\xE0\xA6\xAD",  /* U+09AD bengali letter bha */
    /* 1D 'Z' */ NULL,
    /* 1E '!' */ "\xE0\xA6\x85\xE0\xA7\x8D\xE0\xA6\xAF\xE0\xA6\xBE",  /* U+0985 bengali letter a U+09CD bengali sign virama U+09AF bengali letter ya U+09BE bengali vowel sign aa */
    /* 1F '@' */ "@",
    /* 20 '#' */ "\xE0\xA7\x8D\xE0\xA6\xB0",  /* U+09CD bengali sign virama U+09B0 bengali letter ra */
    /* 21 '$' */ "\xE0\xA6\xB0\xE0\xA7\x8D",  /* U+09B0 bengali letter ra U+09CD bengali sign virama */
    /* 22 '%' */ "\xE0\xA6\x9C\xE0\xA7\x8D\xE0\xA6\x9E",  /* U+099C bengali letter ja U+09CD bengali sign virama U+099E bengali letter nya */
    /* 23 '^' */ "\xE0\xA6\xA4\xE0\xA7\x8D\xE0\xA6\xB0",  /* U+09A4 bengali letter ta U+09CD bengali sign virama U+09B0 bengali letter ra */
    /* 24 '&' */ "\xE0\xA6\x95\xE0\xA7\x8D\xE0\xA6\xB7",  /* U+0995 bengali letter ka U+09CD bengali sign virama U+09B7 bengali letter ssa */
    /* 25 '*' */ "\xE0\xA6\xB6\xE0\xA7\x8D\xE0\xA6\xB0",  /* U+09B6 bengali letter sha U+09CD bengali sign virama U+09B0 bengali letter ra */
    /* 26 '(' */ "(",
    /* 27 ')' */ ")",
    /* 28  */ NULL,
    /* 29  */ NULL,
    /* 2A  */ NULL,
    /* 2B  */ NULL,
    /* 2C ' ' */ " ",
    /* 2D '_' */ "\xE0\xA6\x83",  /* U+0983 bengali sign visarga */
    /* 2E '+' */ "\xE0\xA6\x8B",  /* U+098B bengali letter vocalic r */
    /* 2F '{' */ "\xE0\xA6\xA2",  /* U+09A2 bengali letter ddha */
    /* 30 '}' */ "\xE0\xA6\x9E",  /* U+099E bengali letter nya */
    /* 31 '|' */ "|",
    /* 32  */ NULL,
    /* 33 ':' */ "\xE0\xA6\x9B",  /* U+099B bengali letter cha */
    /* 34 '"' */ "\xE0\xA6\xA0",  /* U+09A0 bengali letter ttha */
    /* 35 '~' */ "\xE2\x80\x8C",  /* U+200C zero width non-joiner (XKB in(ben_inscript)) */
    /* 36 '<' */ "\xE0\xA6\xB7",  /* U+09B7 bengali letter ssa */
    /* 37 '>' */ "\xE0\xA5\xA4",  /* U+0964 devanagari danda */
    /* 38 '?' */ "\xE0\xA6\xAF",  /* U+09AF bengali letter ya */
};

static const key_entry_t BN_IN_EXTRA[] = {
    { 0x1E, L_ALTGR, "\xE2\x80\x8D" },  /* U+200D zero width joiner */
    { 0x1E, L_ALTGR_SHIFT, "\xE0\xA7\xB4" },  /* U+09F4 bengali currency numerator one */
    { 0x1F, L_ALTGR_SHIFT, "\xE0\xA7\xB5" },  /* U+09F5 bengali currency numerator two */
    { 0x1F, L_ALTGR, "\xE2\x80\x8C" },  /* U+200C zero width non-joiner */
    { 0x20, L_ALTGR_SHIFT, "\xE0\xA7\xB6" },  /* U+09F6 bengali currency numerator three */
    { 0x21, L_ALTGR_SHIFT, "\xE0\xA7\xB7" },  /* U+09F7 bengali currency numerator four */
    { 0x21, L_ALTGR, "\xE2\x82\xB9" },  /* U+20B9 indian rupee sign */
    { 0x22, L_ALTGR_SHIFT, "\xE0\xA7\xB8" },  /* U+09F8 bengali currency numerator one less than the denominator */
    { 0x23, L_ALTGR_SHIFT, "\xE0\xA7\xB9" },  /* U+09F9 bengali currency denominator sixteen */
    { 0x2E, L_ALTGR, "\xE0\xA7\x84" },  /* U+09C4 bengali vowel sign vocalic rr */
    { 0x2E, L_ALTGR_SHIFT, "\xE0\xA7\xA0" },  /* U+09E0 bengali letter vocalic rr */
    { 0x15, L_ALTGR_SHIFT, "\xE0\xA7\xA1" },  /* U+09E1 bengali letter vocalic ll */
    { 0x15, L_ALTGR, "\xE0\xA7\xA3" },  /* U+09E3 bengali vowel sign vocalic ll */
    { 0x2F, L_ALTGR_SHIFT, "\xE0\xA7\x9D" },  /* U+09DD bengali letter rha */
    { 0x2F, L_ALTGR, "\xE0\xA7\x9C" },  /* U+09DC bengali letter rra */
    { 0x09, L_ALTGR_SHIFT, "\xE0\xA6\x8C" },  /* U+098C bengali letter vocalic l */
    { 0x09, L_ALTGR, "\xE0\xA7\xA2" },  /* U+09E2 bengali vowel sign vocalic l */
    { 0x0F, L_ALTGR, "\xE0\xA7\x8E" },  /* U+09CE bengali letter khanda ta */
    { 0x1B, L_ALTGR, "\xE0\xA7\xBA" },  /* U+09FA bengali isshar */
    { 0x36, L_ALTGR_SHIFT, "\xE0\xA7\xB2" },  /* U+09F2 bengali rupee mark */
    { 0x36, L_ALTGR, "\xE0\xA7\xB3" },  /* U+09F3 bengali rupee sign */
    { 0x37, L_ALTGR_SHIFT, "\xE0\xA6\xBD" },  /* U+09BD bengali sign avagraha */
    { 0x37, L_ALTGR, "\xE0\xA5\xA5" },  /* U+0965 devanagari double danda */
    { 0x38, L_ALTGR_SHIFT, "\xE0\xA7\xBB" },  /* U+09FB bengali ganda mark */
    { 0x38, L_ALTGR, "\xE0\xA7\x8D\xE0\xA6\xAF" },  /* U+09CD bengali sign virama U+09AF bengali letter ya */
    { 0, 0, NULL },
};
#endif

#ifdef CONFIG_KB_LAYOUT_ENABLE_BN_PR
static const char *const BN_PR_NORMAL[] = {
    /* 04 'a' */ "\xE0\xA6\xBE",  /* U+09BE bengali vowel sign aa */
    /* 05 'b' */ "\xE0\xA6\xAC",  /* U+09AC bengali letter ba */
    /* 06 'c' */ "\xE0\xA6\x9A",  /* U+099A bengali letter ca */
    /* 07 'd' */ "\xE0\xA6\xA1",  /* U+09A1 bengali letter dda */
    /* 08 'e' */ "\xE0\xA7\x80",  /* U+09C0 bengali vowel sign ii */
    /* 09 'f' */ "\xE0\xA6\xA4",  /* U+09A4 bengali letter ta */
    /* 0A 'g' */ "\xE0\xA6\x97",  /* U+0997 bengali letter ga */
    /* 0B 'h' */ "\xE0\xA6\xB9",  /* U+09B9 bengali letter ha */
    /* 0C 'i' */ "\xE0\xA6\xBF",  /* U+09BF bengali vowel sign i */
    /* 0D 'j' */ "\xE0\xA6\x9C",  /* U+099C bengali letter ja */
    /* 0E 'k' */ "\xE0\xA6\x95",  /* U+0995 bengali letter ka */
    /* 0F 'l' */ "\xE0\xA6\xB2",  /* U+09B2 bengali letter la */
    /* 10 'm' */ "\xE0\xA6\xAE",  /* U+09AE bengali letter ma */
    /* 11 'n' */ "\xE0\xA6\xA8",  /* U+09A8 bengali letter na */
    /* 12 'o' */ "\xE0\xA6\x93",  /* U+0993 bengali letter o */
    /* 13 'p' */ "\xE0\xA6\xAA",  /* U+09AA bengali letter pa */
    /* 14 'q' */ "\xE0\xA6\xA6",  /* U+09A6 bengali letter da */
    /* 15 'r' */ "\xE0\xA6\xB0",  /* U+09B0 bengali letter ra */
    /* 16 's' */ "\xE0\xA6\xB8",  /* U+09B8 bengali letter sa */
    /* 17 't' */ "\xE0\xA6\x9F",  /* U+099F bengali letter tta */
    /* 18 'u' */ "\xE0\xA7\x81",  /* U+09C1 bengali vowel sign u */
    /* 19 'v' */ "\xE0\xA6\x86",  /* U+0986 bengali letter aa */
    /* 1A 'w' */ "\xE0\xA7\x82",  /* U+09C2 bengali vowel sign uu */
    /* 1B 'x' */ "\xE0\xA6\xB6",  /* U+09B6 bengali letter sha */
    /* 1C 'y' */ "\xE0\xA6\x8F",  /* U+098F bengali letter e */
    /* 1D 'z' */ "\xE0\xA7\x9F",  /* U+09DF bengali letter yya */
    /* 1E '1' */ "\xE0\xA7\xA7",  /* U+09E7 bengali digit one */
    /* 1F '2' */ "\xE0\xA7\xA8",  /* U+09E8 bengali digit two */
    /* 20 '3' */ "\xE0\xA7\xA9",  /* U+09E9 bengali digit three */
    /* 21 '4' */ "\xE0\xA7\xAA",  /* U+09EA bengali digit four */
    /* 22 '5' */ "\xE0\xA7\xAB",  /* U+09EB bengali digit five */
    /* 23 '6' */ "\xE0\xA7\xAC",  /* U+09EC bengali digit six */
    /* 24 '7' */ "\xE0\xA7\xAD",  /* U+09ED bengali digit seven */
    /* 25 '8' */ "\xE0\xA7\xAE",  /* U+09EE bengali digit eight */
    /* 26 '9' */ "\xE0\xA7\xAF",  /* U+09EF bengali digit nine */
    /* 27 '0' */ "\xE0\xA7\xA6",  /* U+09E6 bengali digit zero */
    /* 28  */ NULL,
    /* 29  */ NULL,
    /* 2A  */ NULL,
    /* 2B  */ NULL,
    /* 2C ' ' */ " ",
    /* 2D '-' */ "-",
    /* 2E '=' */ "=",
    /* 2F '[' */ "\xE0\xA7\x87",  /* U+09C7 bengali vowel sign e */
    /* 30 ']' */ "\xE0\xA7\x8B",  /* U+09CB bengali vowel sign o */
    /* 31 '\\' */ "\xE2\x80\x8C",  /* U+200C zero width non-joiner */
    /* 32  */ NULL,
    /* 33 ';' */ ";",
    /* 34 "'" */ "'",
    /* 35 '`' */ "\xE2\x80\x8D",  /* U+200D zero width joiner */
    /* 36 ',' */ ",",
    /* 37 '.' */ "\xE0\xA5\xA4",  /* U+0964 devanagari danda */
    /* 38 '/' */ "\xE0\xA7\x8D",  /* U+09CD bengali sign virama */
};

static const char *const BN_PR_SHIFT[] = {
    /* 04 'A' */ "\xE0\xA6\x85",  /* U+0985 bengali letter a */
    /* 05 'B' */ "\xE0\xA6\xAD",  /* U+09AD bengali letter bha */
    /* 06 'C' */ "\xE0\xA6\x9B",  /* U+099B bengali letter cha */
    /* 07 'D' */ "\xE0\xA6\xA2",  /* U+09A2 bengali letter ddha */
    /* 08 'E' */ "\xE0\xA6\x88",  /* U+0988 bengali letter ii */
    /* 09 'F' */ "\xE0\xA6\xA5",  /* U+09A5 bengali letter tha */
    /* 0A 'G' */ "\xE0\xA6\x98",  /* U+0998 bengali letter gha */
    /* 0B 'H' */ "\xE0\xA6\x83",  /* U+0983 bengali sign visarga */
    /* 0C 'I' */ "\xE0\xA6\x87",  /* U+0987 bengali letter i */
    /* 0D 'J' */ "\xE0\xA6\x9D",  /* U+099D bengali letter jha */
    /* 0E 'K' */ "\xE0\xA6\x96",  /* U+0996 bengali letter kha */
    /* 0F 'L' */ "\xE0\xA6\x82",  /* U+0982 bengali sign anusvara */
    /* 10 'M' */ "\xE0\xA6\x99",  /* U+0999 bengali letter nga */
    /* 11 'N' */ "\xE0\xA6\xA3",  /* U+09A3 bengali letter nna */
    /* 12 'O' */ "\xE0\xA6\x94",  /* U+0994 bengali letter au */
    /* 13 'P' */ "\xE0\xA6\xAB",  /* U+09AB bengali letter pha */
    /* 14 'Q' */ "\xE0\xA6\xA7",  /* U+09A7 bengali letter dha */
    /* 15 'R' */ "\xE0\xA7\x9C",  /* U+09DC bengali letter rra */
    /* 16 'S' */ "\xE0\xA6\xB7",  /* U+09B7 bengali letter ssa */
    /* 17 'T' */ "\xE0\xA6\xA0",  /* U+09A0 bengali letter ttha */
    /* 18 'U' */ "\xE0\xA6\x89",  /* U+0989 bengali letter u */
    /* 19 'V' */ "\xE0\xA6\x8B",  /* U+098B bengali letter vocalic r */
    /* 1A 'W' */ "\xE0\xA6\x8A",  /* U+098A bengali letter uu */
    /* 1B 'X' */ "\xE0\xA7\x9D",  /* U+09DD bengali letter rha */
    /* 1C 'Y' */ "\xE0\xA6\x90",  /* U+0990 bengali letter ai */
    /* 1D 'Z' */ "\xE0\xA6\xAF",  /* U+09AF bengali letter ya */
    /* 1E '!' */ "!",
    /* 1F '@' */ "@",
    /* 20 '#' */ "#",
    /* 21 '$' */ "\xE0\xA7\xB3",  /* U+09F3 bengali rupee sign */
    /* 22 '%' */ "%",
    /* 23 '^' */ "^",
    /* 24 '&' */ "\xE0\xA6\x9E",  /* U+099E bengali letter nya */
    /* 25 '*' */ "\xE0\xA7\x8E",  /* U+09CE bengali letter khanda ta */
    /* 26 '(' */ "(",
    /* 27 ')' */ ")",
    /* 28  */ NULL,
    /* 29  */ NULL,
    /* 2A  */ NULL,
    /* 2B  */ NULL,
    /* 2C ' ' */ " ",
    /* 2D '_' */ "_",
    /* 2E '+' */ "+",
    /* 2F '{' */ "\xE0\xA7\x88",  /* U+09C8 bengali vowel sign ai */
    /* 30 '}' */ "\xE0\xA7\x8C",  /* U+09CC bengali vowel sign au */
    /* 31 '|' */ "\xE0\xA5\xA5",  /* U+0965 devanagari double danda */
    /* 32  */ NULL,
    /* 33 ':' */ ":",
    /* 34 '"' */ "\"",
    /* 35 '~' */ "~",
    /* 36 '<' */ "\xE0\xA7\x83",  /* U+09C3 bengali vowel sign vocalic r */
    /* 37 '>' */ "\xE0\xA6\x81",  /* U+0981 bengali sign candrabindu */
    /* 38 '?' */ "?",
};

static const key_entry_t BN_PR_EXTRA[] = {
    { 0x1E, L_ALTGR, "\xE0\xA7\xB4" },  /* U+09F4 bengali currency numerator one */
    { 0x1F, L_ALTGR, "\xE0\xA7\xB5" },  /* U+09F5 bengali currency numerator two */
    { 0x20, L_ALTGR, "\xE0\xA7\xB6" },  /* U+09F6 bengali currency numerator three */
    { 0x21, L_ALTGR_SHIFT, "\xE0\xA7\xB2" },  /* U+09F2 bengali rupee mark */
    { 0x21, L_ALTGR, "\xE0\xA7\xB7" },  /* U+09F7 bengali currency numerator four */
    { 0x24, L_ALTGR, "\xE0\xA7\xBA" },  /* U+09FA bengali isshar */
    { 0x27, L_ALTGR_SHIFT, "\xE0\xA7\xB9" },  /* U+09F9 bengali currency denominator sixteen */
    { 0x27, L_ALTGR, "\xE0\xA7\xB8" },  /* U+09F8 bengali currency numerator one less than the denominator */
    { 0x15, L_ALTGR, "\xE2\x82\xB9" },  /* U+20B9 indian rupee sign */
    { 0x30, L_ALTGR, "\xE0\xA7\x97" },  /* U+09D7 bengali au length mark */
    { 0x04, L_ALTGR_SHIFT, "\xE0\xA7\xA0" },  /* U+09E0 bengali letter vocalic rr */
    { 0x04, L_ALTGR, "\xE0\xA6\x8C" },  /* U+098C bengali letter vocalic l */
    { 0x16, L_ALTGR_SHIFT, "\xE0\xA7\xA3" },  /* U+09E3 bengali vowel sign vocalic ll */
    { 0x16, L_ALTGR, "\xE0\xA7\xA1" },  /* U+09E1 bengali letter vocalic ll */
    { 0x07, L_ALTGR_SHIFT, "\xE0\xA7\xA2" },  /* U+09E2 bengali vowel sign vocalic l */
    { 0x07, L_ALTGR, "\xE0\xA7\x84" },  /* U+09C4 bengali vowel sign vocalic rr */
    { 0x0B, L_ALTGR, "\xE0\xA6\xBD" },  /* U+09BD bengali sign avagraha */
    { 0x37, L_ALTGR, "\xE0\xA6\xBC" },  /* U+09BC bengali sign nukta */
    { 0, 0, NULL },
};
#endif

#ifdef CONFIG_KB_LAYOUT_ENABLE_BN_NJ
static const char *const BN_NJ_NORMAL[] = {
    /* 04 'a' */ "\xE0\xA7\x83",  /* U+09C3 bengali vowel sign vocalic r */
    /* 05 'b' */ "\xE0\xA6\xA8",  /* U+09A8 bengali letter na */
    /* 06 'c' */ "\xE0\xA7\x87",  /* U+09C7 bengali vowel sign e */
    /* 07 'd' */ "\xE0\xA6\xBF",  /* U+09BF bengali vowel sign i */
    /* 08 'e' */ "\xE0\xA6\xA1",  /* U+09A1 bengali letter dda */
    /* 09 'f' */ "\xE0\xA6\xAC",  /* U+09AC bengali letter ba */
    /* 0A 'g' */ "\xE0\xA7\x8D",  /* U+09CD bengali sign virama */
    /* 0B 'h' */ "\xE0\xA6\xBE",  /* U+09BE bengali vowel sign aa */
    /* 0C 'i' */ "\xE0\xA6\xB9",  /* U+09B9 bengali letter ha */
    /* 0D 'j' */ "\xE0\xA6\x95",  /* U+0995 bengali letter ka */
    /* 0E 'k' */ "\xE0\xA6\xA4",  /* U+09A4 bengali letter ta */
    /* 0F 'l' */ "\xE0\xA6\xA6",  /* U+09A6 bengali letter da */
    /* 10 'm' */ "\xE0\xA6\xAE",  /* U+09AE bengali letter ma */
    /* 11 'n' */ "\xE0\xA6\xB8",  /* U+09B8 bengali letter sa */
    /* 12 'o' */ "\xE0\xA6\x97",  /* U+0997 bengali letter ga */
    /* 13 'p' */ "\xE0\xA7\x9C",  /* U+09DC bengali letter rra */
    /* 14 'q' */ "\xE0\xA6\x99",  /* U+0999 bengali letter nga */
    /* 15 'r' */ "\xE0\xA6\xAA",  /* U+09AA bengali letter pa */
    /* 16 's' */ "\xE0\xA7\x81",  /* U+09C1 bengali vowel sign u */
    /* 17 't' */ "\xE0\xA6\x9F",  /* U+099F bengali letter tta */
    /* 18 'u' */ "\xE0\xA6\x9C",  /* U+099C bengali letter ja */
    /* 19 'v' */ "\xE0\xA6\xB0",  /* U+09B0 bengali letter ra */
    /* 1A 'w' */ "\xE0\xA6\xAF",  /* U+09AF bengali letter ya */
    /* 1B 'x' */ "\xE0\xA7\x8B",  /* U+09CB bengali vowel sign o */
    /* 1C 'y' */ "\xE0\xA6\x9A",  /* U+099A bengali letter ca */
    /* 1D 'z' */ "\xE0\xA6\x81",  /* U+0981 bengali sign candrabindu */
    /* 1E '1' */ "\xE0\xA7\xA7",  /* U+09E7 bengali digit one */
    /* 1F '2' */ "\xE0\xA7\xA8",  /* U+09E8 bengali digit two */
    /* 20 '3' */ "\xE0\xA7\xA9",  /* U+09E9 bengali digit three */
    /* 21 '4' */ "\xE0\xA7\xAA",  /* U+09EA bengali digit four */
    /* 22 '5' */ "\xE0\xA7\xAB",  /* U+09EB bengali digit five */
    /* 23 '6' */ "\xE0\xA7\xAC",  /* U+09EC bengali digit six */
    /* 24 '7' */ "\xE0\xA7\xAD",  /* U+09ED bengali digit seven */
    /* 25 '8' */ "\xE0\xA7\xAE",  /* U+09EE bengali digit eight */
    /* 26 '9' */ "\xE0\xA7\xAF",  /* U+09EF bengali digit nine */
    /* 27 '0' */ "\xE0\xA7\xA6",  /* U+09E6 bengali digit zero */
    /* 28  */ NULL,
    /* 29  */ NULL,
    /* 2A  */ NULL,
    /* 2B  */ NULL,
    /* 2C ' ' */ " ",
    /* 2D '-' */ "-",
    /* 2E '=' */ "=",
    /* 2F '[' */ "[",
    /* 30 ']' */ "]",
    /* 31 '\\' */ "\\",
    /* 32  */ NULL,
    /* 33 ';' */ ";",
    /* 34 "'" */ "'",
    /* 35 '`' */ "\xE2\x80\x8C",  /* U+200C zero width non-joiner */
    /* 36 ',' */ ",",
    /* 37 '.' */ ".",
    /* 38 '/' */ "/",
};

static const char *const BN_NJ_SHIFT[] = {
    /* 04 'A' */ "\xE0\xA7\x97",  /* U+09D7 bengali au length mark */
    /* 05 'B' */ "\xE0\xA6\xA3",  /* U+09A3 bengali letter nna */
    /* 06 'C' */ "\xE0\xA7\x88",  /* U+09C8 bengali vowel sign ai */
    /* 07 'D' */ "\xE0\xA7\x80",  /* U+09C0 bengali vowel sign ii */
    /* 08 'E' */ "\xE0\xA6\xA2",  /* U+09A2 bengali letter ddha */
    /* 09 'F' */ "\xE0\xA6\xAD",  /* U+09AD bengali letter bha */
    /* 0A 'G' */ "\xE0\xA5\xA4",  /* U+0964 devanagari danda */
    /* 0B 'H' */ "\xE0\xA6\x85",  /* U+0985 bengali letter a */
    /* 0C 'I' */ "\xE0\xA6\x9E",  /* U+099E bengali letter nya */
    /* 0D 'J' */ "\xE0\xA6\x96",  /* U+0996 bengali letter kha */
    /* 0E 'K' */ "\xE0\xA6\xA5",  /* U+09A5 bengali letter tha */
    /* 0F 'L' */ "\xE0\xA6\xA7",  /* U+09A7 bengali letter dha */
    /* 10 'M' */ "\xE0\xA6\xB6",  /* U+09B6 bengali letter sha */
    /* 11 'N' */ "\xE0\xA6\xB7",  /* U+09B7 bengali letter ssa */
    /* 12 'O' */ "\xE0\xA6\x98",  /* U+0998 bengali letter gha */
    /* 13 'P' */ "\xE0\xA7\x9D",  /* U+09DD bengali letter rha */
    /* 14 'Q' */ "\xE0\xA6\x82",  /* U+0982 bengali sign anusvara */
    /* 15 'R' */ "\xE0\xA6\xAB",  /* U+09AB bengali letter pha */
    /* 16 'S' */ "\xE0\xA7\x82",  /* U+09C2 bengali vowel sign uu */
    /* 17 'T' */ "\xE0\xA6\xA0",  /* U+09A0 bengali letter ttha */
    /* 18 'U' */ "\xE0\xA6\x9D",  /* U+099D bengali letter jha */
    /* 19 'V' */ "\xE0\xA6\xB2",  /* U+09B2 bengali letter la */
    /* 1A 'W' */ "\xE0\xA7\x9F",  /* U+09DF bengali letter yya */
    /* 1B 'X' */ "\xE0\xA7\x8C",  /* U+09CC bengali vowel sign au */
    /* 1C 'Y' */ "\xE0\xA6\x9B",  /* U+099B bengali letter cha */
    /* 1D 'Z' */ "\xE0\xA6\x83",  /* U+0983 bengali sign visarga */
    /* 1E '!' */ "!",
    /* 1F '@' */ "@",
    /* 20 '#' */ "#",
    /* 21 '$' */ "$",
    /* 22 '%' */ "%",
    /* 23 '^' */ "^",
    /* 24 '&' */ "&",
    /* 25 '*' */ "*",
    /* 26 '(' */ "(",
    /* 27 ')' */ ")",
    /* 28  */ NULL,
    /* 29  */ NULL,
    /* 2A  */ NULL,
    /* 2B  */ NULL,
    /* 2C ' ' */ " ",
    /* 2D '_' */ "_",
    /* 2E '+' */ "+",
    /* 2F '{' */ "{",
    /* 30 '}' */ "}",
    /* 31 '|' */ "|",
    /* 32  */ NULL,
    /* 33 ':' */ ":",
    /* 34 '"' */ "\"",
    /* 35 '~' */ "\xE2\x80\x8D",  /* U+200D zero width joiner */
    /* 36 '<' */ "<",
    /* 37 '>' */ ">",
    /* 38 '?' */ "?",
};

static const key_entry_t BN_NJ_EXTRA[] = {
    { 0x0B, L_ALTGR, "\xE0\xA6\x86" },  /* U+0986 bengali letter aa */
    { 0x07, L_ALTGR, "\xE0\xA6\x87" },  /* U+0987 bengali letter i */
    { 0x07, L_ALTGR_SHIFT, "\xE0\xA6\x88" },  /* U+0988 bengali letter ii */
    { 0x16, L_ALTGR, "\xE0\xA6\x89" },  /* U+0989 bengali letter u */
    { 0x16, L_ALTGR_SHIFT, "\xE0\xA6\x8A" },  /* U+098A bengali letter uu */
    { 0x04, L_ALTGR, "\xE0\xA6\x8B" },  /* U+098B bengali letter vocalic r */
    { 0x0F, L_ALTGR, "\xE0\xA6\x8C" },  /* U+098C bengali letter vocalic l */
    { 0x06, L_ALTGR, "\xE0\xA6\x8F" },  /* U+098F bengali letter e */
    { 0x06, L_ALTGR_SHIFT, "\xE0\xA6\x90" },  /* U+0990 bengali letter ai */
    { 0x1B, L_ALTGR, "\xE0\xA6\x93" },  /* U+0993 bengali letter o */
    { 0x1B, L_ALTGR_SHIFT, "\xE0\xA6\x94" },  /* U+0994 bengali letter au */
    { 0x04, L_ALTGR_SHIFT, "\xE0\xA7\xA0" },  /* U+09E0 bengali letter vocalic rr */
    { 0x0F, L_ALTGR_SHIFT, "\xE0\xA7\xA1" },  /* U+09E1 bengali letter vocalic ll */
    { 0x08, L_ALTGR, "\xE0\xA7\x84" },  /* U+09C4 bengali vowel sign vocalic rr */
    { 0x14, L_ALTGR, "\xE0\xA7\xA2" },  /* U+09E2 bengali vowel sign vocalic l */
    { 0x14, L_ALTGR_SHIFT, "\xE0\xA7\xA3" },  /* U+09E3 bengali vowel sign vocalic ll */
    { 0x23, L_ALTGR_SHIFT, "\xE0\xA7\x8E" },  /* U+09CE bengali letter khanda ta */
    { 0x37, L_ALTGR, "\xE0\xA6\xBC" },  /* U+09BC bengali sign nukta */
    { 0x0C, L_ALTGR, "\xE0\xA6\xBD" },  /* U+09BD bengali sign avagraha */
    { 0x1D, L_ALTGR, "\xE0\xA7\xBA" },  /* U+09FA bengali isshar */
    { 0x09, L_ALTGR, "\xE0\xA7\xB0" },  /* U+09F0 bengali letter ra with middle diagonal */
    { 0x09, L_ALTGR_SHIFT, "\xE0\xA7\xB1" },  /* U+09F1 bengali letter ra with lower diagonal */
    { 0x21, L_ALTGR_SHIFT, "\xE0\xA7\xB2" },  /* U+09F2 bengali rupee mark */
    { 0x21, L_ALTGR, "\xE0\xA7\xB3" },  /* U+09F3 bengali rupee sign */
    { 0x1E, L_ALTGR, "\xE0\xA7\xB4" },  /* U+09F4 bengali currency numerator one */
    { 0x1F, L_ALTGR, "\xE0\xA7\xB5" },  /* U+09F5 bengali currency numerator two */
    { 0x20, L_ALTGR, "\xE0\xA7\xB6" },  /* U+09F6 bengali currency numerator three */
    { 0x22, L_ALTGR, "\xE0\xA7\xB7" },  /* U+09F7 bengali currency numerator four */
    { 0x23, L_ALTGR, "\xE0\xA7\xB8" },  /* U+09F8 bengali currency numerator one less than the denominator */
    { 0x27, L_ALTGR, "\xE0\xA7\xB9" },  /* U+09F9 bengali currency denominator sixteen */
    { 0x0A, L_ALTGR, "\xE0\xA5\xA5" },  /* U+0965 devanagari double danda */
    { 0x0E, L_ALTGR, "\xE0\xA7\x8E" },  /* U+09CE bengali letter khanda ta */
    { 0x2D, L_ALTGR, "\xE2\x80\x8C" },  /* U+200C zero width non-joiner */
    { 0x35, L_ALTGR_SHIFT, "\xE2\x80\x8D" },  /* U+200D zero width joiner */
    { 0x35, L_ALTGR, "\xE2\x80\x8C" },  /* U+200C zero width non-joiner */
    { 0x2E, L_ALTGR, "\xE2\x80\x8D" },  /* U+200D zero width joiner */
    { 0, 0, NULL },
};
#endif
#endif /* Bengali */

#if defined(CONFIG_KB_LAYOUT_ENABLE_TE_IN) || defined(CONFIG_KB_LAYOUT_ENABLE_TE_SA)
/* ------------------------------------------------------------------ */
/* Telugu: InScript and Sarala                                        */
/* ------------------------------------------------------------------ */

/* InScript: generated from the m17n-db input method "te-inscript2"
 * (Enhanced InScript; keys typed on a US keyboard -> output, G- =
 * AltGr), with the keys m17n leaves empty taken from XKB "in(tel)"
 * (marked). Sarala: generated from XKB "in(tel-sarala)" (four levels:
 * normal, Shift, AltGr, AltGr + Shift), plus the zero width joiners of
 * m17n "te-sarala" on AltGr + - / AltGr + =. Text is typed in Unicode
 * order (consonant, then vowel sign); HarfBuzz shapes it for display.
 * Keys without an entry keep their US character, except letters,
 * which type nothing. */

#ifdef CONFIG_KB_LAYOUT_ENABLE_TE_IN
static const char *const TE_IN_NORMAL[] = {
    /* 04 'a' */ "\xE0\xB1\x8B",  /* U+0C4B telugu vowel sign oo */
    /* 05 'b' */ "\xE0\xB0\xB5",  /* U+0C35 telugu letter va */
    /* 06 'c' */ "\xE0\xB0\xAE",  /* U+0C2E telugu letter ma */
    /* 07 'd' */ "\xE0\xB1\x8D",  /* U+0C4D telugu sign virama */
    /* 08 'e' */ "\xE0\xB0\xBE",  /* U+0C3E telugu vowel sign aa */
    /* 09 'f' */ "\xE0\xB0\xBF",  /* U+0C3F telugu vowel sign i */
    /* 0A 'g' */ "\xE0\xB1\x81",  /* U+0C41 telugu vowel sign u */
    /* 0B 'h' */ "\xE0\xB0\xAA",  /* U+0C2A telugu letter pa */
    /* 0C 'i' */ "\xE0\xB0\x97",  /* U+0C17 telugu letter ga */
    /* 0D 'j' */ "\xE0\xB0\xB0",  /* U+0C30 telugu letter ra */
    /* 0E 'k' */ "\xE0\xB0\x95",  /* U+0C15 telugu letter ka */
    /* 0F 'l' */ "\xE0\xB0\xA4",  /* U+0C24 telugu letter ta */
    /* 10 'm' */ "\xE0\xB0\xB8",  /* U+0C38 telugu letter sa */
    /* 11 'n' */ "\xE0\xB0\xB2",  /* U+0C32 telugu letter la */
    /* 12 'o' */ "\xE0\xB0\xA6",  /* U+0C26 telugu letter da */
    /* 13 'p' */ "\xE0\xB0\x9C",  /* U+0C1C telugu letter ja */
    /* 14 'q' */ "\xE0\xB1\x8C",  /* U+0C4C telugu vowel sign au */
    /* 15 'r' */ "\xE0\xB1\x80",  /* U+0C40 telugu vowel sign ii */
    /* 16 's' */ "\xE0\xB1\x87",  /* U+0C47 telugu vowel sign ee */
    /* 17 't' */ "\xE0\xB1\x82",  /* U+0C42 telugu vowel sign uu */
    /* 18 'u' */ "\xE0\xB0\xB9",  /* U+0C39 telugu letter ha */
    /* 19 'v' */ "\xE0\xB0\xA8",  /* U+0C28 telugu letter na */
    /* 1A 'w' */ "\xE0\xB1\x88",  /* U+0C48 telugu vowel sign ai */
    /* 1B 'x' */ "\xE0\xB0\x82",  /* U+0C02 telugu sign anusvara */
    /* 1C 'y' */ "\xE0\xB0\xAC",  /* U+0C2C telugu letter ba */
    /* 1D 'z' */ "\xE0\xB1\x86",  /* U+0C46 telugu vowel sign e */
    /* 1E '1' */ "\xE0\xB1\xA7",  /* U+0C67 telugu digit one */
    /* 1F '2' */ "\xE0\xB1\xA8",  /* U+0C68 telugu digit two */
    /* 20 '3' */ "\xE0\xB1\xA9",  /* U+0C69 telugu digit three */
    /* 21 '4' */ "\xE0\xB1\xAA",  /* U+0C6A telugu digit four */
    /* 22 '5' */ "\xE0\xB1\xAB",  /* U+0C6B telugu digit five */
    /* 23 '6' */ "\xE0\xB1\xAC",  /* U+0C6C telugu digit six */
    /* 24 '7' */ "\xE0\xB1\xAD",  /* U+0C6D telugu digit seven */
    /* 25 '8' */ "\xE0\xB1\xAE",  /* U+0C6E telugu digit eight */
    /* 26 '9' */ "\xE0\xB1\xAF",  /* U+0C6F telugu digit nine */
    /* 27 '0' */ "\xE0\xB1\xA6",  /* U+0C66 telugu digit zero */
    /* 28  */ NULL,
    /* 29  */ NULL,
    /* 2A  */ NULL,
    /* 2B  */ NULL,
    /* 2C ' ' */ " ",
    /* 2D '-' */ "-",
    /* 2E '=' */ "\xE0\xB1\x83",  /* U+0C43 telugu vowel sign vocalic r */
    /* 2F '[' */ "\xE0\xB0\xA1",  /* U+0C21 telugu letter dda */
    /* 30 ']' */ "\xE0\xB0\x9E",  /* U+0C1E telugu letter nya (XKB in(tel)) */
    /* 31 '\\' */ "\\",
    /* 32  */ NULL,
    /* 33 ';' */ "\xE0\xB0\x9A",  /* U+0C1A telugu letter ca */
    /* 34 "'" */ "\xE0\xB0\x9F",  /* U+0C1F telugu letter tta */
    /* 35 '`' */ "\xE0\xB1\x8A",  /* U+0C4A telugu vowel sign o */
    /* 36 ',' */ ",",
    /* 37 '.' */ ".",
    /* 38 '/' */ "\xE0\xB0\xAF",  /* U+0C2F telugu letter ya */
};

static const char *const TE_IN_SHIFT[] = {
    /* 04 'A' */ "\xE0\xB0\x93",  /* U+0C13 telugu letter oo */
    /* 05 'B' */ NULL,
    /* 06 'C' */ "\xE0\xB0\xA3",  /* U+0C23 telugu letter nna */
    /* 07 'D' */ "\xE0\xB0\x85",  /* U+0C05 telugu letter a */
    /* 08 'E' */ "\xE0\xB0\x86",  /* U+0C06 telugu letter aa */
    /* 09 'F' */ "\xE0\xB0\x87",  /* U+0C07 telugu letter i */
    /* 0A 'G' */ "\xE0\xB0\x89",  /* U+0C09 telugu letter u */
    /* 0B 'H' */ "\xE0\xB0\xAB",  /* U+0C2B telugu letter pha */
    /* 0C 'I' */ "\xE0\xB0\x98",  /* U+0C18 telugu letter gha */
    /* 0D 'J' */ "\xE0\xB0\xB1",  /* U+0C31 telugu letter rra */
    /* 0E 'K' */ "\xE0\xB0\x96",  /* U+0C16 telugu letter kha */
    /* 0F 'L' */ "\xE0\xB0\xA5",  /* U+0C25 telugu letter tha */
    /* 10 'M' */ "\xE0\xB0\xB6",  /* U+0C36 telugu letter sha */
    /* 11 'N' */ "\xE0\xB0\xB3",  /* U+0C33 telugu letter lla */
    /* 12 'O' */ "\xE0\xB0\xA7",  /* U+0C27 telugu letter dha */
    /* 13 'P' */ "\xE0\xB0\x9D",  /* U+0C1D telugu letter jha */
    /* 14 'Q' */ "\xE0\xB0\x94",  /* U+0C14 telugu letter au */
    /* 15 'R' */ "\xE0\xB0\x88",  /* U+0C08 telugu letter ii */
    /* 16 'S' */ "\xE0\xB0\x8F",  /* U+0C0F telugu letter ee */
    /* 17 'T' */ "\xE0\xB0\x8A",  /* U+0C0A telugu letter uu */
    /* 18 'U' */ "\xE0\xB0\x99",  /* U+0C19 telugu letter nga */
    /* 19 'V' */ NULL,
    /* 1A 'W' */ "\xE0\xB0\x90",  /* U+0C10 telugu letter ai */
    /* 1B 'X' */ "\xE0\xB0\x81",  /* U+0C01 telugu sign candrabindu */
    /* 1C 'Y' */ "\xE0\xB0\xAD",  /* U+0C2D telugu letter bha */
    /* 1D 'Z' */ "\xE0\xB0\x8E",  /* U+0C0E telugu letter e */
    /* 1E '!' */ "!",
    /* 1F '@' */ "@",
    /* 20 '#' */ "\xE0\xB1\x8D\xE0\xB0\xB0",  /* U+0C4D telugu sign virama U+0C30 telugu letter ra */
    /* 21 '$' */ "$",
    /* 22 '%' */ "\xE0\xB0\x9C\xE0\xB1\x8D\xE0\xB0\x9E",  /* U+0C1C telugu letter ja U+0C4D telugu sign virama U+0C1E telugu letter nya */
    /* 23 '^' */ "\xE0\xB0\xA4\xE0\xB1\x8D\xE0\xB0\xB0",  /* U+0C24 telugu letter ta U+0C4D telugu sign virama U+0C30 telugu letter ra */
    /* 24 '&' */ "\xE0\xB0\x95\xE0\xB1\x8D\xE0\xB0\xB7",  /* U+0C15 telugu letter ka U+0C4D telugu sign virama U+0C37 telugu letter ssa */
    /* 25 '*' */ "\xE0\xB0\xB6\xE0\xB1\x8D\xE0\xB0\xB0",  /* U+0C36 telugu letter sha U+0C4D telugu sign virama U+0C30 telugu letter ra */
    /* 26 '(' */ "(",
    /* 27 ')' */ ")",
    /* 28  */ NULL,
    /* 29  */ NULL,
    /* 2A  */ NULL,
    /* 2B  */ NULL,
    /* 2C ' ' */ " ",
    /* 2D '_' */ "\xE0\xB0\x83",  /* U+0C03 telugu sign visarga */
    /* 2E '+' */ "\xE0\xB0\x8B",  /* U+0C0B telugu letter vocalic r */
    /* 2F '{' */ "\xE0\xB0\xA2",  /* U+0C22 telugu letter ddha */
    /* 30 '}' */ "\xE0\xB0\x9E",  /* U+0C1E telugu letter nya */
    /* 31 '|' */ "|",
    /* 32  */ NULL,
    /* 33 ':' */ "\xE0\xB0\x9B",  /* U+0C1B telugu letter cha */
    /* 34 '"' */ "\xE0\xB0\xA0",  /* U+0C20 telugu letter ttha */
    /* 35 '~' */ "\xE0\xB0\x92",  /* U+0C12 telugu letter o */
    /* 36 '<' */ "\xE0\xB0\xB7",  /* U+0C37 telugu letter ssa */
    /* 37 '>' */ "\xE0\xA5\xA4",  /* U+0964 devanagari danda */
    /* 38 '?' */ "?",
};

static const key_entry_t TE_IN_EXTRA[] = {
    { 0x1E, L_ALTGR_SHIFT, "\xE0\xB1\xB9" },  /* U+0C79 telugu fraction digit one for odd powers of four */
    { 0x1E, L_ALTGR, "\xE2\x80\x8D" },  /* U+200D zero width joiner */
    { 0x1F, L_ALTGR_SHIFT, "\xE0\xB1\xBC" },  /* U+0C7C telugu fraction digit one for even powers of four */
    { 0x1F, L_ALTGR, "\xE2\x80\x8C" },  /* U+200C zero width non-joiner */
    { 0x20, L_ALTGR_SHIFT, "\xE0\xB1\xBA" },  /* U+0C7A telugu fraction digit two for odd powers of four */
    { 0x21, L_ALTGR_SHIFT, "\xE0\xB1\xBD" },  /* U+0C7D telugu fraction digit two for even powers of four */
    { 0x21, L_ALTGR, "\xE2\x82\xB9" },  /* U+20B9 indian rupee sign */
    { 0x22, L_ALTGR_SHIFT, "\xE0\xB1\xBB" },  /* U+0C7B telugu fraction digit three for odd powers of four */
    { 0x23, L_ALTGR_SHIFT, "\xE0\xB1\xBE" },  /* U+0C7E telugu fraction digit three for even powers of four */
    { 0x27, L_ALTGR_SHIFT, "\xE0\xB1\xB8" },  /* U+0C78 telugu fraction digit zero for odd powers of four */
    { 0x2D, L_ALTGR, "\xE0\xB1\xBF" },  /* U+0C7F telugu sign tuumu */
    { 0x2E, L_ALTGR_SHIFT, "\xE0\xB1\xA0" },  /* U+0C60 telugu letter vocalic rr */
    { 0x2E, L_ALTGR, "\xE0\xB1\x84" },  /* U+0C44 telugu vowel sign vocalic rr */
    { 0x15, L_ALTGR_SHIFT, "\xE0\xB1\xA3" },  /* U+0C63 telugu vowel sign vocalic ll */
    { 0x15, L_ALTGR, "\xE0\xB1\xA1" },  /* U+0C61 telugu letter vocalic ll */
    { 0x13, L_ALTGR, "\xE0\xB1\x99" },  /* U+0C59 telugu letter dza */
    { 0x09, L_ALTGR_SHIFT, "\xE0\xB0\x8C" },  /* U+0C0C telugu letter vocalic l */
    { 0x09, L_ALTGR, "\xE0\xB1\xA2" },  /* U+0C62 telugu vowel sign vocalic l */
    { 0x33, L_ALTGR, "\xE0\xB1\x98" },  /* U+0C58 telugu letter tsa */
    { 0x1D, L_ALTGR_SHIFT, "\xE0\xB1\x96" },  /* U+0C56 telugu ai length mark */
    { 0x1D, L_ALTGR, "\xE0\xB1\x95" },  /* U+0C55 telugu length mark */
    { 0x37, L_ALTGR_SHIFT, "\xE0\xB0\xBD" },  /* U+0C3D telugu sign avagraha */
    { 0x37, L_ALTGR, "\xE0\xA5\xA5" },  /* U+0965 devanagari double danda */
    { 0, 0, NULL },
};
#endif

#ifdef CONFIG_KB_LAYOUT_ENABLE_TE_SA
static const char *const TE_SA_NORMAL[] = {
    /* 04 'a' */ "\xE0\xB0\xAF",  /* U+0C2F telugu letter ya */
    /* 05 'b' */ "\xE0\xB0\xAC",  /* U+0C2C telugu letter ba */
    /* 06 'c' */ "\xE0\xB0\xA1",  /* U+0C21 telugu letter dda */
    /* 07 'd' */ "\xE0\xB0\xA6",  /* U+0C26 telugu letter da */
    /* 08 'e' */ "\xE0\xB1\x87",  /* U+0C47 telugu vowel sign ee */
    /* 09 'f' */ "\xE0\xB1\x8D",  /* U+0C4D telugu sign virama */
    /* 0A 'g' */ "\xE0\xB0\x97",  /* U+0C17 telugu letter ga */
    /* 0B 'h' */ "\xE0\xB0\x9A",  /* U+0C1A telugu letter ca */
    /* 0C 'i' */ "\xE0\xB0\xBF",  /* U+0C3F telugu vowel sign i */
    /* 0D 'j' */ "\xE0\xB0\xBE",  /* U+0C3E telugu vowel sign aa */
    /* 0E 'k' */ "\xE0\xB0\x95",  /* U+0C15 telugu letter ka */
    /* 0F 'l' */ "\xE0\xB0\xB2",  /* U+0C32 telugu letter la */
    /* 10 'm' */ "\xE0\xB0\xAE",  /* U+0C2E telugu letter ma */
    /* 11 'n' */ "\xE0\xB0\xA8",  /* U+0C28 telugu letter na */
    /* 12 'o' */ "\xE0\xB1\x8B",  /* U+0C4B telugu vowel sign oo */
    /* 13 'p' */ "\xE0\xB0\xAA",  /* U+0C2A telugu letter pa */
    /* 14 'q' */ "\xE0\xB1\x86",  /* U+0C46 telugu vowel sign e */
    /* 15 'r' */ "\xE0\xB0\xB0",  /* U+0C30 telugu letter ra */
    /* 16 's' */ "\xE0\xB0\x82",  /* U+0C02 telugu sign anusvara */
    /* 17 't' */ "\xE0\xB0\xA4",  /* U+0C24 telugu letter ta */
    /* 18 'u' */ "\xE0\xB1\x81",  /* U+0C41 telugu vowel sign u */
    /* 19 'v' */ "\xE0\xB0\xB5",  /* U+0C35 telugu letter va */
    /* 1A 'w' */ "\xE0\xB0\xB8",  /* U+0C38 telugu letter sa */
    /* 1B 'x' */ "\xE0\xB1\x82",  /* U+0C42 telugu vowel sign uu */
    /* 1C 'y' */ "\xE0\xB1\x80",  /* U+0C40 telugu vowel sign ii */
    /* 1D 'z' */ "\xE0\xB1\x8A",  /* U+0C4A telugu vowel sign o */
    /* 1E '1' */ "1",
    /* 1F '2' */ "2",
    /* 20 '3' */ "3",
    /* 21 '4' */ "4",
    /* 22 '5' */ "5",
    /* 23 '6' */ "6",
    /* 24 '7' */ "7",
    /* 25 '8' */ "8",
    /* 26 '9' */ "9",
    /* 27 '0' */ "0",
    /* 28  */ NULL,
    /* 29  */ NULL,
    /* 2A  */ NULL,
    /* 2B  */ NULL,
    /* 2C ' ' */ " ",
    /* 2D '-' */ "-",
    /* 2E '=' */ "=",
    /* 2F '[' */ "\xE0\xB0\x9C",  /* U+0C1C telugu letter ja */
    /* 30 ']' */ "\xE0\xB1\x88",  /* U+0C48 telugu vowel sign ai */
    /* 31 '\\' */ "\xE0\xB1\x8C",  /* U+0C4C telugu vowel sign au */
    /* 32  */ NULL,
    /* 33 ';' */ "\xE0\xB0\x9F",  /* U+0C1F telugu letter tta */
    /* 34 "'" */ "'",
    /* 35 '`' */ "\xE0\xB1\x83",  /* U+0C43 telugu vowel sign vocalic r */
    /* 36 ',' */ ",",
    /* 37 '.' */ ".",
    /* 38 '/' */ "\xE0\xB0\xB6",  /* U+0C36 telugu letter sha */
};

static const char *const TE_SA_SHIFT[] = {
    /* 04 'A' */ "\xE0\xB0\xBD",  /* U+0C3D telugu sign avagraha */
    /* 05 'B' */ "\xE0\xB0\xAD",  /* U+0C2D telugu letter bha */
    /* 06 'C' */ "\xE0\xB0\xA2",  /* U+0C22 telugu letter ddha */
    /* 07 'D' */ "\xE0\xB0\xA7",  /* U+0C27 telugu letter dha */
    /* 08 'E' */ "\xE0\xB0\x8F",  /* U+0C0F telugu letter ee */
    /* 09 'F' */ "\xE0\xB0\x85",  /* U+0C05 telugu letter a */
    /* 0A 'G' */ "\xE0\xB0\x98",  /* U+0C18 telugu letter gha */
    /* 0B 'H' */ "\xE0\xB0\x9B",  /* U+0C1B telugu letter cha */
    /* 0C 'I' */ "\xE0\xB0\x87",  /* U+0C07 telugu letter i */
    /* 0D 'J' */ "\xE0\xB0\x86",  /* U+0C06 telugu letter aa */
    /* 0E 'K' */ "\xE0\xB0\x96",  /* U+0C16 telugu letter kha */
    /* 0F 'L' */ "\xE0\xB0\xB3",  /* U+0C33 telugu letter lla */
    /* 10 'M' */ "\xE0\xB0\x81",  /* U+0C01 telugu sign candrabindu */
    /* 11 'N' */ "\xE0\xB0\xA3",  /* U+0C23 telugu letter nna */
    /* 12 'O' */ "\xE0\xB0\x93",  /* U+0C13 telugu letter oo */
    /* 13 'P' */ "\xE0\xB0\xAB",  /* U+0C2B telugu letter pha */
    /* 14 'Q' */ "\xE0\xB0\x8E",  /* U+0C0E telugu letter e */
    /* 15 'R' */ "\xE0\xB0\xB1",  /* U+0C31 telugu letter rra */
    /* 16 'S' */ "\xE0\xB0\x83",  /* U+0C03 telugu sign visarga */
    /* 17 'T' */ "\xE0\xB0\xA5",  /* U+0C25 telugu letter tha */
    /* 18 'U' */ "\xE0\xB0\x89",  /* U+0C09 telugu letter u */
    /* 19 'V' */ "\xE0\xB0\xB9",  /* U+0C39 telugu letter ha */
    /* 1A 'W' */ "\xE0\xB0\xB7",  /* U+0C37 telugu letter ssa */
    /* 1B 'X' */ "\xE0\xB0\x8A",  /* U+0C0A telugu letter uu */
    /* 1C 'Y' */ "\xE0\xB0\x88",  /* U+0C08 telugu letter ii */
    /* 1D 'Z' */ "\xE0\xB0\x92",  /* U+0C12 telugu letter o */
    /* 1E '!' */ "!",
    /* 1F '@' */ "@",
    /* 20 '#' */ "#",
    /* 21 '$' */ "$",
    /* 22 '%' */ "%",
    /* 23 '^' */ "^",
    /* 24 '&' */ "&",
    /* 25 '*' */ "*",
    /* 26 '(' */ "(",
    /* 27 ')' */ ")",
    /* 28  */ NULL,
    /* 29  */ NULL,
    /* 2A  */ NULL,
    /* 2B  */ NULL,
    /* 2C ' ' */ " ",
    /* 2D '_' */ "_",
    /* 2E '+' */ "+",
    /* 2F '{' */ "\xE0\xB0\x9D",  /* U+0C1D telugu letter jha */
    /* 30 '}' */ "\xE0\xB0\x90",  /* U+0C10 telugu letter ai */
    /* 31 '|' */ "\xE0\xB0\x94",  /* U+0C14 telugu letter au */
    /* 32  */ NULL,
    /* 33 ':' */ "\xE0\xB0\xA0",  /* U+0C20 telugu letter ttha */
    /* 34 '"' */ "\"",
    /* 35 '~' */ "\xE0\xB0\x8B",  /* U+0C0B telugu letter vocalic r */
    /* 36 '<' */ "\xE0\xB0\x9E",  /* U+0C1E telugu letter nya */
    /* 37 '>' */ "\xE0\xB0\x99",  /* U+0C19 telugu letter nga */
    /* 38 '?' */ "?",
};

static const key_entry_t TE_SA_EXTRA[] = {
    { 0x08, L_ALTGR, "\xE0\xB1\x84" },  /* U+0C44 telugu vowel sign vocalic rr */
    { 0x0B, L_ALTGR, "\xE0\xB1\x98" },  /* U+0C58 telugu letter tsa */
    { 0x0B, L_ALTGR_SHIFT, "\xE0\xB1\x99" },  /* U+0C59 telugu letter dza */
    { 0x0E, L_ALTGR, "\xE0\xB1\xA2" },  /* U+0C62 telugu vowel sign vocalic l */
    { 0x0E, L_ALTGR_SHIFT, "\xE0\xB1\xA3" },  /* U+0C63 telugu vowel sign vocalic ll */
    { 0x0F, L_ALTGR, "\xE0\xB0\x8C" },  /* U+0C0C telugu letter vocalic l */
    { 0x0F, L_ALTGR_SHIFT, "\xE0\xB1\xA1" },  /* U+0C61 telugu letter vocalic ll */
    { 0x14, L_ALTGR, "\xE0\xB1\x84" },  /* U+0C44 telugu vowel sign vocalic rr */
    { 0x14, L_ALTGR_SHIFT, "\xE0\xB0\xB4" },  /* U+0C34 telugu letter llla */
    { 0x15, L_ALTGR, "\xE0\xB1\x84" },  /* U+0C44 telugu vowel sign vocalic rr */
    { 0x15, L_ALTGR_SHIFT, "\xE0\xB1\xA0" },  /* U+0C60 telugu letter vocalic rr */
    { 0x1A, L_ALTGR, "\xE0\xB1\x84" },  /* U+0C44 telugu vowel sign vocalic rr */
    { 0x1E, L_ALTGR, "\xE0\xB1\xA7" },  /* U+0C67 telugu digit one */
    { 0x1E, L_ALTGR_SHIFT, "\xE0\xB1\xB8" },  /* U+0C78 telugu fraction digit zero for odd powers of four */
    { 0x1F, L_ALTGR, "\xE0\xB1\xA8" },  /* U+0C68 telugu digit two */
    { 0x1F, L_ALTGR_SHIFT, "\xE0\xB1\xB9" },  /* U+0C79 telugu fraction digit one for odd powers of four */
    { 0x20, L_ALTGR, "\xE0\xB1\xA9" },  /* U+0C69 telugu digit three */
    { 0x20, L_ALTGR_SHIFT, "\xE0\xB1\xBA" },  /* U+0C7A telugu fraction digit two for odd powers of four */
    { 0x21, L_ALTGR, "\xE0\xB1\xAA" },  /* U+0C6A telugu digit four */
    { 0x21, L_ALTGR_SHIFT, "\xE0\xB1\xBB" },  /* U+0C7B telugu fraction digit three for odd powers of four */
    { 0x22, L_ALTGR, "\xE0\xB1\xAB" },  /* U+0C6B telugu digit five */
    { 0x22, L_ALTGR_SHIFT, "\xE0\xB1\xBC" },  /* U+0C7C telugu fraction digit one for even powers of four */
    { 0x23, L_ALTGR, "\xE0\xB1\xAC" },  /* U+0C6C telugu digit six */
    { 0x23, L_ALTGR_SHIFT, "\xE0\xB1\xBD" },  /* U+0C7D telugu fraction digit two for even powers of four */
    { 0x24, L_ALTGR, "\xE0\xB1\xAD" },  /* U+0C6D telugu digit seven */
    { 0x24, L_ALTGR_SHIFT, "\xE0\xB1\xBE" },  /* U+0C7E telugu fraction digit three for even powers of four */
    { 0x25, L_ALTGR, "\xE0\xB1\xAE" },  /* U+0C6E telugu digit eight */
    { 0x25, L_ALTGR_SHIFT, "\xE0\xB1\xBF" },  /* U+0C7F telugu sign tuumu */
    { 0x26, L_ALTGR, "\xE0\xB1\xAF" },  /* U+0C6F telugu digit nine */
    { 0x26, L_ALTGR_SHIFT, "\xE2\x82\xB9" },  /* U+20B9 indian rupee sign */
    { 0x27, L_ALTGR, "\xE0\xB1\xA6" },  /* U+0C66 telugu digit zero */
    { 0x27, L_ALTGR_SHIFT, "\xE0\xB1\x95" },  /* U+0C55 telugu length mark */
    { 0x2D, L_ALTGR, "\xE2\x80\x8C" },  /* U+200C zero width non-joiner (m17n te-sarala) */
    { 0x2D, L_ALTGR_SHIFT, "\xE0\xB1\x96" },  /* U+0C56 telugu ai length mark */
    { 0x2E, L_ALTGR, "\xE2\x80\x8D" },  /* U+200D zero width joiner (m17n te-sarala) */
    { 0x2F, L_ALTGR, "[" },
    { 0x2F, L_ALTGR_SHIFT, "{" },
    { 0x30, L_ALTGR, "]" },
    { 0x30, L_ALTGR_SHIFT, "}" },
    { 0x31, L_ALTGR, "\xE0\xA5\xA4" },  /* U+0964 devanagari danda */
    { 0x31, L_ALTGR_SHIFT, "\xE0\xA5\xA5" },  /* U+0965 devanagari double danda */
    { 0x33, L_ALTGR, ";" },
    { 0x33, L_ALTGR_SHIFT, ":" },
    { 0x35, L_ALTGR, "`" },
    { 0x35, L_ALTGR_SHIFT, "~" },
    { 0x36, L_ALTGR, "<" },
    { 0x37, L_ALTGR, ">" },
    { 0x38, L_ALTGR, "/" },
    { 0, 0, NULL },
};
#endif
#endif /* Telugu */

#if defined(CONFIG_KB_LAYOUT_ENABLE_DE_CH) || defined(CONFIG_KB_LAYOUT_ENABLE_FR_CH)
/* ------------------------------------------------------------------ */
/* Swiss German and Swiss French (QWERTZ)                             */
/* ------------------------------------------------------------------ */
/* The two Swiss layouts share one keyboard and differ only in the
 * three accented-letter keys right of P and L: Swiss German types
 * u/o/a-umlaut unshifted and e-grave / e-acute / a-grave shifted,
 * Swiss French the other way round. The ISO key left of Enter is
 * sent as 0x32 by the HID spec but as 0x31 by many keyboards, so
 * both carry it. */

#define CH_SHIFT_DIGITS \
    "+","\x22","*",CH_c_CED,"%","&","/","(",")","="

/* AltGr layer and the ISO key, same on both */
#define CH_EXTRA_COMMON \
    { 0x08,   L_ALTGR,  CH_EURO },      /* e */ \
    { 0x1E,   L_ALTGR,  CH_BROKEN },    /* 1 */ \
    { 0x1F,   L_ALTGR,  "@" },          /* 2 */ \
    { 0x20,   L_ALTGR,  "#" },          /* 3 */ \
    { 0x23,   L_ALTGR,  CH_NOT },       /* 6 */ \
    { 0x24,   L_ALTGR,  "|" },          /* 7 */ \
    { 0x25,   L_ALTGR,  CH_CENT },      /* 8 */ \
    { 0x2D,   L_ALTGR,  DK_ACUTE },     /* ' key */ \
    { 0x2E,   L_ALTGR,  DK_TILDE },     /* ^ key */ \
    { 0x2F,   L_ALTGR,  "[" },          /* u-umlaut / e-grave key */ \
    { 0x30,   L_ALTGR,  "]" },          /* diaeresis key */ \
    { 0x31,   L_ALTGR,  "}" },          /* $ key */ \
    { 0x32,   L_ALTGR,  "}" }, \
    { 0x34,   L_ALTGR,  "{" },          /* a-umlaut / a-grave key */ \
    { KC_ISO, L_NORMAL, "<" }, \
    { KC_ISO, L_SHIFT,  ">" }, \
    { KC_ISO, L_ALTGR,  "\\" },
#endif

#ifdef CONFIG_KB_LAYOUT_ENABLE_DE_CH
static const char *const DE_CH_NORMAL[] = {
    LETTERS_QWERTZ, DIGITS, CTRL_KEYS,
    /* 2D */ "'",
    /* 2E */ DK_CIRC,
    /* 2F */ CH_u_UML,
    /* 30 */ DK_DIAER,
    /* 31 */ "$",
    /* 32 */ "$",
    /* 33 */ CH_o_UML,
    /* 34 */ CH_a_UML,
    /* 35 */ CH_SECT,
    /* 36 */ ",", ".", "-",
};

static const char *const DE_CH_SHIFT[] = {
    LETTERS_QWERTZ_UP, CH_SHIFT_DIGITS, CTRL_KEYS,
    /* 2D */ "?",
    /* 2E */ DK_GRAVE,
    /* 2F */ CH_e_GRV,
    /* 30 */ "!",
    /* 31 */ CH_POUND,
    /* 32 */ CH_POUND,
    /* 33 */ CH_e_ACU,
    /* 34 */ CH_a_GRV,
    /* 35 */ CH_DEG,
    /* 36 */ ";", ":", "_",
};

/* Caps Lock gives the capital umlauts, as on Swiss keyboards. */
static const key_entry_t DE_CH_EXTRA[] = {
    CH_EXTRA_COMMON
    { 0x2F,   L_CAPS,   "\xC3\x9C" },   /* U+00DC */
    { 0x33,   L_CAPS,   "\xC3\x96" },   /* U+00D6 */
    { 0x34,   L_CAPS,   "\xC3\x84" },   /* U+00C4 */
    { 0, 0, NULL },
};
#endif /* CONFIG_KB_LAYOUT_ENABLE_DE_CH */

#ifdef CONFIG_KB_LAYOUT_ENABLE_FR_CH
static const char *const FR_CH_NORMAL[] = {
    LETTERS_QWERTZ, DIGITS, CTRL_KEYS,
    /* 2D */ "'",
    /* 2E */ DK_CIRC,
    /* 2F */ CH_e_GRV,
    /* 30 */ DK_DIAER,
    /* 31 */ "$",
    /* 32 */ "$",
    /* 33 */ CH_e_ACU,
    /* 34 */ CH_a_GRV,
    /* 35 */ CH_SECT,
    /* 36 */ ",", ".", "-",
};

static const char *const FR_CH_SHIFT[] = {
    LETTERS_QWERTZ_UP, CH_SHIFT_DIGITS, CTRL_KEYS,
    /* 2D */ "?",
    /* 2E */ DK_GRAVE,
    /* 2F */ CH_u_UML,
    /* 30 */ "!",
    /* 31 */ CH_POUND,
    /* 32 */ CH_POUND,
    /* 33 */ CH_o_UML,
    /* 34 */ CH_a_UML,
    /* 35 */ CH_DEG,
    /* 36 */ ";", ":", "_",
};

/* Caps Lock gives the capital accented letters. */
static const key_entry_t FR_CH_EXTRA[] = {
    CH_EXTRA_COMMON
    { 0x2F,   L_CAPS,   "\xC3\x88" },   /* U+00C8 */
    { 0x33,   L_CAPS,   "\xC3\x89" },   /* U+00C9 */
    { 0x34,   L_CAPS,   "\xC3\x80" },   /* U+00C0 */
    { 0, 0, NULL },
};
#endif /* CONFIG_KB_LAYOUT_ENABLE_FR_CH */

#ifdef CONFIG_KB_LAYOUT_ENABLE_IT
/* ------------------------------------------------------------------ */
/* Italian (QWERTY)                                                   */
/* ------------------------------------------------------------------ */
/* Accented vowels have their own keys, so there are no dead keys.
 * Braces are Shift+AltGr on the bracket keys, as on Windows. */
static const char *const IT_NORMAL[] = {
    LETTERS_QWERTY, DIGITS, CTRL_KEYS,
    /* 2D */ "'",
    /* 2E */ CH_i_GRV,
    /* 2F */ CH_e_GRV,
    /* 30 */ "+",
    /* 31 */ CH_u_GRV,
    /* 32 */ CH_u_GRV,
    /* 33 */ CH_o_GRV,
    /* 34 */ CH_a_GRV,
    /* 35 */ "\\",
    /* 36 */ ",", ".", "-",
};

static const char *const IT_SHIFT[] = {
    LETTERS_QWERTY_UP,
    "!","\x22",CH_POUND,"$","%","&","/","(",")","=",
    CTRL_KEYS,
    /* 2D */ "?",
    /* 2E */ "^",
    /* 2F */ CH_e_ACU,
    /* 30 */ "*",
    /* 31 */ CH_SECT,
    /* 32 */ CH_SECT,
    /* 33 */ CH_c_CED,
    /* 34 */ CH_DEG,
    /* 35 */ "|",
    /* 36 */ ";", ":", "_",
};

static const key_entry_t IT_EXTRA[] = {
    { 0x08,   L_ALTGR,       CH_EURO },  /* e */
    { 0x2F,   L_ALTGR,       "[" },      /* e-grave key */
    { 0x30,   L_ALTGR,       "]" },      /* + key */
    { 0x2F,   L_ALTGR_SHIFT, "{" },
    { 0x30,   L_ALTGR_SHIFT, "}" },
    { 0x33,   L_ALTGR,       "@" },      /* o-grave key */
    { 0x34,   L_ALTGR,       "#" },      /* a-grave key */
    { KC_ISO, L_NORMAL,      "<" },
    { KC_ISO, L_SHIFT,       ">" },
    /* Caps Lock gives the capital accented vowels. */
    { 0x2E,   L_CAPS,        "\xC3\x8C" },  /* U+00CC */
    { 0x2F,   L_CAPS,        "\xC3\x88" },  /* U+00C8 */
    { 0x31,   L_CAPS,        "\xC3\x99" },  /* U+00D9 */
    { 0x32,   L_CAPS,        "\xC3\x99" },
    { 0x33,   L_CAPS,        "\xC3\x92" },  /* U+00D2 */
    { 0x34,   L_CAPS,        "\xC3\x80" },  /* U+00C0 */
    { 0, 0, NULL },
};
#endif /* CONFIG_KB_LAYOUT_ENABLE_IT */

#ifdef CONFIG_KB_LAYOUT_ENABLE_PT
/* ------------------------------------------------------------------ */
/* Portuguese (Portugal, QWERTY)                                      */
/* ------------------------------------------------------------------ */
/* Acute / grave and tilde / circumflex are dead keys (a + tilde ->
 * a-tilde); the diaeresis is AltGr on the + key. */
static const char *const PT_NORMAL[] = {
    LETTERS_QWERTY, DIGITS, CTRL_KEYS,
    /* 2D */ "'",
    /* 2E */ CH_LAQUO,
    /* 2F */ "+",
    /* 30 */ DK_ACUTE,
    /* 31 */ DK_TILDE,
    /* 32 */ DK_TILDE,
    /* 33 */ CH_c_CED,
    /* 34 */ CH_ORDM,
    /* 35 */ "\\",
    /* 36 */ ",", ".", "-",
};

static const char *const PT_SHIFT[] = {
    LETTERS_QWERTY_UP,
    "!","\x22","#","$","%","&","/","(",")","=",
    CTRL_KEYS,
    /* 2D */ "?",
    /* 2E */ CH_RAQUO,
    /* 2F */ "*",
    /* 30 */ DK_GRAVE,
    /* 31 */ DK_CIRC,
    /* 32 */ DK_CIRC,
    /* 33 */ CH_C_CED,
    /* 34 */ CH_ORDF,
    /* 35 */ "|",
    /* 36 */ ";", ":", "_",
};

static const key_entry_t PT_EXTRA[] = {
    { 0x08,   L_ALTGR,  CH_EURO },      /* e */
    { 0x1F,   L_ALTGR,  "@" },          /* 2 */
    { 0x20,   L_ALTGR,  CH_POUND },     /* 3 */
    { 0x21,   L_ALTGR,  CH_SECT },      /* 4 */
    { 0x22,   L_ALTGR,  CH_EURO },      /* 5 */
    { 0x24,   L_ALTGR,  "{" },          /* 7 */
    { 0x25,   L_ALTGR,  "[" },          /* 8 */
    { 0x26,   L_ALTGR,  "]" },          /* 9 */
    { 0x27,   L_ALTGR,  "}" },          /* 0 */
    { 0x2F,   L_ALTGR,  DK_DIAER },     /* + key */
    { KC_ISO, L_NORMAL, "<" },
    { KC_ISO, L_SHIFT,  ">" },
    { 0, 0, NULL },
};
#endif /* CONFIG_KB_LAYOUT_ENABLE_PT */

#ifdef CONFIG_KB_LAYOUT_ENABLE_ES
/* ------------------------------------------------------------------ */
/* Spanish (Spain, QWERTY)                                            */
/* ------------------------------------------------------------------ */
/* Accented vowels are typed with the acute / grave / circumflex /
 * diaeresis dead keys (acute + a -> a-acute); n-tilde has its own
 * key. */
static const char *const ES_NORMAL[] = {
    LETTERS_QWERTY, DIGITS, CTRL_KEYS,
    /* 2D */ "'",
    /* 2E */ CH_IEXCL,
    /* 2F */ DK_GRAVE,
    /* 30 */ "+",
    /* 31 */ CH_c_CED,
    /* 32 */ CH_c_CED,
    /* 33 */ CH_n_TIL,
    /* 34 */ DK_ACUTE,
    /* 35 */ CH_ORDM,
    /* 36 */ ",", ".", "-",
};

static const char *const ES_SHIFT[] = {
    LETTERS_QWERTY_UP,
    "!","\x22",CH_MIDDOT,"$","%","&","/","(",")","=",
    CTRL_KEYS,
    /* 2D */ "?",
    /* 2E */ CH_IQUES,
    /* 2F */ DK_CIRC,
    /* 30 */ "*",
    /* 31 */ CH_C_CED,
    /* 32 */ CH_C_CED,
    /* 33 */ CH_N_TIL,
    /* 34 */ DK_DIAER,
    /* 35 */ CH_ORDF,
    /* 36 */ ";", ":", "_",
};

static const key_entry_t ES_EXTRA[] = {
    { 0x08,   L_ALTGR,  CH_EURO },      /* e */
    { 0x1E,   L_ALTGR,  "|" },          /* 1 */
    { 0x1F,   L_ALTGR,  "@" },          /* 2 */
    { 0x20,   L_ALTGR,  "#" },          /* 3 */
    { 0x21,   L_ALTGR,  "~" },          /* 4 */
    { 0x22,   L_ALTGR,  CH_EURO },      /* 5 */
    { 0x23,   L_ALTGR,  CH_NOT },       /* 6 */
    { 0x2F,   L_ALTGR,  "[" },          /* grave key */
    { 0x30,   L_ALTGR,  "]" },          /* + key */
    { 0x31,   L_ALTGR,  "}" },          /* c-cedilla key */
    { 0x32,   L_ALTGR,  "}" },
    { 0x34,   L_ALTGR,  "{" },          /* acute key */
    { 0x35,   L_ALTGR,  "\\" },         /* ordinal key */
    { KC_ISO, L_NORMAL, "<" },
    { KC_ISO, L_SHIFT,  ">" },
    { 0, 0, NULL },
};
#endif /* CONFIG_KB_LAYOUT_ENABLE_ES */

#ifdef CONFIG_KB_LAYOUT_ENABLE_GB
/* ------------------------------------------------------------------ */
/* United Kingdom (QWERTY)                                            */
/* ------------------------------------------------------------------ */
/* Differs from US in the symbols: Shift+2 is ", Shift+3 the pound
 * sign, @ is Shift+', and the key left of Enter is # / ~. AltGr +
 * vowel types the acute vowels, as on Windows. */
static const char *const GB_NORMAL[] = {
    LETTERS_QWERTY, DIGITS, CTRL_KEYS,
    /* 2D */ "-",
    /* 2E */ "=",
    /* 2F */ "[",
    /* 30 */ "]",
    /* 31 */ "#",
    /* 32 */ "#",
    /* 33 */ ";",
    /* 34 */ "'",
    /* 35 */ "`",
    /* 36 */ ",", ".", "/",
};

static const char *const GB_SHIFT[] = {
    LETTERS_QWERTY_UP,
    "!","\x22",CH_POUND,"$","%","^","&","*","(",")",
    CTRL_KEYS,
    /* 2D */ "_",
    /* 2E */ "+",
    /* 2F */ "{",
    /* 30 */ "}",
    /* 31 */ "~",
    /* 32 */ "~",
    /* 33 */ ":",
    /* 34 */ "@",
    /* 35 */ CH_NOT,
    /* 36 */ "<", ">", "?",
};

static const key_entry_t GB_EXTRA[] = {
    { 0x04,   L_ALTGR,       CH_a_ACU },  /* a */
    { 0x08,   L_ALTGR,       CH_e_ACU },  /* e */
    { 0x0C,   L_ALTGR,       CH_i_ACU },  /* i */
    { 0x12,   L_ALTGR,       CH_o_ACU },  /* o */
    { 0x18,   L_ALTGR,       CH_u_ACU },  /* u */
    { 0x04,   L_ALTGR_SHIFT, CH_A_ACU },
    { 0x08,   L_ALTGR_SHIFT, CH_E_ACU },
    { 0x0C,   L_ALTGR_SHIFT, CH_I_ACU },
    { 0x12,   L_ALTGR_SHIFT, CH_O_ACU },
    { 0x18,   L_ALTGR_SHIFT, CH_U_ACU },
    { 0x21,   L_ALTGR,       CH_EURO },   /* 4 */
    { 0x35,   L_ALTGR,       CH_BROKEN }, /* ` key */
    { KC_ISO, L_NORMAL,      "\\" },
    { KC_ISO, L_SHIFT,       "|" },
    { 0, 0, NULL },
};
#endif /* CONFIG_KB_LAYOUT_ENABLE_GB */

#ifdef CONFIG_KB_LAYOUT_ENABLE_SE_FI
/* ------------------------------------------------------------------ */
/* Swedish / Finnish (QWERTY)                                         */
/* ------------------------------------------------------------------ */
/* Swedish and Finnish share one layout. Acute / grave and diaeresis /
 * circumflex are dead keys; AltGr + the diaeresis key is a dead tilde. */
static const char *const SE_FI_NORMAL[] = {
    /* 04 */ "a",
    /* 05 */ "b",
    /* 06 */ "c",
    /* 07 */ "d",
    /* 08 */ "e",
    /* 09 */ "f",
    /* 0A */ "g",
    /* 0B */ "h",
    /* 0C */ "i",
    /* 0D */ "j",
    /* 0E */ "k",
    /* 0F */ "l",
    /* 10 */ "m",
    /* 11 */ "n",
    /* 12 */ "o",
    /* 13 */ "p",
    /* 14 */ "q",
    /* 15 */ "r",
    /* 16 */ "s",
    /* 17 */ "t",
    /* 18 */ "u",
    /* 19 */ "v",
    /* 1A */ "w",
    /* 1B */ "x",
    /* 1C */ "y",
    /* 1D */ "z",
    /* 1E */ "1",
    /* 1F */ "2",
    /* 20 */ "3",
    /* 21 */ "4",
    /* 22 */ "5",
    /* 23 */ "6",
    /* 24 */ "7",
    /* 25 */ "8",
    /* 26 */ "9",
    /* 27 */ "0",
    /* 28 */ NULL,
    /* 29 */ NULL,
    /* 2A */ NULL,
    /* 2B */ NULL,
    /* 2C */ " ",
    /* 2D */ "+",
    /* 2E */ DK_ACUTE,
    /* 2F */ "\xC3\xA5",  /* U+00E5 latin small letter a with ring above */
    /* 30 */ DK_DIAER,
    /* 31 */ "'",
    /* 32 */ "'",
    /* 33 */ "\xC3\xB6",  /* U+00F6 latin small letter o with diaeresis */
    /* 34 */ "\xC3\xA4",  /* U+00E4 latin small letter a with diaeresis */
    /* 35 */ "\xC2\xA7",  /* U+00A7 section sign */
    /* 36 */ ",",
    /* 37 */ ".",
    /* 38 */ "-",
};

static const char *const SE_FI_SHIFT[] = {
    /* 04 */ "A",
    /* 05 */ "B",
    /* 06 */ "C",
    /* 07 */ "D",
    /* 08 */ "E",
    /* 09 */ "F",
    /* 0A */ "G",
    /* 0B */ "H",
    /* 0C */ "I",
    /* 0D */ "J",
    /* 0E */ "K",
    /* 0F */ "L",
    /* 10 */ "M",
    /* 11 */ "N",
    /* 12 */ "O",
    /* 13 */ "P",
    /* 14 */ "Q",
    /* 15 */ "R",
    /* 16 */ "S",
    /* 17 */ "T",
    /* 18 */ "U",
    /* 19 */ "V",
    /* 1A */ "W",
    /* 1B */ "X",
    /* 1C */ "Y",
    /* 1D */ "Z",
    /* 1E */ "!",
    /* 1F */ "\x22",
    /* 20 */ "#",
    /* 21 */ "\xC2\xA4",  /* U+00A4 currency sign */
    /* 22 */ "%",
    /* 23 */ "&",
    /* 24 */ "/",
    /* 25 */ "(",
    /* 26 */ ")",
    /* 27 */ "=",
    /* 28 */ NULL,
    /* 29 */ NULL,
    /* 2A */ NULL,
    /* 2B */ NULL,
    /* 2C */ " ",
    /* 2D */ "?",
    /* 2E */ DK_GRAVE,
    /* 2F */ "\xC3\x85",  /* U+00C5 latin capital letter a with ring above */
    /* 30 */ DK_CIRC,
    /* 31 */ "*",
    /* 32 */ "*",
    /* 33 */ "\xC3\x96",  /* U+00D6 latin capital letter o with diaeresis */
    /* 34 */ "\xC3\x84",  /* U+00C4 latin capital letter a with diaeresis */
    /* 35 */ "\xC2\xBD",  /* U+00BD vulgar fraction one half */
    /* 36 */ ";",
    /* 37 */ ":",
    /* 38 */ "_",
};

static const key_entry_t SE_FI_EXTRA[] = {
    { 0x1F,  L_ALTGR,        "@" },
    { 0x20,  L_ALTGR,        "\xC2\xA3" },  /* U+00A3 pound sign */
    { 0x21,  L_ALTGR,        "$" },
    { 0x22,  L_ALTGR,        "\xE2\x82\xAC" },  /* U+20AC euro sign */
    { 0x24,  L_ALTGR,        "{" },
    { 0x25,  L_ALTGR,        "[" },
    { 0x26,  L_ALTGR,        "]" },
    { 0x27,  L_ALTGR,        "}" },
    { 0x2D,  L_ALTGR,        "\\" },
    { 0x30,  L_ALTGR,        DK_TILDE },
    { 0x08,  L_ALTGR,        "\xE2\x82\xAC" },  /* U+20AC euro sign */
    { 0x10,  L_ALTGR,        "\xC2\xB5" },  /* U+00B5 micro sign */
    { KC_ISO, L_NORMAL,       "<" },
    { KC_ISO, L_SHIFT,        ">" },
    { KC_ISO, L_ALTGR,        "|" },
    { 0, 0, NULL },
};
#endif /* CONFIG_KB_LAYOUT_ENABLE_SE_FI */

#ifdef CONFIG_KB_LAYOUT_ENABLE_NO
/* ------------------------------------------------------------------ */
/* Norwegian (QWERTY)                                                 */
/* ------------------------------------------------------------------ */
/* Grave / circumflex / diaeresis and (AltGr) acute / tilde are dead keys. */
static const char *const NO_NORMAL[] = {
    /* 04 */ "a",
    /* 05 */ "b",
    /* 06 */ "c",
    /* 07 */ "d",
    /* 08 */ "e",
    /* 09 */ "f",
    /* 0A */ "g",
    /* 0B */ "h",
    /* 0C */ "i",
    /* 0D */ "j",
    /* 0E */ "k",
    /* 0F */ "l",
    /* 10 */ "m",
    /* 11 */ "n",
    /* 12 */ "o",
    /* 13 */ "p",
    /* 14 */ "q",
    /* 15 */ "r",
    /* 16 */ "s",
    /* 17 */ "t",
    /* 18 */ "u",
    /* 19 */ "v",
    /* 1A */ "w",
    /* 1B */ "x",
    /* 1C */ "y",
    /* 1D */ "z",
    /* 1E */ "1",
    /* 1F */ "2",
    /* 20 */ "3",
    /* 21 */ "4",
    /* 22 */ "5",
    /* 23 */ "6",
    /* 24 */ "7",
    /* 25 */ "8",
    /* 26 */ "9",
    /* 27 */ "0",
    /* 28 */ NULL,
    /* 29 */ NULL,
    /* 2A */ NULL,
    /* 2B */ NULL,
    /* 2C */ " ",
    /* 2D */ "+",
    /* 2E */ "\\",
    /* 2F */ "\xC3\xA5",  /* U+00E5 latin small letter a with ring above */
    /* 30 */ DK_DIAER,
    /* 31 */ "'",
    /* 32 */ "'",
    /* 33 */ "\xC3\xB8",  /* U+00F8 latin small letter o with stroke */
    /* 34 */ "\xC3\xA6",  /* U+00E6 latin small letter ae */
    /* 35 */ "|",
    /* 36 */ ",",
    /* 37 */ ".",
    /* 38 */ "-",
};

static const char *const NO_SHIFT[] = {
    /* 04 */ "A",
    /* 05 */ "B",
    /* 06 */ "C",
    /* 07 */ "D",
    /* 08 */ "E",
    /* 09 */ "F",
    /* 0A */ "G",
    /* 0B */ "H",
    /* 0C */ "I",
    /* 0D */ "J",
    /* 0E */ "K",
    /* 0F */ "L",
    /* 10 */ "M",
    /* 11 */ "N",
    /* 12 */ "O",
    /* 13 */ "P",
    /* 14 */ "Q",
    /* 15 */ "R",
    /* 16 */ "S",
    /* 17 */ "T",
    /* 18 */ "U",
    /* 19 */ "V",
    /* 1A */ "W",
    /* 1B */ "X",
    /* 1C */ "Y",
    /* 1D */ "Z",
    /* 1E */ "!",
    /* 1F */ "\x22",
    /* 20 */ "#",
    /* 21 */ "\xC2\xA4",  /* U+00A4 currency sign */
    /* 22 */ "%",
    /* 23 */ "&",
    /* 24 */ "/",
    /* 25 */ "(",
    /* 26 */ ")",
    /* 27 */ "=",
    /* 28 */ NULL,
    /* 29 */ NULL,
    /* 2A */ NULL,
    /* 2B */ NULL,
    /* 2C */ " ",
    /* 2D */ "?",
    /* 2E */ DK_GRAVE,
    /* 2F */ "\xC3\x85",  /* U+00C5 latin capital letter a with ring above */
    /* 30 */ DK_CIRC,
    /* 31 */ "*",
    /* 32 */ "*",
    /* 33 */ "\xC3\x98",  /* U+00D8 latin capital letter o with stroke */
    /* 34 */ "\xC3\x86",  /* U+00C6 latin capital letter ae */
    /* 35 */ "\xC2\xA7",  /* U+00A7 section sign */
    /* 36 */ ";",
    /* 37 */ ":",
    /* 38 */ "_",
};

static const key_entry_t NO_EXTRA[] = {
    { 0x1F,  L_ALTGR,        "@" },
    { 0x20,  L_ALTGR,        "\xC2\xA3" },  /* U+00A3 pound sign */
    { 0x21,  L_ALTGR,        "$" },
    { 0x22,  L_ALTGR,        "\xE2\x82\xAC" },  /* U+20AC euro sign */
    { 0x24,  L_ALTGR,        "{" },
    { 0x25,  L_ALTGR,        "[" },
    { 0x26,  L_ALTGR,        "]" },
    { 0x27,  L_ALTGR,        "}" },
    { 0x2E,  L_ALTGR,        DK_ACUTE },
    { 0x30,  L_ALTGR,        DK_TILDE },
    { 0x08,  L_ALTGR,        "\xE2\x82\xAC" },  /* U+20AC euro sign */
    { 0x10,  L_ALTGR,        "\xC2\xB5" },  /* U+00B5 micro sign */
    { KC_ISO, L_NORMAL,       "<" },
    { KC_ISO, L_SHIFT,        ">" },
    { 0, 0, NULL },
};
#endif /* CONFIG_KB_LAYOUT_ENABLE_NO */

#ifdef CONFIG_KB_LAYOUT_ENABLE_DK
/* ------------------------------------------------------------------ */
/* Danish (QWERTY)                                                    */
/* ------------------------------------------------------------------ */
/* Acute / grave, diaeresis / circumflex and (AltGr) tilde are dead keys. */
static const char *const DK_NORMAL[] = {
    /* 04 */ "a",
    /* 05 */ "b",
    /* 06 */ "c",
    /* 07 */ "d",
    /* 08 */ "e",
    /* 09 */ "f",
    /* 0A */ "g",
    /* 0B */ "h",
    /* 0C */ "i",
    /* 0D */ "j",
    /* 0E */ "k",
    /* 0F */ "l",
    /* 10 */ "m",
    /* 11 */ "n",
    /* 12 */ "o",
    /* 13 */ "p",
    /* 14 */ "q",
    /* 15 */ "r",
    /* 16 */ "s",
    /* 17 */ "t",
    /* 18 */ "u",
    /* 19 */ "v",
    /* 1A */ "w",
    /* 1B */ "x",
    /* 1C */ "y",
    /* 1D */ "z",
    /* 1E */ "1",
    /* 1F */ "2",
    /* 20 */ "3",
    /* 21 */ "4",
    /* 22 */ "5",
    /* 23 */ "6",
    /* 24 */ "7",
    /* 25 */ "8",
    /* 26 */ "9",
    /* 27 */ "0",
    /* 28 */ NULL,
    /* 29 */ NULL,
    /* 2A */ NULL,
    /* 2B */ NULL,
    /* 2C */ " ",
    /* 2D */ "+",
    /* 2E */ DK_ACUTE,
    /* 2F */ "\xC3\xA5",  /* U+00E5 latin small letter a with ring above */
    /* 30 */ DK_DIAER,
    /* 31 */ "'",
    /* 32 */ "'",
    /* 33 */ "\xC3\xA6",  /* U+00E6 latin small letter ae */
    /* 34 */ "\xC3\xB8",  /* U+00F8 latin small letter o with stroke */
    /* 35 */ "\xC2\xBD",  /* U+00BD vulgar fraction one half */
    /* 36 */ ",",
    /* 37 */ ".",
    /* 38 */ "-",
};

static const char *const DK_SHIFT[] = {
    /* 04 */ "A",
    /* 05 */ "B",
    /* 06 */ "C",
    /* 07 */ "D",
    /* 08 */ "E",
    /* 09 */ "F",
    /* 0A */ "G",
    /* 0B */ "H",
    /* 0C */ "I",
    /* 0D */ "J",
    /* 0E */ "K",
    /* 0F */ "L",
    /* 10 */ "M",
    /* 11 */ "N",
    /* 12 */ "O",
    /* 13 */ "P",
    /* 14 */ "Q",
    /* 15 */ "R",
    /* 16 */ "S",
    /* 17 */ "T",
    /* 18 */ "U",
    /* 19 */ "V",
    /* 1A */ "W",
    /* 1B */ "X",
    /* 1C */ "Y",
    /* 1D */ "Z",
    /* 1E */ "!",
    /* 1F */ "\x22",
    /* 20 */ "#",
    /* 21 */ "\xC2\xA4",  /* U+00A4 currency sign */
    /* 22 */ "%",
    /* 23 */ "&",
    /* 24 */ "/",
    /* 25 */ "(",
    /* 26 */ ")",
    /* 27 */ "=",
    /* 28 */ NULL,
    /* 29 */ NULL,
    /* 2A */ NULL,
    /* 2B */ NULL,
    /* 2C */ " ",
    /* 2D */ "?",
    /* 2E */ DK_GRAVE,
    /* 2F */ "\xC3\x85",  /* U+00C5 latin capital letter a with ring above */
    /* 30 */ DK_CIRC,
    /* 31 */ "*",
    /* 32 */ "*",
    /* 33 */ "\xC3\x86",  /* U+00C6 latin capital letter ae */
    /* 34 */ "\xC3\x98",  /* U+00D8 latin capital letter o with stroke */
    /* 35 */ "\xC2\xA7",  /* U+00A7 section sign */
    /* 36 */ ";",
    /* 37 */ ":",
    /* 38 */ "_",
};

static const key_entry_t DK_EXTRA[] = {
    { 0x1F,  L_ALTGR,        "@" },
    { 0x20,  L_ALTGR,        "\xC2\xA3" },  /* U+00A3 pound sign */
    { 0x21,  L_ALTGR,        "$" },
    { 0x22,  L_ALTGR,        "\xE2\x82\xAC" },  /* U+20AC euro sign */
    { 0x24,  L_ALTGR,        "{" },
    { 0x25,  L_ALTGR,        "[" },
    { 0x26,  L_ALTGR,        "]" },
    { 0x27,  L_ALTGR,        "}" },
    { 0x2E,  L_ALTGR,        "|" },
    { 0x30,  L_ALTGR,        DK_TILDE },
    { 0x08,  L_ALTGR,        "\xE2\x82\xAC" },  /* U+20AC euro sign */
    { 0x10,  L_ALTGR,        "\xC2\xB5" },  /* U+00B5 micro sign */
    { KC_ISO, L_NORMAL,       "<" },
    { KC_ISO, L_SHIFT,        ">" },
    { KC_ISO, L_ALTGR,        "\\" },
    { 0, 0, NULL },
};
#endif /* CONFIG_KB_LAYOUT_ENABLE_DK */

#ifdef CONFIG_KB_LAYOUT_ENABLE_PL
/* ------------------------------------------------------------------ */
/* Polish (programmers, QWERTY)                                       */
/* ------------------------------------------------------------------ */
/* The US layout with the Polish letters on AltGr + letter (AltGr + x
 * is z-acute, AltGr + z is z-dot), as on Windows. */
static const key_entry_t PL_EXTRA[] = {
    { 0x04,  L_ALTGR,        "\xC4\x85" },  /* U+0105 latin small letter a with ogonek */
    { 0x06,  L_ALTGR,        "\xC4\x87" },  /* U+0107 latin small letter c with acute */
    { 0x08,  L_ALTGR,        "\xC4\x99" },  /* U+0119 latin small letter e with ogonek */
    { 0x0F,  L_ALTGR,        "\xC5\x82" },  /* U+0142 latin small letter l with stroke */
    { 0x11,  L_ALTGR,        "\xC5\x84" },  /* U+0144 latin small letter n with acute */
    { 0x12,  L_ALTGR,        "\xC3\xB3" },  /* U+00F3 latin small letter o with acute */
    { 0x16,  L_ALTGR,        "\xC5\x9B" },  /* U+015B latin small letter s with acute */
    { 0x1B,  L_ALTGR,        "\xC5\xBA" },  /* U+017A latin small letter z with acute */
    { 0x1D,  L_ALTGR,        "\xC5\xBC" },  /* U+017C latin small letter z with dot above */
    { 0x04,  L_ALTGR_SHIFT,  "\xC4\x84" },  /* U+0104 latin capital letter a with ogonek */
    { 0x06,  L_ALTGR_SHIFT,  "\xC4\x86" },  /* U+0106 latin capital letter c with acute */
    { 0x08,  L_ALTGR_SHIFT,  "\xC4\x98" },  /* U+0118 latin capital letter e with ogonek */
    { 0x0F,  L_ALTGR_SHIFT,  "\xC5\x81" },  /* U+0141 latin capital letter l with stroke */
    { 0x11,  L_ALTGR_SHIFT,  "\xC5\x83" },  /* U+0143 latin capital letter n with acute */
    { 0x12,  L_ALTGR_SHIFT,  "\xC3\x93" },  /* U+00D3 latin capital letter o with acute */
    { 0x16,  L_ALTGR_SHIFT,  "\xC5\x9A" },  /* U+015A latin capital letter s with acute */
    { 0x1B,  L_ALTGR_SHIFT,  "\xC5\xB9" },  /* U+0179 latin capital letter z with acute */
    { 0x1D,  L_ALTGR_SHIFT,  "\xC5\xBB" },  /* U+017B latin capital letter z with dot above */
    { 0x18,  L_ALTGR,        "\xE2\x82\xAC" },  /* U+20AC euro sign */
    { KC_ISO, L_NORMAL,       "\\" },
    { KC_ISO, L_SHIFT,        "|" },
    { 0, 0, NULL },
};
#endif /* CONFIG_KB_LAYOUT_ENABLE_PL */

#ifdef CONFIG_KB_LAYOUT_ENABLE_CZ
/* ------------------------------------------------------------------ */
/* Czech (QWERTZ)                                                     */
/* ------------------------------------------------------------------ */
/* Accented letters on the number row (digits with Shift), acute /
 * caron and ring dead keys; AltGr + the digits are the other
 * accents (caron, circumflex, breve, ring, ogonek, dot, acute,
 * double acute) as dead keys. */
static const char *const CZ_NORMAL[] = {
    /* 04 */ "a",
    /* 05 */ "b",
    /* 06 */ "c",
    /* 07 */ "d",
    /* 08 */ "e",
    /* 09 */ "f",
    /* 0A */ "g",
    /* 0B */ "h",
    /* 0C */ "i",
    /* 0D */ "j",
    /* 0E */ "k",
    /* 0F */ "l",
    /* 10 */ "m",
    /* 11 */ "n",
    /* 12 */ "o",
    /* 13 */ "p",
    /* 14 */ "q",
    /* 15 */ "r",
    /* 16 */ "s",
    /* 17 */ "t",
    /* 18 */ "u",
    /* 19 */ "v",
    /* 1A */ "w",
    /* 1B */ "x",
    /* 1C */ "z",
    /* 1D */ "y",
    /* 1E */ "+",
    /* 1F */ "\xC4\x9B",  /* U+011B latin small letter e with caron */
    /* 20 */ "\xC5\xA1",  /* U+0161 latin small letter s with caron */
    /* 21 */ "\xC4\x8D",  /* U+010D latin small letter c with caron */
    /* 22 */ "\xC5\x99",  /* U+0159 latin small letter r with caron */
    /* 23 */ "\xC5\xBE",  /* U+017E latin small letter z with caron */
    /* 24 */ "\xC3\xBD",  /* U+00FD latin small letter y with acute */
    /* 25 */ "\xC3\xA1",  /* U+00E1 latin small letter a with acute */
    /* 26 */ "\xC3\xAD",  /* U+00ED latin small letter i with acute */
    /* 27 */ "\xC3\xA9",  /* U+00E9 latin small letter e with acute */
    /* 28 */ NULL,
    /* 29 */ NULL,
    /* 2A */ NULL,
    /* 2B */ NULL,
    /* 2C */ " ",
    /* 2D */ "=",
    /* 2E */ DK_ACUTE,
    /* 2F */ "\xC3\xBA",  /* U+00FA latin small letter u with acute */
    /* 30 */ ")",
    /* 31 */ DK_DIAER,
    /* 32 */ DK_DIAER,
    /* 33 */ "\xC5\xAF",  /* U+016F latin small letter u with ring above */
    /* 34 */ "\xC2\xA7",  /* U+00A7 section sign */
    /* 35 */ ";",
    /* 36 */ ",",
    /* 37 */ ".",
    /* 38 */ "-",
};

static const char *const CZ_SHIFT[] = {
    /* 04 */ "A",
    /* 05 */ "B",
    /* 06 */ "C",
    /* 07 */ "D",
    /* 08 */ "E",
    /* 09 */ "F",
    /* 0A */ "G",
    /* 0B */ "H",
    /* 0C */ "I",
    /* 0D */ "J",
    /* 0E */ "K",
    /* 0F */ "L",
    /* 10 */ "M",
    /* 11 */ "N",
    /* 12 */ "O",
    /* 13 */ "P",
    /* 14 */ "Q",
    /* 15 */ "R",
    /* 16 */ "S",
    /* 17 */ "T",
    /* 18 */ "U",
    /* 19 */ "V",
    /* 1A */ "W",
    /* 1B */ "X",
    /* 1C */ "Z",
    /* 1D */ "Y",
    /* 1E */ "1",
    /* 1F */ "2",
    /* 20 */ "3",
    /* 21 */ "4",
    /* 22 */ "5",
    /* 23 */ "6",
    /* 24 */ "7",
    /* 25 */ "8",
    /* 26 */ "9",
    /* 27 */ "0",
    /* 28 */ NULL,
    /* 29 */ NULL,
    /* 2A */ NULL,
    /* 2B */ NULL,
    /* 2C */ " ",
    /* 2D */ "%",
    /* 2E */ DK_CARON,
    /* 2F */ "/",
    /* 30 */ "(",
    /* 31 */ "'",
    /* 32 */ "'",
    /* 33 */ "\x22",
    /* 34 */ "!",
    /* 35 */ DK_RING,
    /* 36 */ "?",
    /* 37 */ ":",
    /* 38 */ "_",
};

static const key_entry_t CZ_EXTRA[] = {
    { 0x1E,  L_ALTGR,        "~" },
    { 0x1F,  L_ALTGR,        DK_CARON },
    { 0x20,  L_ALTGR,        DK_CIRC },
    { 0x21,  L_ALTGR,        DK_BREVE },
    { 0x22,  L_ALTGR,        DK_RING },
    { 0x23,  L_ALTGR,        DK_OGONEK },
    { 0x24,  L_ALTGR,        "`" },
    { 0x25,  L_ALTGR,        DK_DOT },
    { 0x26,  L_ALTGR,        DK_ACUTE },
    { 0x27,  L_ALTGR,        DK_DACUTE },
    { 0x14,  L_ALTGR,        "\\" },
    { 0x1A,  L_ALTGR,        "|" },
    { 0x08,  L_ALTGR,        "\xE2\x82\xAC" },  /* U+20AC euro sign */
    { 0x09,  L_ALTGR,        "[" },
    { 0x0A,  L_ALTGR,        "]" },
    { 0x1B,  L_ALTGR,        "#" },
    { 0x06,  L_ALTGR,        "&" },
    { 0x19,  L_ALTGR,        "@" },
    { 0x05,  L_ALTGR,        "{" },
    { 0x11,  L_ALTGR,        "}" },
    { 0x33,  L_ALTGR,        "$" },
    { 0x34,  L_ALTGR,        "\xC3\x9F" },  /* U+00DF latin small letter sharp s */
    { 0x36,  L_ALTGR,        "<" },
    { 0x37,  L_ALTGR,        ">" },
    { 0x38,  L_ALTGR,        "*" },
    { KC_ISO, L_NORMAL,       "\\" },
    { KC_ISO, L_SHIFT,        "|" },
    { KC_ISO, L_ALTGR,        "/" },
    { 0x1F,  L_CAPS,         "\xC4\x9A" },  /* U+011A latin capital letter e with caron */
    { 0x20,  L_CAPS,         "\xC5\xA0" },  /* U+0160 latin capital letter s with caron */
    { 0x21,  L_CAPS,         "\xC4\x8C" },  /* U+010C latin capital letter c with caron */
    { 0x22,  L_CAPS,         "\xC5\x98" },  /* U+0158 latin capital letter r with caron */
    { 0x23,  L_CAPS,         "\xC5\xBD" },  /* U+017D latin capital letter z with caron */
    { 0x24,  L_CAPS,         "\xC3\x9D" },  /* U+00DD latin capital letter y with acute */
    { 0x25,  L_CAPS,         "\xC3\x81" },  /* U+00C1 latin capital letter a with acute */
    { 0x26,  L_CAPS,         "\xC3\x8D" },  /* U+00CD latin capital letter i with acute */
    { 0x27,  L_CAPS,         "\xC3\x89" },  /* U+00C9 latin capital letter e with acute */
    { 0x2F,  L_CAPS,         "\xC3\x9A" },  /* U+00DA latin capital letter u with acute */
    { 0x33,  L_CAPS,         "\xC5\xAE" },  /* U+016E latin capital letter u with ring above */
    { 0, 0, NULL },
};
#endif /* CONFIG_KB_LAYOUT_ENABLE_CZ */

#ifdef CONFIG_KB_LAYOUT_ENABLE_SK
/* ------------------------------------------------------------------ */
/* Slovak (QWERTZ)                                                    */
/* ------------------------------------------------------------------ */
/* Accented letters on the number row (digits with Shift), acute /
 * caron and ring dead keys; AltGr + the digits are the other
 * accents as dead keys, as on the Czech layout. */
static const char *const SK_NORMAL[] = {
    /* 04 */ "a",
    /* 05 */ "b",
    /* 06 */ "c",
    /* 07 */ "d",
    /* 08 */ "e",
    /* 09 */ "f",
    /* 0A */ "g",
    /* 0B */ "h",
    /* 0C */ "i",
    /* 0D */ "j",
    /* 0E */ "k",
    /* 0F */ "l",
    /* 10 */ "m",
    /* 11 */ "n",
    /* 12 */ "o",
    /* 13 */ "p",
    /* 14 */ "q",
    /* 15 */ "r",
    /* 16 */ "s",
    /* 17 */ "t",
    /* 18 */ "u",
    /* 19 */ "v",
    /* 1A */ "w",
    /* 1B */ "x",
    /* 1C */ "z",
    /* 1D */ "y",
    /* 1E */ "+",
    /* 1F */ "\xC4\xBE",  /* U+013E latin small letter l with caron */
    /* 20 */ "\xC5\xA1",  /* U+0161 latin small letter s with caron */
    /* 21 */ "\xC4\x8D",  /* U+010D latin small letter c with caron */
    /* 22 */ "\xC5\xA5",  /* U+0165 latin small letter t with caron */
    /* 23 */ "\xC5\xBE",  /* U+017E latin small letter z with caron */
    /* 24 */ "\xC3\xBD",  /* U+00FD latin small letter y with acute */
    /* 25 */ "\xC3\xA1",  /* U+00E1 latin small letter a with acute */
    /* 26 */ "\xC3\xAD",  /* U+00ED latin small letter i with acute */
    /* 27 */ "\xC3\xA9",  /* U+00E9 latin small letter e with acute */
    /* 28 */ NULL,
    /* 29 */ NULL,
    /* 2A */ NULL,
    /* 2B */ NULL,
    /* 2C */ " ",
    /* 2D */ "=",
    /* 2E */ DK_ACUTE,
    /* 2F */ "\xC3\xBA",  /* U+00FA latin small letter u with acute */
    /* 30 */ "\xC3\xA4",  /* U+00E4 latin small letter a with diaeresis */
    /* 31 */ "\xC5\x88",  /* U+0148 latin small letter n with caron */
    /* 32 */ "\xC5\x88",  /* U+0148 latin small letter n with caron */
    /* 33 */ "\xC3\xB4",  /* U+00F4 latin small letter o with circumflex */
    /* 34 */ "\xC2\xA7",  /* U+00A7 section sign */
    /* 35 */ ";",
    /* 36 */ ",",
    /* 37 */ ".",
    /* 38 */ "-",
};

static const char *const SK_SHIFT[] = {
    /* 04 */ "A",
    /* 05 */ "B",
    /* 06 */ "C",
    /* 07 */ "D",
    /* 08 */ "E",
    /* 09 */ "F",
    /* 0A */ "G",
    /* 0B */ "H",
    /* 0C */ "I",
    /* 0D */ "J",
    /* 0E */ "K",
    /* 0F */ "L",
    /* 10 */ "M",
    /* 11 */ "N",
    /* 12 */ "O",
    /* 13 */ "P",
    /* 14 */ "Q",
    /* 15 */ "R",
    /* 16 */ "S",
    /* 17 */ "T",
    /* 18 */ "U",
    /* 19 */ "V",
    /* 1A */ "W",
    /* 1B */ "X",
    /* 1C */ "Z",
    /* 1D */ "Y",
    /* 1E */ "1",
    /* 1F */ "2",
    /* 20 */ "3",
    /* 21 */ "4",
    /* 22 */ "5",
    /* 23 */ "6",
    /* 24 */ "7",
    /* 25 */ "8",
    /* 26 */ "9",
    /* 27 */ "0",
    /* 28 */ NULL,
    /* 29 */ NULL,
    /* 2A */ NULL,
    /* 2B */ NULL,
    /* 2C */ " ",
    /* 2D */ "%",
    /* 2E */ DK_CARON,
    /* 2F */ "/",
    /* 30 */ "(",
    /* 31 */ ")",
    /* 32 */ ")",
    /* 33 */ "\x22",
    /* 34 */ "!",
    /* 35 */ DK_RING,
    /* 36 */ "?",
    /* 37 */ ":",
    /* 38 */ "_",
};

static const key_entry_t SK_EXTRA[] = {
    { 0x1E,  L_ALTGR,        "~" },
    { 0x1F,  L_ALTGR,        DK_CARON },
    { 0x20,  L_ALTGR,        DK_CIRC },
    { 0x21,  L_ALTGR,        DK_BREVE },
    { 0x22,  L_ALTGR,        DK_RING },
    { 0x23,  L_ALTGR,        DK_OGONEK },
    { 0x24,  L_ALTGR,        "`" },
    { 0x25,  L_ALTGR,        DK_DOT },
    { 0x26,  L_ALTGR,        DK_ACUTE },
    { 0x27,  L_ALTGR,        DK_DACUTE },
    { 0x14,  L_ALTGR,        "\\" },
    { 0x1A,  L_ALTGR,        "|" },
    { 0x08,  L_ALTGR,        "\xE2\x82\xAC" },  /* U+20AC euro sign */
    { 0x09,  L_ALTGR,        "[" },
    { 0x0A,  L_ALTGR,        "]" },
    { 0x1B,  L_ALTGR,        "#" },
    { 0x06,  L_ALTGR,        "&" },
    { 0x19,  L_ALTGR,        "@" },
    { 0x05,  L_ALTGR,        "{" },
    { 0x11,  L_ALTGR,        "}" },
    { 0x33,  L_ALTGR,        "$" },
    { 0x34,  L_ALTGR,        "\xC3\x9F" },  /* U+00DF latin small letter sharp s */
    { 0x36,  L_ALTGR,        "<" },
    { 0x37,  L_ALTGR,        ">" },
    { 0x38,  L_ALTGR,        "*" },
    { KC_ISO, L_NORMAL,       "&" },
    { KC_ISO, L_SHIFT,        "*" },
    { KC_ISO, L_ALTGR,        "\\" },
    { 0x1F,  L_CAPS,         "\xC4\xBD" },  /* U+013D latin capital letter l with caron */
    { 0x20,  L_CAPS,         "\xC5\xA0" },  /* U+0160 latin capital letter s with caron */
    { 0x21,  L_CAPS,         "\xC4\x8C" },  /* U+010C latin capital letter c with caron */
    { 0x22,  L_CAPS,         "\xC5\xA4" },  /* U+0164 latin capital letter t with caron */
    { 0x23,  L_CAPS,         "\xC5\xBD" },  /* U+017D latin capital letter z with caron */
    { 0x24,  L_CAPS,         "\xC3\x9D" },  /* U+00DD latin capital letter y with acute */
    { 0x25,  L_CAPS,         "\xC3\x81" },  /* U+00C1 latin capital letter a with acute */
    { 0x26,  L_CAPS,         "\xC3\x8D" },  /* U+00CD latin capital letter i with acute */
    { 0x27,  L_CAPS,         "\xC3\x89" },  /* U+00C9 latin capital letter e with acute */
    { 0x2F,  L_CAPS,         "\xC3\x9A" },  /* U+00DA latin capital letter u with acute */
    { 0x30,  L_CAPS,         "\xC3\x84" },  /* U+00C4 latin capital letter a with diaeresis */
    { 0x31,  L_CAPS,         "\xC5\x87" },  /* U+0147 latin capital letter n with caron */
    { 0x32,  L_CAPS,         "\xC5\x87" },  /* U+0147 latin capital letter n with caron */
    { 0x33,  L_CAPS,         "\xC3\x94" },  /* U+00D4 latin capital letter o with circumflex */
    { 0, 0, NULL },
};
#endif /* CONFIG_KB_LAYOUT_ENABLE_SK */

#ifdef CONFIG_KB_LAYOUT_ENABLE_HU
/* ------------------------------------------------------------------ */
/* Hungarian (QWERTZ)                                                 */
/* ------------------------------------------------------------------ */
/* Every Hungarian letter has its own key (i-acute is the ISO key
 * left of Y); 0 is left of 1. AltGr + the digits are dead accents. */
static const char *const HU_NORMAL[] = {
    /* 04 */ "a",
    /* 05 */ "b",
    /* 06 */ "c",
    /* 07 */ "d",
    /* 08 */ "e",
    /* 09 */ "f",
    /* 0A */ "g",
    /* 0B */ "h",
    /* 0C */ "i",
    /* 0D */ "j",
    /* 0E */ "k",
    /* 0F */ "l",
    /* 10 */ "m",
    /* 11 */ "n",
    /* 12 */ "o",
    /* 13 */ "p",
    /* 14 */ "q",
    /* 15 */ "r",
    /* 16 */ "s",
    /* 17 */ "t",
    /* 18 */ "u",
    /* 19 */ "v",
    /* 1A */ "w",
    /* 1B */ "x",
    /* 1C */ "z",
    /* 1D */ "y",
    /* 1E */ "1",
    /* 1F */ "2",
    /* 20 */ "3",
    /* 21 */ "4",
    /* 22 */ "5",
    /* 23 */ "6",
    /* 24 */ "7",
    /* 25 */ "8",
    /* 26 */ "9",
    /* 27 */ "\xC3\xB6",  /* U+00F6 latin small letter o with diaeresis */
    /* 28 */ NULL,
    /* 29 */ NULL,
    /* 2A */ NULL,
    /* 2B */ NULL,
    /* 2C */ " ",
    /* 2D */ "\xC3\xBC",  /* U+00FC latin small letter u with diaeresis */
    /* 2E */ "\xC3\xB3",  /* U+00F3 latin small letter o with acute */
    /* 2F */ "\xC5\x91",  /* U+0151 latin small letter o with double acute */
    /* 30 */ "\xC3\xBA",  /* U+00FA latin small letter u with acute */
    /* 31 */ "\xC5\xB1",  /* U+0171 latin small letter u with double acute */
    /* 32 */ "\xC5\xB1",  /* U+0171 latin small letter u with double acute */
    /* 33 */ "\xC3\xA9",  /* U+00E9 latin small letter e with acute */
    /* 34 */ "\xC3\xA1",  /* U+00E1 latin small letter a with acute */
    /* 35 */ "0",
    /* 36 */ ",",
    /* 37 */ ".",
    /* 38 */ "-",
};

static const char *const HU_SHIFT[] = {
    /* 04 */ "A",
    /* 05 */ "B",
    /* 06 */ "C",
    /* 07 */ "D",
    /* 08 */ "E",
    /* 09 */ "F",
    /* 0A */ "G",
    /* 0B */ "H",
    /* 0C */ "I",
    /* 0D */ "J",
    /* 0E */ "K",
    /* 0F */ "L",
    /* 10 */ "M",
    /* 11 */ "N",
    /* 12 */ "O",
    /* 13 */ "P",
    /* 14 */ "Q",
    /* 15 */ "R",
    /* 16 */ "S",
    /* 17 */ "T",
    /* 18 */ "U",
    /* 19 */ "V",
    /* 1A */ "W",
    /* 1B */ "X",
    /* 1C */ "Z",
    /* 1D */ "Y",
    /* 1E */ "'",
    /* 1F */ "\x22",
    /* 20 */ "+",
    /* 21 */ "!",
    /* 22 */ "%",
    /* 23 */ "/",
    /* 24 */ "=",
    /* 25 */ "(",
    /* 26 */ ")",
    /* 27 */ "\xC3\x96",  /* U+00D6 latin capital letter o with diaeresis */
    /* 28 */ NULL,
    /* 29 */ NULL,
    /* 2A */ NULL,
    /* 2B */ NULL,
    /* 2C */ " ",
    /* 2D */ "\xC3\x9C",  /* U+00DC latin capital letter u with diaeresis */
    /* 2E */ "\xC3\x93",  /* U+00D3 latin capital letter o with acute */
    /* 2F */ "\xC5\x90",  /* U+0150 latin capital letter o with double acute */
    /* 30 */ "\xC3\x9A",  /* U+00DA latin capital letter u with acute */
    /* 31 */ "\xC5\xB0",  /* U+0170 latin capital letter u with double acute */
    /* 32 */ "\xC5\xB0",  /* U+0170 latin capital letter u with double acute */
    /* 33 */ "\xC3\x89",  /* U+00C9 latin capital letter e with acute */
    /* 34 */ "\xC3\x81",  /* U+00C1 latin capital letter a with acute */
    /* 35 */ "\xC2\xA7",  /* U+00A7 section sign */
    /* 36 */ "?",
    /* 37 */ ":",
    /* 38 */ "_",
};

static const key_entry_t HU_EXTRA[] = {
    { 0x1E,  L_ALTGR,        "~" },
    { 0x1F,  L_ALTGR,        DK_CARON },
    { 0x20,  L_ALTGR,        DK_CIRC },
    { 0x21,  L_ALTGR,        DK_BREVE },
    { 0x22,  L_ALTGR,        DK_RING },
    { 0x23,  L_ALTGR,        DK_OGONEK },
    { 0x24,  L_ALTGR,        "`" },
    { 0x25,  L_ALTGR,        DK_DOT },
    { 0x26,  L_ALTGR,        DK_ACUTE },
    { 0x27,  L_ALTGR,        DK_DACUTE },
    { 0x14,  L_ALTGR,        "\\" },
    { 0x1A,  L_ALTGR,        "|" },
    { 0x18,  L_ALTGR,        "\xE2\x82\xAC" },  /* U+20AC euro sign */
    { 0x09,  L_ALTGR,        "[" },
    { 0x0A,  L_ALTGR,        "]" },
    { 0x0E,  L_ALTGR,        "\xC5\x82" },  /* U+0142 latin small letter l with stroke */
    { 0x0F,  L_ALTGR,        "\xC5\x81" },  /* U+0141 latin capital letter l with stroke */
    { 0x1B,  L_ALTGR,        "#" },
    { 0x06,  L_ALTGR,        "&" },
    { 0x19,  L_ALTGR,        "@" },
    { 0x05,  L_ALTGR,        "{" },
    { 0x11,  L_ALTGR,        "}" },
    { 0x33,  L_ALTGR,        "$" },
    { 0x34,  L_ALTGR,        "\xC3\x9F" },  /* U+00DF latin small letter sharp s */
    { 0x36,  L_ALTGR,        ";" },
    { 0x37,  L_ALTGR,        ">" },
    { 0x38,  L_ALTGR,        "*" },
    { KC_ISO, L_NORMAL,       "\xC3\xAD" },  /* U+00ED latin small letter i with acute */
    { KC_ISO, L_SHIFT,        "\xC3\x8D" },  /* U+00CD latin capital letter i with acute */
    { KC_ISO, L_ALTGR,        "<" },
    { 0, 0, NULL },
};
#endif /* CONFIG_KB_LAYOUT_ENABLE_HU */

#ifdef CONFIG_KB_LAYOUT_ENABLE_RO
/* ------------------------------------------------------------------ */
/* Romanian (standard, QWERTY)                                        */
/* ------------------------------------------------------------------ */
/* Romanian standard layout (SR 13392): s and t with comma below,
 * a-breve, a / i-circumflex on their own keys, low / high quotes left
 * of 1, and AltGr + the digits as dead accents. */
static const char *const RO_NORMAL[] = {
    /* 04 */ "a",
    /* 05 */ "b",
    /* 06 */ "c",
    /* 07 */ "d",
    /* 08 */ "e",
    /* 09 */ "f",
    /* 0A */ "g",
    /* 0B */ "h",
    /* 0C */ "i",
    /* 0D */ "j",
    /* 0E */ "k",
    /* 0F */ "l",
    /* 10 */ "m",
    /* 11 */ "n",
    /* 12 */ "o",
    /* 13 */ "p",
    /* 14 */ "q",
    /* 15 */ "r",
    /* 16 */ "s",
    /* 17 */ "t",
    /* 18 */ "u",
    /* 19 */ "v",
    /* 1A */ "w",
    /* 1B */ "x",
    /* 1C */ "y",
    /* 1D */ "z",
    /* 1E */ "1",
    /* 1F */ "2",
    /* 20 */ "3",
    /* 21 */ "4",
    /* 22 */ "5",
    /* 23 */ "6",
    /* 24 */ "7",
    /* 25 */ "8",
    /* 26 */ "9",
    /* 27 */ "0",
    /* 28 */ NULL,
    /* 29 */ NULL,
    /* 2A */ NULL,
    /* 2B */ NULL,
    /* 2C */ " ",
    /* 2D */ "-",
    /* 2E */ "=",
    /* 2F */ "\xC4\x83",  /* U+0103 latin small letter a with breve */
    /* 30 */ "\xC3\xAE",  /* U+00EE latin small letter i with circumflex */
    /* 31 */ "\xC3\xA2",  /* U+00E2 latin small letter a with circumflex */
    /* 32 */ "\xC3\xA2",  /* U+00E2 latin small letter a with circumflex */
    /* 33 */ "\xC8\x99",  /* U+0219 latin small letter s with comma below */
    /* 34 */ "\xC8\x9B",  /* U+021B latin small letter t with comma below */
    /* 35 */ "\xE2\x80\x9E",  /* U+201E double low-9 quotation mark */
    /* 36 */ ",",
    /* 37 */ ".",
    /* 38 */ "/",
};

static const char *const RO_SHIFT[] = {
    /* 04 */ "A",
    /* 05 */ "B",
    /* 06 */ "C",
    /* 07 */ "D",
    /* 08 */ "E",
    /* 09 */ "F",
    /* 0A */ "G",
    /* 0B */ "H",
    /* 0C */ "I",
    /* 0D */ "J",
    /* 0E */ "K",
    /* 0F */ "L",
    /* 10 */ "M",
    /* 11 */ "N",
    /* 12 */ "O",
    /* 13 */ "P",
    /* 14 */ "Q",
    /* 15 */ "R",
    /* 16 */ "S",
    /* 17 */ "T",
    /* 18 */ "U",
    /* 19 */ "V",
    /* 1A */ "W",
    /* 1B */ "X",
    /* 1C */ "Y",
    /* 1D */ "Z",
    /* 1E */ "!",
    /* 1F */ "@",
    /* 20 */ "#",
    /* 21 */ "$",
    /* 22 */ "%",
    /* 23 */ "^",
    /* 24 */ "&",
    /* 25 */ "*",
    /* 26 */ "(",
    /* 27 */ ")",
    /* 28 */ NULL,
    /* 29 */ NULL,
    /* 2A */ NULL,
    /* 2B */ NULL,
    /* 2C */ " ",
    /* 2D */ "_",
    /* 2E */ "+",
    /* 2F */ "\xC4\x82",  /* U+0102 latin capital letter a with breve */
    /* 30 */ "\xC3\x8E",  /* U+00CE latin capital letter i with circumflex */
    /* 31 */ "\xC3\x82",  /* U+00C2 latin capital letter a with circumflex */
    /* 32 */ "\xC3\x82",  /* U+00C2 latin capital letter a with circumflex */
    /* 33 */ "\xC8\x98",  /* U+0218 latin capital letter s with comma below */
    /* 34 */ "\xC8\x9A",  /* U+021A latin capital letter t with comma below */
    /* 35 */ "\xE2\x80\x9D",  /* U+201D right double quotation mark */
    /* 36 */ ";",
    /* 37 */ ":",
    /* 38 */ "?",
};

static const key_entry_t RO_EXTRA[] = {
    { 0x1E,  L_ALTGR,        "~" },
    { 0x1F,  L_ALTGR,        DK_CARON },
    { 0x20,  L_ALTGR,        DK_CIRC },
    { 0x21,  L_ALTGR,        DK_BREVE },
    { 0x22,  L_ALTGR,        DK_RING },
    { 0x23,  L_ALTGR,        DK_OGONEK },
    { 0x24,  L_ALTGR,        "`" },
    { 0x25,  L_ALTGR,        DK_DOT },
    { 0x26,  L_ALTGR,        DK_ACUTE },
    { 0x27,  L_ALTGR,        DK_DACUTE },
    { 0x08,  L_ALTGR,        "\xE2\x82\xAC" },  /* U+20AC euro sign */
    { 0x16,  L_ALTGR,        "\xC3\x9F" },  /* U+00DF latin small letter sharp s */
    { 0x07,  L_ALTGR,        "\xC4\x91" },  /* U+0111 latin small letter d with stroke */
    { 0x07,  L_ALTGR_SHIFT,  "\xC4\x90" },  /* U+0110 latin capital letter d with stroke */
    { 0x0F,  L_ALTGR,        "\xC5\x82" },  /* U+0142 latin small letter l with stroke */
    { 0x0F,  L_ALTGR_SHIFT,  "\xC5\x81" },  /* U+0141 latin capital letter l with stroke */
    { 0x2F,  L_ALTGR,        "[" },
    { 0x2F,  L_ALTGR_SHIFT,  "{" },
    { 0x30,  L_ALTGR,        "]" },
    { 0x30,  L_ALTGR_SHIFT,  "}" },
    { 0x31,  L_ALTGR,        "\\" },
    { 0x31,  L_ALTGR_SHIFT,  "|" },
    { 0x32,  L_ALTGR,        "\\" },
    { 0x32,  L_ALTGR_SHIFT,  "|" },
    { 0x33,  L_ALTGR,        ";" },
    { 0x33,  L_ALTGR_SHIFT,  ":" },
    { 0x34,  L_ALTGR,        "'" },
    { 0x34,  L_ALTGR_SHIFT,  "\x22" },
    { 0x35,  L_ALTGR,        "`" },
    { 0x35,  L_ALTGR_SHIFT,  "~" },
    { 0x36,  L_ALTGR,        "<" },
    { 0x36,  L_ALTGR_SHIFT,  "\xC2\xAB" },  /* U+00AB left-pointing double angle quotation mark */
    { 0x37,  L_ALTGR,        ">" },
    { 0x37,  L_ALTGR_SHIFT,  "\xC2\xBB" },  /* U+00BB right-pointing double angle quotation mark */
    { KC_ISO, L_NORMAL,       "\\" },
    { KC_ISO, L_SHIFT,        "|" },
    { 0, 0, NULL },
};
#endif /* CONFIG_KB_LAYOUT_ENABLE_RO */

#ifdef CONFIG_KB_LAYOUT_ENABLE_HR
/* ------------------------------------------------------------------ */
/* Croatian / Slovenian (QWERTZ)                                      */
/* ------------------------------------------------------------------ */
/* Croatian and Slovenian share one layout. Cedilla / diaeresis left
 * of 1 are dead keys; AltGr + the digits are the other accents. */
static const char *const HR_NORMAL[] = {
    /* 04 */ "a",
    /* 05 */ "b",
    /* 06 */ "c",
    /* 07 */ "d",
    /* 08 */ "e",
    /* 09 */ "f",
    /* 0A */ "g",
    /* 0B */ "h",
    /* 0C */ "i",
    /* 0D */ "j",
    /* 0E */ "k",
    /* 0F */ "l",
    /* 10 */ "m",
    /* 11 */ "n",
    /* 12 */ "o",
    /* 13 */ "p",
    /* 14 */ "q",
    /* 15 */ "r",
    /* 16 */ "s",
    /* 17 */ "t",
    /* 18 */ "u",
    /* 19 */ "v",
    /* 1A */ "w",
    /* 1B */ "x",
    /* 1C */ "z",
    /* 1D */ "y",
    /* 1E */ "1",
    /* 1F */ "2",
    /* 20 */ "3",
    /* 21 */ "4",
    /* 22 */ "5",
    /* 23 */ "6",
    /* 24 */ "7",
    /* 25 */ "8",
    /* 26 */ "9",
    /* 27 */ "0",
    /* 28 */ NULL,
    /* 29 */ NULL,
    /* 2A */ NULL,
    /* 2B */ NULL,
    /* 2C */ " ",
    /* 2D */ "'",
    /* 2E */ "+",
    /* 2F */ "\xC5\xA1",  /* U+0161 latin small letter s with caron */
    /* 30 */ "\xC4\x91",  /* U+0111 latin small letter d with stroke */
    /* 31 */ "\xC5\xBE",  /* U+017E latin small letter z with caron */
    /* 32 */ "\xC5\xBE",  /* U+017E latin small letter z with caron */
    /* 33 */ "\xC4\x8D",  /* U+010D latin small letter c with caron */
    /* 34 */ "\xC4\x87",  /* U+0107 latin small letter c with acute */
    /* 35 */ DK_CEDIL,
    /* 36 */ ",",
    /* 37 */ ".",
    /* 38 */ "-",
};

static const char *const HR_SHIFT[] = {
    /* 04 */ "A",
    /* 05 */ "B",
    /* 06 */ "C",
    /* 07 */ "D",
    /* 08 */ "E",
    /* 09 */ "F",
    /* 0A */ "G",
    /* 0B */ "H",
    /* 0C */ "I",
    /* 0D */ "J",
    /* 0E */ "K",
    /* 0F */ "L",
    /* 10 */ "M",
    /* 11 */ "N",
    /* 12 */ "O",
    /* 13 */ "P",
    /* 14 */ "Q",
    /* 15 */ "R",
    /* 16 */ "S",
    /* 17 */ "T",
    /* 18 */ "U",
    /* 19 */ "V",
    /* 1A */ "W",
    /* 1B */ "X",
    /* 1C */ "Z",
    /* 1D */ "Y",
    /* 1E */ "!",
    /* 1F */ "\x22",
    /* 20 */ "#",
    /* 21 */ "$",
    /* 22 */ "%",
    /* 23 */ "&",
    /* 24 */ "/",
    /* 25 */ "(",
    /* 26 */ ")",
    /* 27 */ "=",
    /* 28 */ NULL,
    /* 29 */ NULL,
    /* 2A */ NULL,
    /* 2B */ NULL,
    /* 2C */ " ",
    /* 2D */ "?",
    /* 2E */ "*",
    /* 2F */ "\xC5\xA0",  /* U+0160 latin capital letter s with caron */
    /* 30 */ "\xC4\x90",  /* U+0110 latin capital letter d with stroke */
    /* 31 */ "\xC5\xBD",  /* U+017D latin capital letter z with caron */
    /* 32 */ "\xC5\xBD",  /* U+017D latin capital letter z with caron */
    /* 33 */ "\xC4\x8C",  /* U+010C latin capital letter c with caron */
    /* 34 */ "\xC4\x86",  /* U+0106 latin capital letter c with acute */
    /* 35 */ DK_DIAER,
    /* 36 */ ";",
    /* 37 */ ":",
    /* 38 */ "_",
};

static const key_entry_t HR_EXTRA[] = {
    { 0x1E,  L_ALTGR,        "~" },
    { 0x1F,  L_ALTGR,        DK_CARON },
    { 0x20,  L_ALTGR,        DK_CIRC },
    { 0x21,  L_ALTGR,        DK_BREVE },
    { 0x22,  L_ALTGR,        DK_RING },
    { 0x23,  L_ALTGR,        DK_OGONEK },
    { 0x24,  L_ALTGR,        "`" },
    { 0x25,  L_ALTGR,        DK_DOT },
    { 0x26,  L_ALTGR,        DK_ACUTE },
    { 0x27,  L_ALTGR,        DK_DACUTE },
    { 0x14,  L_ALTGR,        "\\" },
    { 0x1A,  L_ALTGR,        "|" },
    { 0x08,  L_ALTGR,        "\xE2\x82\xAC" },  /* U+20AC euro sign */
    { 0x09,  L_ALTGR,        "[" },
    { 0x0A,  L_ALTGR,        "]" },
    { 0x0E,  L_ALTGR,        "\xC5\x82" },  /* U+0142 latin small letter l with stroke */
    { 0x0F,  L_ALTGR,        "\xC5\x81" },  /* U+0141 latin capital letter l with stroke */
    { 0x19,  L_ALTGR,        "@" },
    { 0x05,  L_ALTGR,        "{" },
    { 0x11,  L_ALTGR,        "}" },
    { 0x10,  L_ALTGR,        "\xC2\xA7" },  /* U+00A7 section sign */
    { KC_ISO, L_NORMAL,       "<" },
    { KC_ISO, L_SHIFT,        ">" },
    { 0, 0, NULL },
};
#endif /* CONFIG_KB_LAYOUT_ENABLE_HR */

#ifdef CONFIG_KB_LAYOUT_ENABLE_TR
/* ------------------------------------------------------------------ */
/* Turkish Q (QWERTY)                                                 */
/* ------------------------------------------------------------------ */
/* The i key types dotless i (Shift: I) and the key right of L types
 * i (Shift: I-dot), as on Turkish keyboards. Shift+3 is a dead
 * circumflex; AltGr gives the other dead accents. */
static const char *const TR_NORMAL[] = {
    /* 04 */ "a",
    /* 05 */ "b",
    /* 06 */ "c",
    /* 07 */ "d",
    /* 08 */ "e",
    /* 09 */ "f",
    /* 0A */ "g",
    /* 0B */ "h",
    /* 0C */ "\xC4\xB1",  /* U+0131 latin small letter dotless i */
    /* 0D */ "j",
    /* 0E */ "k",
    /* 0F */ "l",
    /* 10 */ "m",
    /* 11 */ "n",
    /* 12 */ "o",
    /* 13 */ "p",
    /* 14 */ "q",
    /* 15 */ "r",
    /* 16 */ "s",
    /* 17 */ "t",
    /* 18 */ "u",
    /* 19 */ "v",
    /* 1A */ "w",
    /* 1B */ "x",
    /* 1C */ "y",
    /* 1D */ "z",
    /* 1E */ "1",
    /* 1F */ "2",
    /* 20 */ "3",
    /* 21 */ "4",
    /* 22 */ "5",
    /* 23 */ "6",
    /* 24 */ "7",
    /* 25 */ "8",
    /* 26 */ "9",
    /* 27 */ "0",
    /* 28 */ NULL,
    /* 29 */ NULL,
    /* 2A */ NULL,
    /* 2B */ NULL,
    /* 2C */ " ",
    /* 2D */ "*",
    /* 2E */ "-",
    /* 2F */ "\xC4\x9F",  /* U+011F latin small letter g with breve */
    /* 30 */ "\xC3\xBC",  /* U+00FC latin small letter u with diaeresis */
    /* 31 */ ",",
    /* 32 */ ",",
    /* 33 */ "\xC5\x9F",  /* U+015F latin small letter s with cedilla */
    /* 34 */ "i",
    /* 35 */ "\x22",
    /* 36 */ "\xC3\xB6",  /* U+00F6 latin small letter o with diaeresis */
    /* 37 */ "\xC3\xA7",  /* U+00E7 latin small letter c with cedilla */
    /* 38 */ ".",
};

static const char *const TR_SHIFT[] = {
    /* 04 */ "A",
    /* 05 */ "B",
    /* 06 */ "C",
    /* 07 */ "D",
    /* 08 */ "E",
    /* 09 */ "F",
    /* 0A */ "G",
    /* 0B */ "H",
    /* 0C */ "I",
    /* 0D */ "J",
    /* 0E */ "K",
    /* 0F */ "L",
    /* 10 */ "M",
    /* 11 */ "N",
    /* 12 */ "O",
    /* 13 */ "P",
    /* 14 */ "Q",
    /* 15 */ "R",
    /* 16 */ "S",
    /* 17 */ "T",
    /* 18 */ "U",
    /* 19 */ "V",
    /* 1A */ "W",
    /* 1B */ "X",
    /* 1C */ "Y",
    /* 1D */ "Z",
    /* 1E */ "!",
    /* 1F */ "'",
    /* 20 */ DK_CIRC,
    /* 21 */ "+",
    /* 22 */ "%",
    /* 23 */ "&",
    /* 24 */ "/",
    /* 25 */ "(",
    /* 26 */ ")",
    /* 27 */ "=",
    /* 28 */ NULL,
    /* 29 */ NULL,
    /* 2A */ NULL,
    /* 2B */ NULL,
    /* 2C */ " ",
    /* 2D */ "?",
    /* 2E */ "_",
    /* 2F */ "\xC4\x9E",  /* U+011E latin capital letter g with breve */
    /* 30 */ "\xC3\x9C",  /* U+00DC latin capital letter u with diaeresis */
    /* 31 */ ";",
    /* 32 */ ";",
    /* 33 */ "\xC5\x9E",  /* U+015E latin capital letter s with cedilla */
    /* 34 */ "\xC4\xB0",  /* U+0130 latin capital letter i with dot above */
    /* 35 */ "\xC3\xA9",  /* U+00E9 latin small letter e with acute */
    /* 36 */ "\xC3\x96",  /* U+00D6 latin capital letter o with diaeresis */
    /* 37 */ "\xC3\x87",  /* U+00C7 latin capital letter c with cedilla */
    /* 38 */ ":",
};

static const key_entry_t TR_EXTRA[] = {
    { 0x1E,  L_ALTGR,        ">" },
    { 0x1F,  L_ALTGR,        "\xC2\xA3" },  /* U+00A3 pound sign */
    { 0x20,  L_ALTGR,        "#" },
    { 0x21,  L_ALTGR,        "$" },
    { 0x22,  L_ALTGR,        "\xC2\xBD" },  /* U+00BD vulgar fraction one half */
    { 0x24,  L_ALTGR,        "{" },
    { 0x25,  L_ALTGR,        "[" },
    { 0x26,  L_ALTGR,        "]" },
    { 0x27,  L_ALTGR,        "}" },
    { 0x2D,  L_ALTGR,        "\\" },
    { 0x2E,  L_ALTGR,        "|" },
    { 0x2F,  L_ALTGR,        DK_DIAER },
    { 0x30,  L_ALTGR,        DK_TILDE },
    { 0x33,  L_ALTGR,        DK_ACUTE },
    { 0x31,  L_ALTGR,        DK_GRAVE },
    { 0x32,  L_ALTGR,        DK_GRAVE },
    { 0x35,  L_ALTGR,        "<" },
    { 0x14,  L_ALTGR,        "@" },
    { 0x08,  L_ALTGR,        "\xE2\x82\xAC" },  /* U+20AC euro sign */
    { KC_ISO, L_NORMAL,       "<" },
    { KC_ISO, L_SHIFT,        ">" },
    { KC_ISO, L_ALTGR,        "|" },
    { 0, 0, NULL },
};
#endif /* CONFIG_KB_LAYOUT_ENABLE_TR */

#ifdef CONFIG_KB_LAYOUT_ENABLE_FR_CA
/* ------------------------------------------------------------------ */
/* Canadian French, traditional (QWERTY)                              */
/* ------------------------------------------------------------------ */
/* Windows "Canadian French" (KBDCA), XKB "ca": e-acute right of the
 * period key, dead circumflex / cedilla / diaeresis / grave on the
 * bracket and quote keys, # | on the key left of 1, guillemets on the
 * ISO key, the Latin-1 symbols on AltGr. */
static const char *const FR_CA_NORMAL[] = {
    LETTERS_QWERTY, DIGITS, CTRL_KEYS,
    /* 2D */ "-",
    /* 2E */ "=",
    /* 2F */ DK_CIRC,
    /* 30 */ DK_CEDIL,
    /* 31 */ "<",
    /* 32 */ "<",
    /* 33 */ ";",
    /* 34 */ DK_GRAVE,
    /* 35 */ "#",
    /* 36 */ ",", ".", CH_e_ACU,
};

static const char *const FR_CA_SHIFT[] = {
    LETTERS_QWERTY_UP,
    "!","\x22","/","$","%","?","&","*","(",")",
    CTRL_KEYS,
    /* 2D */ "_",
    /* 2E */ "+",
    /* 2F */ DK_CIRC,
    /* 30 */ DK_DIAER,
    /* 31 */ ">",
    /* 32 */ ">",
    /* 33 */ ":",
    /* 34 */ DK_GRAVE,
    /* 35 */ "|",
    /* 36 */ "'", ".", CH_E_ACU,
};

static const key_entry_t FR_CA_EXTRA[] = {
    { 0x1E,   L_ALTGR,  "\xC2\xB1" },   /* 1: U+00B1 plus-minus */
    { 0x1F,   L_ALTGR,  "@" },          /* 2 */
    { 0x20,   L_ALTGR,  CH_POUND },     /* 3 */
    { 0x21,   L_ALTGR,  CH_CENT },      /* 4 */
    { 0x22,   L_ALTGR,  "\xC2\xA4" },   /* 5: U+00A4 currency sign */
    { 0x23,   L_ALTGR,  CH_NOT },       /* 6 */
    { 0x24,   L_ALTGR,  CH_BROKEN },    /* 7 */
    { 0x25,   L_ALTGR,  "\xC2\xB2" },   /* 8: U+00B2 superscript 2 */
    { 0x26,   L_ALTGR,  "\xC2\xB3" },   /* 9: U+00B3 superscript 3 */
    { 0x27,   L_ALTGR,  "\xC2\xBC" },   /* 0: U+00BC one quarter */
    { 0x2D,   L_ALTGR,  "\xC2\xBD" },   /* - key: U+00BD one half */
    { 0x2E,   L_ALTGR,  "\xC2\xBE" },   /* = key: U+00BE three quarters */
    { 0x12,   L_ALTGR,  CH_SECT },      /* o */
    { 0x13,   L_ALTGR,  "\xC2\xB6" },   /* p: U+00B6 pilcrow */
    { 0x10,   L_ALTGR,  CH_MICRO },     /* m */
    { 0x2F,   L_ALTGR,  "[" },          /* circumflex key */
    { 0x30,   L_ALTGR,  "]" },          /* cedilla key */
    { 0x31,   L_ALTGR,  "}" },          /* < > key */
    { 0x32,   L_ALTGR,  "}" },
    { 0x33,   L_ALTGR,  "~" },          /* ; key */
    { 0x34,   L_ALTGR,  "{" },          /* grave key */
    { 0x35,   L_ALTGR,  "\\" },         /* # key */
    { 0x36,   L_ALTGR,  "\xC2\xAF" },   /* , key: U+00AF macron */
    { 0x37,   L_ALTGR,  "\xC2\xAD" },   /* . key: U+00AD soft hyphen */
    { 0x38,   L_ALTGR,  DK_ACUTE },     /* e-acute key */
    { 0x2C,   L_ALTGR,  "\xC2\xA0" },   /* Space: U+00A0 no-break space */
    { KC_ISO, L_NORMAL, CH_LAQUO },
    { KC_ISO, L_SHIFT,  CH_RAQUO },
    { KC_ISO, L_ALTGR,  CH_DEG },
    { 0, 0, NULL },
};
#endif /* CONFIG_KB_LAYOUT_ENABLE_FR_CA */

#ifdef CONFIG_KB_LAYOUT_ENABLE_BR
/* ------------------------------------------------------------------ */
/* Portuguese (Brazil, ABNT2)                                         */
/* ------------------------------------------------------------------ */
/* Windows "Portuguese (Brazilian ABNT2)", XKB "br(abnt2)": c-cedilla
 * right of L, dead acute / grave right of P, dead tilde / circumflex
 * right of c-cedilla, dead diaeresis on Shift + 6, ' " left of 1,
 * \ | on the ISO key left of Z, and the extra ABNT2 key left of Right
 * Shift (HID 0x87, International1) typing / ? (AltGr: degree). The
 * keys right of the brackets carry ordinal indicators on AltGr. */
#define KC_INTL1 0x87
static const char *const BR_NORMAL[] = {
    LETTERS_QWERTY, DIGITS, CTRL_KEYS,
    /* 2D */ "-",
    /* 2E */ "=",
    /* 2F */ DK_ACUTE,
    /* 30 */ "[",
    /* 31 */ "]",
    /* 32 */ "]",
    /* 33 */ CH_c_CED,
    /* 34 */ DK_TILDE,
    /* 35 */ "'",
    /* 36 */ ",", ".", ";",
};

static const char *const BR_SHIFT[] = {
    LETTERS_QWERTY_UP,
    "!","@","#","$","%",DK_DIAER,"&","*","(",")",
    CTRL_KEYS,
    /* 2D */ "_",
    /* 2E */ "+",
    /* 2F */ DK_GRAVE,
    /* 30 */ "{",
    /* 31 */ "}",
    /* 32 */ "}",
    /* 33 */ CH_C_CED,
    /* 34 */ DK_CIRC,
    /* 35 */ "\x22",
    /* 36 */ "<", ">", ":",
};

static const key_entry_t BR_EXTRA[] = {
    { 0x1E,     L_ALTGR,  "\xC2\xB9" },   /* 1: U+00B9 superscript 1 */
    { 0x1F,     L_ALTGR,  "\xC2\xB2" },   /* 2: U+00B2 superscript 2 */
    { 0x20,     L_ALTGR,  "\xC2\xB3" },   /* 3: U+00B3 superscript 3 */
    { 0x21,     L_ALTGR,  CH_POUND },     /* 4 */
    { 0x22,     L_ALTGR,  CH_CENT },      /* 5 */
    { 0x23,     L_ALTGR,  CH_NOT },       /* 6 */
    { 0x2E,     L_ALTGR,  CH_SECT },      /* = key */
    { 0x14,     L_ALTGR,  "/" },          /* q */
    { 0x1A,     L_ALTGR,  "?" },          /* w */
    { 0x08,     L_ALTGR,  CH_DEG },       /* e */
    { 0x15,     L_ALTGR,  "\xC2\xAE" },   /* r: U+00AE registered */
    { 0x06,     L_ALTGR,  "\xC2\xA9" },   /* c: U+00A9 copyright */
    { 0x10,     L_ALTGR,  CH_MICRO },     /* m */
    { 0x30,     L_ALTGR,  CH_ORDF },      /* [ key */
    { 0x31,     L_ALTGR,  CH_ORDM },      /* ] key */
    { 0x32,     L_ALTGR,  CH_ORDM },
    { KC_ISO,   L_NORMAL, "\\" },
    { KC_ISO,   L_SHIFT,  "|" },
    { KC_INTL1, L_NORMAL, "/" },
    { KC_INTL1, L_SHIFT,  "?" },
    { KC_INTL1, L_ALTGR,  CH_DEG },
    { 0, 0, NULL },
};
#endif /* CONFIG_KB_LAYOUT_ENABLE_BR */

#if defined(CONFIG_KB_LAYOUT_ENABLE_CRH) || defined(CONFIG_KB_LAYOUT_ENABLE_CRH_B)
/* ------------------------------------------------------------------ */
/* Crimean Tatar, Latin (Ave Team)                                    */
/* ------------------------------------------------------------------ */
/* Ave Team's Crimean Tatar Roman layout (keyboard.aveteam.org/en/
 * crh-more; sources, CC0: gitlab.com/aveteam/crimean-tatar-keyboard-
 * layout-sources, Windows/Regular/CRH.klc and Version B/CRHB.klc).
 * QWERTY letters with the Ukrainian (Windows) punctuation; the
 * Crimean Tatar letters on the keys that are Cyrillic letters on the
 * Ukrainian layout: g-breve / u-diaeresis on [ ], s-cedilla / i on
 * ; ', o-diaeresis / c-cedilla on , . , a-circumflex left of 1 and
 * n-tilde on the backslash key. The I key types dotless i (Shift: I);
 * variant B (CRH-B) swaps it with the key right of S-cedilla (i,
 * Shift: I-dot). AltGr + a / s / g / c / n / o / u / i types the
 * letter with its diacritic; AltGr + Shift + 6 / 7 / 8 and AltGr +
 * Shift + the a-circumflex key are dead breve, acute, comma below and
 * circumflex (for the Romanian Crimean Tatar spelling). */
#define CH_i_DOTLESS "\xC4\xB1"  /* U+0131 latin small letter dotless i */
#define CH_I_DOT     "\xC4\xB0"  /* U+0130 latin capital letter i with dot above */
#define CH_a_CIRC    "\xC3\xA2"  /* U+00E2 latin small letter a with circumflex */
#define CH_A_CIRC    "\xC3\x82"  /* U+00C2 latin capital letter a with circumflex */
#define CH_g_BREVE   "\xC4\x9F"  /* U+011F latin small letter g with breve */
#define CH_G_BREVE   "\xC4\x9E"  /* U+011E latin capital letter g with breve */
#define CH_s_CED     "\xC5\x9F"  /* U+015F latin small letter s with cedilla */
#define CH_S_CED     "\xC5\x9E"  /* U+015E latin capital letter s with cedilla */
#define CH_O_UML     "\xC3\x96"  /* U+00D6 latin capital letter o with diaeresis */
#define CH_U_UML     "\xC3\x9C"  /* U+00DC latin capital letter u with diaeresis */

/* Keycodes 0x04..0x1D with the I key (0x0C) given by the variant. */
#define CRH_LETTERS(i_key) \
    "a","b","c","d","e","f","g","h",i_key,"j","k","l", \
    "m","n","o","p","q","r","s","t","u","v","w","x","y","z"
#define CRH_LETTERS_UP(i_key) \
    "A","B","C","D","E","F","G","H",i_key,"J","K","L", \
    "M","N","O","P","Q","R","S","T","U","V","W","X","Y","Z"

/* 0x1E..0x38 after the letters; quote_key is the key right of
 * s-cedilla (0x34). */
#define CRH_REST_NORMAL(quote_key) \
    DIGITS, CTRL_KEYS, \
    /* 2D */ "-", \
    /* 2E */ "=", \
    /* 2F */ CH_g_BREVE, \
    /* 30 */ CH_u_UML, \
    /* 31 */ CH_n_TIL, \
    /* 32 */ CH_n_TIL, \
    /* 33 */ CH_s_CED, \
    /* 34 */ quote_key, \
    /* 35 */ CH_a_CIRC, \
    /* 36 */ CH_o_UML, \
    /* 37 */ CH_c_CED, \
    /* 38 */ "."
#define CRH_REST_SHIFT(quote_key) \
    "!","\x22","\xE2\x84\x96",";","%",":","?","*","(",")", \
    CTRL_KEYS, \
    /* 2D */ "_", \
    /* 2E */ "+", \
    /* 2F */ CH_G_BREVE, \
    /* 30 */ CH_U_UML, \
    /* 31 */ CH_N_TIL, \
    /* 32 */ CH_N_TIL, \
    /* 33 */ CH_S_CED, \
    /* 34 */ quote_key, \
    /* 35 */ CH_A_CIRC, \
    /* 36 */ CH_O_UML, \
    /* 37 */ CH_C_CED, \
    /* 38 */ ","

/* AltGr layers; i_lo / i_up are AltGr (+ Shift) on the I key. */
#define CRH_EXTRA_ENTRIES(i_lo, i_up) \
    { 0x1E,   L_ALTGR_SHIFT, "|" }, \
    { 0x1F,   L_ALTGR_SHIFT, "@" }, \
    { 0x20,   L_ALTGR_SHIFT, "#" }, \
    { 0x21,   L_ALTGR,       "$" }, \
    { 0x21,   L_ALTGR_SHIFT, CH_EURO }, \
    { 0x23,   L_ALTGR_SHIFT, DK_BREVE }, \
    { 0x24,   L_ALTGR_SHIFT, DK_ACUTE }, \
    { 0x25,   L_ALTGR_SHIFT, DK_COMMA }, \
    { 0x26,   L_ALTGR_SHIFT, "`" }, \
    { 0x2D,   L_ALTGR,       "\xE2\x80\x93" },  /* U+2013 en dash */ \
    { 0x2D,   L_ALTGR_SHIFT, "\xE2\x80\x94" },  /* U+2014 em dash */ \
    { 0x2E,   L_ALTGR,       "{" }, \
    { 0x2E,   L_ALTGR_SHIFT, "}" }, \
    { 0x2F,   L_ALTGR,       "[" }, \
    { 0x30,   L_ALTGR,       "]" }, \
    { 0x31,   L_ALTGR,       "\\" }, \
    { 0x31,   L_ALTGR_SHIFT, "/" }, \
    { 0x32,   L_ALTGR,       "\\" }, \
    { 0x32,   L_ALTGR_SHIFT, "/" }, \
    { 0x34,   L_ALTGR,       "\xE2\x80\x99" },  /* U+2019 right single quotation mark */ \
    { 0x34,   L_ALTGR_SHIFT, "'" }, \
    { 0x35,   L_ALTGR,       "\xE2\x82\xB4" },  /* U+20B4 hryvnia sign */ \
    { 0x35,   L_ALTGR_SHIFT, DK_CIRC }, \
    { 0x38,   L_ALTGR,       "<" }, \
    { 0x38,   L_ALTGR_SHIFT, ">" }, \
    { 0x04,   L_ALTGR,       CH_a_CIRC }, \
    { 0x04,   L_ALTGR_SHIFT, CH_A_CIRC }, \
    { 0x06,   L_ALTGR,       CH_c_CED }, \
    { 0x06,   L_ALTGR_SHIFT, CH_C_CED }, \
    { 0x0A,   L_ALTGR,       CH_g_BREVE }, \
    { 0x0A,   L_ALTGR_SHIFT, CH_G_BREVE }, \
    { 0x0C,   L_ALTGR,       i_lo }, \
    { 0x0C,   L_ALTGR_SHIFT, i_up }, \
    { 0x11,   L_ALTGR,       CH_n_TIL }, \
    { 0x11,   L_ALTGR_SHIFT, CH_N_TIL }, \
    { 0x12,   L_ALTGR,       CH_o_UML }, \
    { 0x12,   L_ALTGR_SHIFT, CH_O_UML }, \
    { 0x16,   L_ALTGR,       CH_s_CED }, \
    { 0x16,   L_ALTGR_SHIFT, CH_S_CED }, \
    { 0x18,   L_ALTGR,       CH_u_UML }, \
    { 0x18,   L_ALTGR_SHIFT, CH_U_UML }, \
    { KC_ISO, L_NORMAL,      "\\" }, \
    { KC_ISO, L_SHIFT,       "/" }, \
    { 0, 0, NULL }
#endif

#ifdef CONFIG_KB_LAYOUT_ENABLE_CRH
static const char *const CRH_NORMAL[] = {
    CRH_LETTERS(CH_i_DOTLESS), CRH_REST_NORMAL("i"),
};
static const char *const CRH_SHIFT[] = {
    CRH_LETTERS_UP("I"), CRH_REST_SHIFT(CH_I_DOT),
};
static const key_entry_t CRH_EXTRA[] = { CRH_EXTRA_ENTRIES("i", CH_I_DOT) };
#endif

#ifdef CONFIG_KB_LAYOUT_ENABLE_CRH_B
static const char *const CRH_B_NORMAL[] = {
    CRH_LETTERS("i"), CRH_REST_NORMAL(CH_i_DOTLESS),
};
static const char *const CRH_B_SHIFT[] = {
    CRH_LETTERS_UP(CH_I_DOT), CRH_REST_SHIFT("I"),
};
static const key_entry_t CRH_B_EXTRA[] = { CRH_EXTRA_ENTRIES(CH_i_DOTLESS, "I") };
#endif

/* ------------------------------------------------------------------ */
/* Layout table structure                                             */
/* ------------------------------------------------------------------ */

/* is_latin: the normal (unshifted) layer produces plain Latin letters
 * -- used by kb_layout_shortcut_char() below to decide whether
 * Ctrl-shortcut letters should follow the national layout (Latin) or
 * fall back to the US physical key position (non-Latin scripts, which
 * have no national Latin letter to remap a shortcut to). */
typedef struct {
    const char *const *normal;    /* keycodes 0x04..0x38 */
    const char *const *shift;     /* keycodes 0x04..0x38 */
    const key_entry_t *extra;  /* AltGr layers, keys past 0x38; or NULL */
    const char *name;          /* short tag (title bar) */
    const char *long_name;     /* human-readable name (settings) */
    kb_ime_t ime;
    bool is_latin;
} layout_table_t;

#define ARRAY_SIZE(a) ((int)(sizeof(a) / sizeof((a)[0])))

/* Every normal / shift table covers exactly keycodes 0x04..0x38. */
#define CHECK_TABLE(t) \
    static_assert(ARRAY_SIZE(t) == TABLE_KEYS, #t " must have 53 entries")

CHECK_TABLE(US_NORMAL); CHECK_TABLE(US_SHIFT);
#ifdef CONFIG_KB_LAYOUT_ENABLE_UA
CHECK_TABLE(UA_NORMAL); CHECK_TABLE(UA_SHIFT);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_DE
CHECK_TABLE(DE_NORMAL); CHECK_TABLE(DE_SHIFT);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_FR
CHECK_TABLE(FR_NORMAL); CHECK_TABLE(FR_SHIFT);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_HE
CHECK_TABLE(HE_NORMAL); CHECK_TABLE(HE_SHIFT);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_DE_CH
CHECK_TABLE(DE_CH_NORMAL); CHECK_TABLE(DE_CH_SHIFT);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_FR_CH
CHECK_TABLE(FR_CH_NORMAL); CHECK_TABLE(FR_CH_SHIFT);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_IT
CHECK_TABLE(IT_NORMAL); CHECK_TABLE(IT_SHIFT);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_PT
CHECK_TABLE(PT_NORMAL); CHECK_TABLE(PT_SHIFT);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_ES
CHECK_TABLE(ES_NORMAL); CHECK_TABLE(ES_SHIFT);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_GB
CHECK_TABLE(GB_NORMAL); CHECK_TABLE(GB_SHIFT);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_SE_FI
CHECK_TABLE(SE_FI_NORMAL); CHECK_TABLE(SE_FI_SHIFT);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_NO
CHECK_TABLE(NO_NORMAL); CHECK_TABLE(NO_SHIFT);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_DK
CHECK_TABLE(DK_NORMAL); CHECK_TABLE(DK_SHIFT);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_CZ
CHECK_TABLE(CZ_NORMAL); CHECK_TABLE(CZ_SHIFT);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_SK
CHECK_TABLE(SK_NORMAL); CHECK_TABLE(SK_SHIFT);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_HU
CHECK_TABLE(HU_NORMAL); CHECK_TABLE(HU_SHIFT);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_RO
CHECK_TABLE(RO_NORMAL); CHECK_TABLE(RO_SHIFT);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_HR
CHECK_TABLE(HR_NORMAL); CHECK_TABLE(HR_SHIFT);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_TR
CHECK_TABLE(TR_NORMAL); CHECK_TABLE(TR_SHIFT);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_FR_CA
CHECK_TABLE(FR_CA_NORMAL); CHECK_TABLE(FR_CA_SHIFT);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_CRH
CHECK_TABLE(CRH_NORMAL); CHECK_TABLE(CRH_SHIFT);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_CRH_B
CHECK_TABLE(CRH_B_NORMAL); CHECK_TABLE(CRH_B_SHIFT);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_BR
CHECK_TABLE(BR_NORMAL); CHECK_TABLE(BR_SHIFT);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_KO
CHECK_TABLE(KO_NORMAL); CHECK_TABLE(KO_SHIFT);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_TA_IN
CHECK_TABLE(TA_IN_NORMAL); CHECK_TABLE(TA_IN_SHIFT);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_HI_IN
CHECK_TABLE(HI_IN_NORMAL); CHECK_TABLE(HI_IN_SHIFT);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_MR_IN
CHECK_TABLE(MR_IN_NORMAL); CHECK_TABLE(MR_IN_SHIFT);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_NE_TR
CHECK_TABLE(NE_TR_NORMAL); CHECK_TABLE(NE_TR_SHIFT);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_NE_RO
CHECK_TABLE(NE_RO_NORMAL); CHECK_TABLE(NE_RO_SHIFT);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_BN_IN
CHECK_TABLE(BN_IN_NORMAL); CHECK_TABLE(BN_IN_SHIFT);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_BN_PR
CHECK_TABLE(BN_PR_NORMAL); CHECK_TABLE(BN_PR_SHIFT);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_BN_NJ
CHECK_TABLE(BN_NJ_NORMAL); CHECK_TABLE(BN_NJ_SHIFT);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_TE_IN
CHECK_TABLE(TE_IN_NORMAL); CHECK_TABLE(TE_IN_SHIFT);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_TE_SA
CHECK_TABLE(TE_SA_NORMAL); CHECK_TABLE(TE_SA_SHIFT);
#endif

/* The active set is a 32-bit mask (persisted in NVS). */
static_assert(KB_LAYOUT_COUNT <= 64, "too many layouts for the active mask");

/* In kb_layout_id_t order (C++ needs designators in order). */
static const layout_table_t s_layouts[KB_LAYOUT_COUNT] = {
#ifdef CONFIG_KB_LAYOUT_ENABLE_US
    [KB_LAYOUT_US] = { US_NORMAL, US_SHIFT, NULL, "US", "English (US)", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_UA
    [KB_LAYOUT_UA] = { UA_NORMAL, UA_SHIFT, NULL, "UA", "Ukrainian", KB_IME_NONE, false },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_DE
    [KB_LAYOUT_DE] = { DE_NORMAL, DE_SHIFT, DE_EXTRA, "DE", "German", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_FR
    [KB_LAYOUT_FR] = { FR_NORMAL, FR_SHIFT, FR_EXTRA, "FR", "French (AZERTY)", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_HE
    [KB_LAYOUT_HE] = { HE_NORMAL, HE_SHIFT, NULL, "HE", "Hebrew", KB_IME_NONE, false },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_DE_CH
    [KB_LAYOUT_DE_CH] = { DE_CH_NORMAL, DE_CH_SHIFT, DE_CH_EXTRA, "DE-CH", "Swiss German", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_FR_CH
    [KB_LAYOUT_FR_CH] = { FR_CH_NORMAL, FR_CH_SHIFT, FR_CH_EXTRA, "FR-CH", "Swiss French", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_IT
    [KB_LAYOUT_IT] = { IT_NORMAL, IT_SHIFT, IT_EXTRA, "IT", "Italian", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_PT
    [KB_LAYOUT_PT] = { PT_NORMAL, PT_SHIFT, PT_EXTRA, "PT", "Portuguese", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_ES
    [KB_LAYOUT_ES] = { ES_NORMAL, ES_SHIFT, ES_EXTRA, "ES", "Spanish", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_GB
    [KB_LAYOUT_GB] = { GB_NORMAL, GB_SHIFT, GB_EXTRA, "GB", "English (UK)", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_SE_FI
    [KB_LAYOUT_SE_FI] = { SE_FI_NORMAL, SE_FI_SHIFT, SE_FI_EXTRA, "SE/FI", "Swedish / Finnish", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_NO
    [KB_LAYOUT_NO] = { NO_NORMAL, NO_SHIFT, NO_EXTRA, "NO", "Norwegian", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_DK
    [KB_LAYOUT_DK] = { DK_NORMAL, DK_SHIFT, DK_EXTRA, "DK", "Danish", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_PL
    [KB_LAYOUT_PL] = { US_NORMAL, US_SHIFT, PL_EXTRA, "PL", "Polish", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_CZ
    [KB_LAYOUT_CZ] = { CZ_NORMAL, CZ_SHIFT, CZ_EXTRA, "CZ", "Czech", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_SK
    [KB_LAYOUT_SK] = { SK_NORMAL, SK_SHIFT, SK_EXTRA, "SK", "Slovak", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_HU
    [KB_LAYOUT_HU] = { HU_NORMAL, HU_SHIFT, HU_EXTRA, "HU", "Hungarian", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_RO
    [KB_LAYOUT_RO] = { RO_NORMAL, RO_SHIFT, RO_EXTRA, "RO", "Romanian", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_HR
    [KB_LAYOUT_HR] = { HR_NORMAL, HR_SHIFT, HR_EXTRA, "HR/SI", "Croatian / Slovenian", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_TR
    [KB_LAYOUT_TR] = { TR_NORMAL, TR_SHIFT, TR_EXTRA, "TR", "Turkish", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_FR_CA
    [KB_LAYOUT_FR_CA] = { FR_CA_NORMAL, FR_CA_SHIFT, FR_CA_EXTRA, "FR-CA", "Canadian French", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_CRH
    [KB_LAYOUT_CRH] = { CRH_NORMAL, CRH_SHIFT, CRH_EXTRA, "CRH", "Crimean Tatar (Latin)", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_CRH_B
    [KB_LAYOUT_CRH_B] = { CRH_B_NORMAL, CRH_B_SHIFT, CRH_B_EXTRA, "CRH-B", "Crimean Tatar (Latin, variant B)", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_VI
    [KB_LAYOUT_VI] = { US_NORMAL, US_SHIFT, NULL, "VI", "Vietnamese (Telex)", KB_IME_TELEX, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_KO
    [KB_LAYOUT_KO] = { KO_NORMAL, KO_SHIFT, NULL, "KO", "Korean (2-set)", KB_IME_HANGUL, false },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_JA
    [KB_LAYOUT_JA] = { US_NORMAL, US_SHIFT, NULL, "JA", "Japanese (romaji)", KB_IME_KANA, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_ZH
    [KB_LAYOUT_ZH] = { US_NORMAL, US_SHIFT, NULL, "ZH", "Chinese (pinyin)", KB_IME_PINYIN, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_TA99
    [KB_LAYOUT_TA99] = { US_NORMAL, US_SHIFT, NULL, "TA99", "Tamil 99", KB_IME_TAMIL99, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_TA_TW
    [KB_LAYOUT_TA_TW] = { US_NORMAL, US_SHIFT, NULL, "TA-TW", "Tamil Typewriter", KB_IME_TAMIL_TW, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_TA_IN
    [KB_LAYOUT_TA_IN] = { TA_IN_NORMAL, TA_IN_SHIFT, TA_IN_EXTRA, "TA-IN", "Tamil InScript", KB_IME_NONE, false },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_TA_TV
    [KB_LAYOUT_TA_TV] = { US_NORMAL, US_SHIFT, NULL, "TA-TV", "Tamil Taravinyas", KB_IME_TARAVINYAS, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_HI_IN
    [KB_LAYOUT_HI_IN] = { HI_IN_NORMAL, HI_IN_SHIFT, HI_IN_EXTRA, "HI-IN", "Hindi InScript", KB_IME_NONE, false },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_MR_IN
    [KB_LAYOUT_MR_IN] = { MR_IN_NORMAL, MR_IN_SHIFT, MR_IN_EXTRA, "MR-IN", "Marathi InScript", KB_IME_NONE, false },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_NE_TR
    [KB_LAYOUT_NE_TR] = { NE_TR_NORMAL, NE_TR_SHIFT, NULL, "NE-TR", "Nepali Traditional", KB_IME_NONE, false },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_NE_RO
    [KB_LAYOUT_NE_RO] = { NE_RO_NORMAL, NE_RO_SHIFT, NULL, "NE-RO", "Nepali Romanized", KB_IME_NONE, false },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_BN_IN
    [KB_LAYOUT_BN_IN] = { BN_IN_NORMAL, BN_IN_SHIFT, BN_IN_EXTRA, "BN-IN", "Bengali InScript", KB_IME_NONE, false },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_BN_PR
    [KB_LAYOUT_BN_PR] = { BN_PR_NORMAL, BN_PR_SHIFT, BN_PR_EXTRA, "BN-PR", "Bengali Probhat", KB_IME_NONE, false },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_BN_NJ
    [KB_LAYOUT_BN_NJ] = { BN_NJ_NORMAL, BN_NJ_SHIFT, BN_NJ_EXTRA, "BN-NJ", "Bengali National (Jatiya)", KB_IME_BENGALI_NJ, false },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_BR
    [KB_LAYOUT_BR] = { BR_NORMAL, BR_SHIFT, BR_EXTRA, "BR", "Portuguese (Brazil, ABNT2)", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_TE_IN
    [KB_LAYOUT_TE_IN] = { TE_IN_NORMAL, TE_IN_SHIFT, TE_IN_EXTRA, "TE-IN", "Telugu InScript", KB_IME_NONE, false },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_TE_SA
    [KB_LAYOUT_TE_SA] = { TE_SA_NORMAL, TE_SA_SHIFT, TE_SA_EXTRA, "TE-SA", "Telugu Sarala", KB_IME_NONE, false },
#endif
};

/* ------------------------------------------------------------------ */
/* Dead keys                                                          */
/* ------------------------------------------------------------------ */

/* Letters each accent combines with, and the code points they become
 * (Latin-1 or Latin Extended-A; the layouts with the accents beyond
 * Latin-1 pull in the Latin Extended-A font subset). */
typedef struct {
    const char *spacing;   /* the accent alone (DK_* without \x01) */
    const char *bases;     /* ASCII letters it combines with */
    const uint16_t *cps;   /* result per letter */
} dead_key_t;

#define DK_TABLE(name, bases, ...)                                        \
    static const uint16_t name##_CPS[] = { __VA_ARGS__ };                 \
    static_assert(sizeof(name##_CPS) / 2 == sizeof(bases) - 1,          \
                  #name " letters and results differ in length");        \
    static const char name##_BASES[] = bases;

DK_TABLE(GRAVE, "aeiouAEIOU",
         0xE0,0xE8,0xEC,0xF2,0xF9, 0xC0,0xC8,0xCC,0xD2,0xD9)
DK_TABLE(ACUTE, "aeiouycnszlrAEIOUYCNSZLR",
         0xE1,0xE9,0xED,0xF3,0xFA,0xFD,0x107,0x144,0x15B,0x17A,0x13A,0x155,
         0xC1,0xC9,0xCD,0xD3,0xDA,0xDD,0x106,0x143,0x15A,0x179,0x139,0x154)
DK_TABLE(CIRC, "aeiouAEIOU",
         0xE2,0xEA,0xEE,0xF4,0xFB, 0xC2,0xCA,0xCE,0xD4,0xDB)
DK_TABLE(TILDE, "aonAON",
         0xE3,0xF5,0xF1, 0xC3,0xD5,0xD1)
DK_TABLE(DIAER, "aeiouyAEIOU",
         0xE4,0xEB,0xEF,0xF6,0xFC,0xFF, 0xC4,0xCB,0xCF,0xD6,0xDC)
DK_TABLE(CEDIL, "cstgklnrCSTGKLNR",
         0xE7,0x15F,0x163,0x123,0x137,0x13C,0x146,0x157,
         0xC7,0x15E,0x162,0x122,0x136,0x13B,0x145,0x156)
DK_TABLE(CARON, "cdenrstzlCDENRSTZL",
         0x10D,0x10F,0x11B,0x148,0x159,0x161,0x165,0x17E,0x13E,
         0x10C,0x10E,0x11A,0x147,0x158,0x160,0x164,0x17D,0x13D)
DK_TABLE(BREVE, "aguiAGUI",
         0x103,0x11F,0x16D,0x12D, 0x102,0x11E,0x16C,0x12C)
DK_TABLE(DOT, "zecgIZECG",
         0x17C,0x117,0x10B,0x121, 0x130, 0x17B,0x116,0x10A,0x120)
DK_TABLE(RING, "auAU",
         0xE5,0x16F, 0xC5,0x16E)
DK_TABLE(OGONEK, "aeiuAEIU",
         0x105,0x119,0x12F,0x173, 0x104,0x118,0x12E,0x172)
DK_TABLE(DACUTE, "ouOU",
         0x151,0x171, 0x150,0x170)
DK_TABLE(COMMA, "stST",
         0x219,0x21B, 0x218,0x21A)

static const dead_key_t s_dead_keys[] = {
    { DK_GRAVE + 1,  GRAVE_BASES,  GRAVE_CPS },
    { DK_ACUTE + 1,  ACUTE_BASES,  ACUTE_CPS },
    { DK_CIRC + 1,   CIRC_BASES,   CIRC_CPS },
    { DK_TILDE + 1,  TILDE_BASES,  TILDE_CPS },
    { DK_DIAER + 1,  DIAER_BASES,  DIAER_CPS },
    { DK_CEDIL + 1,  CEDIL_BASES,  CEDIL_CPS },
    { DK_CARON + 1,  CARON_BASES,  CARON_CPS },
    { DK_BREVE + 1,  BREVE_BASES,  BREVE_CPS },
    { DK_DOT + 1,    DOT_BASES,    DOT_CPS },
    { DK_RING + 1,   RING_BASES,   RING_CPS },
    { DK_OGONEK + 1, OGONEK_BASES, OGONEK_CPS },
    { DK_DACUTE + 1, DACUTE_BASES, DACUTE_CPS },
    { DK_COMMA + 1,  COMMA_BASES,  COMMA_CPS },
};

/* ------------------------------------------------------------------ */
/* Public API                                                         */
/* ------------------------------------------------------------------ */

/* Pending dead key (a DK_* entry), or NULL. */
static const char *s_dead = NULL;

extern "C" kb_layout_id_t kb_layout_get(void)
{
    return s_layout;
}

extern "C" const char *kb_layout_long_name(kb_layout_id_t layout)
{
    if (layout >= KB_LAYOUT_COUNT) return "";
    return s_layouts[layout].long_name;
}

extern "C" kb_ime_t kb_layout_ime(void)
{
    if (s_layout >= KB_LAYOUT_COUNT) return KB_IME_NONE;
    return s_layouts[s_layout].ime;
}

extern "C" const char *kb_layout_name(kb_layout_id_t layout)
{
    if (layout >= KB_LAYOUT_COUNT) return "??";
    return s_layouts[layout].name;
}

extern "C" kb_layout_id_t kb_layout_next(void)
{
    s_dead = NULL;
    kb_layout_id_t next = s_layout;
    for (int i = 0; i < KB_LAYOUT_COUNT; i++) {
        next = (kb_layout_id_t)((next + 1) % KB_LAYOUT_COUNT);
        if (s_active_mask & (1ull << next)) {
            s_layout = next;
            break;
        }
    }
    return s_layout;
}

extern "C" bool kb_layout_is_active(kb_layout_id_t layout)
{
    if (layout >= KB_LAYOUT_COUNT) return false;
    return (s_active_mask & (1ull << layout)) != 0;
}

extern "C" int kb_layout_active_count(void)
{
    int count = 0;
    for (int i = 0; i < KB_LAYOUT_COUNT; i++) {
        if (s_active_mask & (1ull << i)) count++;
    }
    return count;
}

extern "C" bool kb_layout_set_active(kb_layout_id_t layout, bool active)
{
    if (layout >= KB_LAYOUT_COUNT) return false;
    uint64_t bit = 1ull << layout;

    if (active) {
        s_active_mask |= bit;
        return true;
    }

    /* Refuse to drop the last active layout -- the set must never be
     * empty, or Ctrl+L / Win+Space would have nothing to cycle to. */
    if ((s_active_mask & ~bit) == 0) return false;

    s_active_mask &= ~bit;
    if (s_layout == layout) {
        /* The currently selected layout just left the active set --
         * switch to the next one immediately. */
        kb_layout_next();
    }
    return true;
}

extern "C" uint64_t kb_layout_get_active_mask(void)
{
    return s_active_mask;
}

extern "C" void kb_layout_set_active_mask(uint64_t mask)
{
    mask &= KB_LAYOUT_COUNT >= 64 ? ~0ull : (1ull << KB_LAYOUT_COUNT) - 1;
    if (mask == 0) mask = default_active_mask();
    s_active_mask = mask;

    if (!(s_active_mask & (1ull << s_layout))) {
        for (int i = 0; i < KB_LAYOUT_COUNT; i++) {
            if (s_active_mask & (1ull << i)) {
                s_layout = (kb_layout_id_t)i;
                break;
            }
        }
    }
}

extern "C" bool kb_layout_is_rtl(void)
{
#ifdef CONFIG_KB_LAYOUT_ENABLE_HE
    return s_layout == KB_LAYOUT_HE;
#else
    return false;
#endif
}

static volatile bool s_caps_lock = false;

extern "C" void kb_layout_set_caps_lock(bool on) { s_caps_lock = on; }
extern "C" bool kb_layout_get_caps_lock(void)    { return s_caps_lock; }

/* True if the UTF-8 string is a single uppercase letter (one that has
 * a lowercase form): A-Z, Latin-1 and Latin Extended-A capitals,
 * Romanian S / T with comma below, Cyrillic capitals. */
static bool is_upper_letter(const char *s)
{
    if (!s) return false;
    const uint8_t *u = (const uint8_t *)s;
    uint32_t cp;
    if (u[0] < 0x80) {
        if (u[1]) return false;
        cp = u[0];
    } else if ((u[0] & 0xE0) == 0xC0 && (u[1] & 0xC0) == 0x80 && !u[2]) {
        cp = ((uint32_t)(u[0] & 0x1F) << 6) | (u[1] & 0x3F);
    } else {
        return false;
    }
    if (cp >= 'A' && cp <= 'Z') return true;
    if (cp >= 0xC0 && cp <= 0xDE && cp != 0xD7) return true;   /* Latin-1 */
    /* Latin Extended-A: capitals are even in U+0100..U+0137 and
     * U+014A..U+0177, odd in U+0139..U+0148 and U+0179..U+017E. */
    if (cp >= 0x100 && cp <= 0x137) return !(cp & 1);
    if (cp >= 0x139 && cp <= 0x148) return (cp & 1);
    if (cp >= 0x14A && cp <= 0x177) return !(cp & 1);
    if (cp == 0x178) return true;                              /* Y diaeresis */
    if (cp >= 0x179 && cp <= 0x17E) return (cp & 1);
    if (cp == 0x218 || cp == 0x21A) return true;               /* Romanian */
    if (cp >= 0x400 && cp <= 0x42F) return true;               /* Cyrillic */
    if (cp >= 0x460 && cp <= 0x4FF && !(cp & 1)) return true;  /* e.g. Ukrainian G with upturn */
    return false;
}

/* Output of a key on one layer, or NULL. */
static const char *key_lookup(const layout_table_t *lt, uint8_t keycode, int layer)
{
    int idx = keycode - KC_A;
    if (layer == L_NORMAL && idx < TABLE_KEYS) return lt->normal[idx];
    if (layer == L_SHIFT && idx < TABLE_KEYS) return lt->shift[idx];
    for (const key_entry_t *e = lt->extra; e && e->key; e++) {
        if (e->key == keycode && e->layer == layer) return e->str;
    }
    return NULL;
}

static const dead_key_t *dead_key_info(const char *dk)
{
    for (const dead_key_t &d : s_dead_keys) {
        if (strcmp(dk + 1, d.spacing) == 0) return &d;
    }
    return NULL;
}

/* Applies a pending dead key to the next key's output. */
static const char *apply_dead_key(const char *dk, const char *next)
{
    static char buf[16];
    const dead_key_t *d = dead_key_info(dk);
    const char *spacing = d ? d->spacing : "";

    /* Space or the same dead key again: the accent alone. */
    if (strcmp(next, " ") == 0 || strcmp(next, dk) == 0) return spacing;

    /* Dotless i (Turkish, Crimean Tatar) takes the accents of i:
     * acute + dotless i = i-acute, as on the Crimean Tatar layout. */
    char base = (next[0] && !next[1]) ? next[0]
              : (strcmp(next, "\xC4\xB1") == 0) ? 'i' : 0;

    if (d && base) {
        const char *p = strchr(d->bases, base);
        if (p) {
            uint16_t cp = d->cps[p - d->bases];   /* < U+0800 */
            buf[0] = (char)(0xC0 | (cp >> 6));
            buf[1] = (char)(0x80 | (cp & 0x3F));
            buf[2] = '\0';
            return buf;
        }
    }

    /* No combination: the accent, then the key. A second dead key
     * stays pending instead. */
    if (next[0] == '\x01') {
        s_dead = next;
        return spacing;
    }
    snprintf(buf, sizeof(buf), "%s%s", spacing, next);
    return buf;
}

extern "C" bool kb_layout_has_altgr(void)
{
    for (const key_entry_t *e = s_layouts[s_layout].extra; e && e->key; e++) {
        if (e->layer == L_ALTGR || e->layer == L_ALTGR_SHIFT) return true;
    }
    return false;
}

extern "C" bool kb_layout_dead_key_pending(void)
{
    return s_dead != NULL;
}

extern "C" const char *kb_layout_translate(uint8_t keycode, uint8_t modifier)
{
    if (keycode < KC_A) return NULL;

    const layout_table_t *lt = &s_layouts[s_layout];
    bool shift = (modifier & (MOD_LSHIFT | MOD_RSHIFT)) != 0;
    bool altgr = (modifier & MOD_RALT) != 0;

    const char *result = NULL;

    if (altgr) {
        /* Caps Lock applies to AltGr letters too (Polish AltGr + a). */
        bool s = shift;
        if (s_caps_lock && is_upper_letter(key_lookup(lt, keycode, L_ALTGR_SHIFT)))
            s = !s;
        if (s) result = key_lookup(lt, keycode, L_ALTGR_SHIFT);
        if (!result) result = key_lookup(lt, keycode, L_ALTGR);
    }
    if (!result && s_caps_lock && !shift) result = key_lookup(lt, keycode, L_CAPS);
    if (!result) {
        if (s_caps_lock && is_upper_letter(key_lookup(lt, keycode, L_SHIFT)))
            shift = !shift;
        result = key_lookup(lt, keycode, shift ? L_SHIFT : L_NORMAL);
    }

    if (s_dead) {
        const char *dk = s_dead;
        s_dead = NULL;
        /* A key with no character (Enter, Tab, ...) drops the accent. */
        if (!result) return NULL;
        return apply_dead_key(dk, result);
    }
    if (result && result[0] == '\x01') {
        s_dead = result;
        return NULL;
    }
    return result;
}

extern "C" char kb_layout_shortcut_char(uint8_t keycode)
{
    if (keycode < 0x04) return 0;

    if (s_layouts[s_layout].is_latin) {
        /* Latin layout: resolve to whichever letter this physical key
         * actually produces on the national layout's normal
         * (unshifted, no AltGr) layer. Deliberately NOT restricted to
         * the classic 26-key US letter block (0x04..0x1D) -- a layout
         * can move a Latin letter onto a key that is a symbol on US.
         * French AZERTY, for example, puts "m" on the semicolon key
         * (0x33) and a comma where US has "m", so Ctrl+M on a French
         * keyboard is that semicolon-position key, not the US M
         * position. A key producing a non-letter here (a genuine
         * symbol under this layout) has no Ctrl-shortcut letter. */
        const layout_table_t *lt = &s_layouts[s_layout];
        int idx = keycode - KC_A;
        const char *t = (idx < TABLE_KEYS) ? lt->normal[idx] : NULL;
        /* Turkish: the dotless-i key is the I key for shortcuts. */
        if (t && strcmp(t, "\xC4\xB1") == 0) return 'i';
        if (t && t[0] && !t[1]) {
            if (t[0] >= 'a' && t[0] <= 'z') return t[0];
            if (t[0] >= 'A' && t[0] <= 'Z') return (char)(t[0] - 'A' + 'a');
        }
        return 0;
    }

    /* Non-Latin layout (Ukrainian, Hebrew, ...): fall back to the
     * classic 26-key US physical letter block, since there is no
     * national Latin letter to remap a shortcut to (e.g. Ctrl+N keeps
     * working exactly as if the US layout were active). */
    if (keycode > 0x1D) return 0;
    return (char)('a' + (keycode - 0x04));
}
