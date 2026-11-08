/*
 * Keyboard layout translation module.
 *
 * Translates HID keycodes + modifier flags into UTF-8 strings
 * for US-English, Ukrainian, German, French, Hebrew, Swiss German,
 * Swiss French, Italian, Portuguese, Spanish, United Kingdom and
 * Korean layouts. Vietnamese, Japanese and Chinese type on the US layout
 * and are composed by an input method (components/ime/).
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
 * input-method layout (VI, KO, JA, ZH) of a pack, else the first
 * compiled-in layout so the set is never empty. */
static uint32_t default_active_mask(void)
{
    uint32_t mask = 0;
#ifdef CONFIG_KB_LAYOUT_ENABLE_VI
    mask |= (1u << KB_LAYOUT_VI);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_KO
    mask |= (1u << KB_LAYOUT_KO);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_JA
    mask |= (1u << KB_LAYOUT_JA);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_ZH
    mask |= (1u << KB_LAYOUT_ZH);
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_US
    mask |= (1u << KB_LAYOUT_US);
#endif
    if (mask == 0) mask = 1u;
    return mask;
}

static uint32_t s_active_mask = default_active_mask();

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

static const char *DE_SHIFT[] = {
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

static const char *FR_SHIFT[] = {
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

#ifdef CONFIG_KB_LAYOUT_ENABLE_KO
/* ------------------------------------------------------------------ */
/* Korean 2-set (Dubeolsik) layout                                    */
/* ------------------------------------------------------------------ */

/* Letter keys produce Hangul Compatibility Jamo (U+3131..U+3163); the
 * Hangul input method composes them into syllables. Shift only
 * changes the five keys with a tense / extra form (Q W E R T -> the
 * double consonants, O P -> yae / ye); digits and symbols are US. */
static const char *KO_NORMAL[] = {
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

static const char *KO_SHIFT[] = {
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
static const char *DE_CH_NORMAL[] = {
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

static const char *DE_CH_SHIFT[] = {
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
static const char *FR_CH_NORMAL[] = {
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

static const char *FR_CH_SHIFT[] = {
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
static const char *IT_NORMAL[] = {
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

static const char *IT_SHIFT[] = {
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
static const char *PT_NORMAL[] = {
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

static const char *PT_SHIFT[] = {
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
static const char *ES_NORMAL[] = {
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

static const char *ES_SHIFT[] = {
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
static const char *GB_NORMAL[] = {
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

static const char *GB_SHIFT[] = {
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
static const char *SE_FI_NORMAL[] = {
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

static const char *SE_FI_SHIFT[] = {
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
static const char *NO_NORMAL[] = {
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

static const char *NO_SHIFT[] = {
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
static const char *DK_NORMAL[] = {
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

static const char *DK_SHIFT[] = {
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
static const char *CZ_NORMAL[] = {
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

static const char *CZ_SHIFT[] = {
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
static const char *SK_NORMAL[] = {
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

static const char *SK_SHIFT[] = {
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
static const char *HU_NORMAL[] = {
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

static const char *HU_SHIFT[] = {
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
static const char *RO_NORMAL[] = {
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

static const char *RO_SHIFT[] = {
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
static const char *HR_NORMAL[] = {
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

static const char *HR_SHIFT[] = {
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
static const char *TR_NORMAL[] = {
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

static const char *TR_SHIFT[] = {
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

/* ------------------------------------------------------------------ */
/* Layout table structure                                             */
/* ------------------------------------------------------------------ */

/* is_latin: the normal (unshifted) layer produces plain Latin letters
 * -- used by kb_layout_shortcut_char() below to decide whether
 * Ctrl-shortcut letters should follow the national layout (Latin) or
 * fall back to the US physical key position (non-Latin scripts, which
 * have no national Latin letter to remap a shortcut to). */
typedef struct {
    const char **normal;       /* keycodes 0x04..0x38 */
    const char **shift;        /* keycodes 0x04..0x38 */
    const key_entry_t *extra;  /* AltGr layers, keys past 0x38; or NULL */
    const char *name;
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
#ifdef CONFIG_KB_LAYOUT_ENABLE_KO
CHECK_TABLE(KO_NORMAL); CHECK_TABLE(KO_SHIFT);
#endif

/* The active set is a 32-bit mask (persisted in NVS). */
static_assert(KB_LAYOUT_COUNT <= 32, "too many layouts for the active mask");

/* In kb_layout_id_t order (C++ needs designators in order). */
static const layout_table_t s_layouts[KB_LAYOUT_COUNT] = {
#ifdef CONFIG_KB_LAYOUT_ENABLE_US
    [KB_LAYOUT_US] = { US_NORMAL, US_SHIFT, NULL, "US", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_UA
    [KB_LAYOUT_UA] = { UA_NORMAL, UA_SHIFT, NULL, "UA", KB_IME_NONE, false },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_DE
    [KB_LAYOUT_DE] = { DE_NORMAL, DE_SHIFT, DE_EXTRA, "DE", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_FR
    [KB_LAYOUT_FR] = { FR_NORMAL, FR_SHIFT, FR_EXTRA, "FR", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_HE
    [KB_LAYOUT_HE] = { HE_NORMAL, HE_SHIFT, NULL, "HE", KB_IME_NONE, false },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_DE_CH
    [KB_LAYOUT_DE_CH] = { DE_CH_NORMAL, DE_CH_SHIFT, DE_CH_EXTRA, "DE-CH", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_FR_CH
    [KB_LAYOUT_FR_CH] = { FR_CH_NORMAL, FR_CH_SHIFT, FR_CH_EXTRA, "FR-CH", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_IT
    [KB_LAYOUT_IT] = { IT_NORMAL, IT_SHIFT, IT_EXTRA, "IT", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_PT
    [KB_LAYOUT_PT] = { PT_NORMAL, PT_SHIFT, PT_EXTRA, "PT", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_ES
    [KB_LAYOUT_ES] = { ES_NORMAL, ES_SHIFT, ES_EXTRA, "ES", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_GB
    [KB_LAYOUT_GB] = { GB_NORMAL, GB_SHIFT, GB_EXTRA, "GB", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_SE_FI
    [KB_LAYOUT_SE_FI] = { SE_FI_NORMAL, SE_FI_SHIFT, SE_FI_EXTRA, "SE/FI", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_NO
    [KB_LAYOUT_NO] = { NO_NORMAL, NO_SHIFT, NO_EXTRA, "NO", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_DK
    [KB_LAYOUT_DK] = { DK_NORMAL, DK_SHIFT, DK_EXTRA, "DK", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_PL
    [KB_LAYOUT_PL] = { US_NORMAL, US_SHIFT, PL_EXTRA, "PL", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_CZ
    [KB_LAYOUT_CZ] = { CZ_NORMAL, CZ_SHIFT, CZ_EXTRA, "CZ", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_SK
    [KB_LAYOUT_SK] = { SK_NORMAL, SK_SHIFT, SK_EXTRA, "SK", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_HU
    [KB_LAYOUT_HU] = { HU_NORMAL, HU_SHIFT, HU_EXTRA, "HU", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_RO
    [KB_LAYOUT_RO] = { RO_NORMAL, RO_SHIFT, RO_EXTRA, "RO", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_HR
    [KB_LAYOUT_HR] = { HR_NORMAL, HR_SHIFT, HR_EXTRA, "HR/SI", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_TR
    [KB_LAYOUT_TR] = { TR_NORMAL, TR_SHIFT, TR_EXTRA, "TR", KB_IME_NONE, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_VI
    [KB_LAYOUT_VI] = { US_NORMAL, US_SHIFT, NULL, "VI", KB_IME_TELEX, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_KO
    [KB_LAYOUT_KO] = { KO_NORMAL, KO_SHIFT, NULL, "KO", KB_IME_HANGUL, false },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_JA
    [KB_LAYOUT_JA] = { US_NORMAL, US_SHIFT, NULL, "JA", KB_IME_KANA, true },
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_ZH
    [KB_LAYOUT_ZH] = { US_NORMAL, US_SHIFT, NULL, "ZH", KB_IME_PINYIN, true },
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
DK_TABLE(BREVE, "aguAGU",
         0x103,0x11F,0x16D, 0x102,0x11E,0x16C)
DK_TABLE(DOT, "zecgIZECG",
         0x17C,0x117,0x10B,0x121, 0x130, 0x17B,0x116,0x10A,0x120)
DK_TABLE(RING, "auAU",
         0xE5,0x16F, 0xC5,0x16E)
DK_TABLE(OGONEK, "aeiuAEIU",
         0x105,0x119,0x12F,0x173, 0x104,0x118,0x12E,0x172)
DK_TABLE(DACUTE, "ouOU",
         0x151,0x171, 0x150,0x170)

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
        if (s_active_mask & (1u << next)) {
            s_layout = next;
            break;
        }
    }
    return s_layout;
}

extern "C" bool kb_layout_is_active(kb_layout_id_t layout)
{
    if (layout >= KB_LAYOUT_COUNT) return false;
    return (s_active_mask & (1u << layout)) != 0;
}

extern "C" int kb_layout_active_count(void)
{
    int count = 0;
    for (int i = 0; i < KB_LAYOUT_COUNT; i++) {
        if (s_active_mask & (1u << i)) count++;
    }
    return count;
}

extern "C" bool kb_layout_set_active(kb_layout_id_t layout, bool active)
{
    if (layout >= KB_LAYOUT_COUNT) return false;
    uint32_t bit = 1u << layout;

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

extern "C" uint32_t kb_layout_get_active_mask(void)
{
    return s_active_mask;
}

extern "C" void kb_layout_set_active_mask(uint32_t mask)
{
    mask &= (1u << KB_LAYOUT_COUNT) - 1;
    if (mask == 0) mask = default_active_mask();
    s_active_mask = mask;

    if (!(s_active_mask & (1u << s_layout))) {
        for (int i = 0; i < KB_LAYOUT_COUNT; i++) {
            if (s_active_mask & (1u << i)) {
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

    if (d && next[0] && !next[1]) {
        const char *p = strchr(d->bases, next[0]);
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
