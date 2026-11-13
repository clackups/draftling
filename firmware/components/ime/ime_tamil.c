/* Helpers shared by the Tamil input methods (see ime_tamil.h). */

#include <string.h>
#include "ime_tamil.h"

const uint16_t TA_VOWEL_LETTER[TA_VOWELS] = {
    0x0B85, 0x0B86, 0x0B87, 0x0B88, 0x0B89, 0x0B8A,
    0x0B8E, 0x0B8F, 0x0B90, 0x0B92, 0x0B93, 0x0B94,
};

const uint16_t TA_VOWEL_SIGN[TA_VOWELS] = {
    0, TA_SIGN_AA, TA_SIGN_I, TA_SIGN_II, TA_SIGN_U, TA_SIGN_UU,
    TA_SIGN_E, TA_SIGN_EE, TA_SIGN_AI, TA_SIGN_O, TA_SIGN_OO, TA_SIGN_AU,
};

bool ta_is_consonant(uint32_t cp)
{
    switch (cp) {
    case 0x0B95: case 0x0B99: case 0x0B9A: case 0x0B9C: case 0x0B9E:
    case 0x0B9F: case 0x0BA3: case 0x0BA4: case 0x0BA8: case 0x0BA9:
    case 0x0BAA: case 0x0BAE: case 0x0BAF: case 0x0BB0: case 0x0BB1:
    case 0x0BB2: case 0x0BB3: case 0x0BB4: case 0x0BB5: case 0x0BB6:
    case 0x0BB7: case 0x0BB8: case 0x0BB9:
        return true;
    default:
        return false;
    }
}

size_t ta_peek(uint32_t *cps, uint8_t *bytes, size_t max)
{
    char buf[48];
    size_t len = ime_peek(buf, sizeof(buf));
    /* Walk back over whole code points. */
    uint32_t rev_cp[16];
    uint8_t rev_len[16];
    size_t n = 0;
    while (len > 0 && n < max && n < 16) {
        size_t l = utf8_last_len(buf, len);
        size_t used;
        rev_cp[n] = utf8_decode(buf + len - l, l, &used);
        rev_len[n] = (uint8_t)l;
        n++;
        len -= l;
    }
    for (size_t i = 0; i < n; i++) {
        cps[i] = rev_cp[n - 1 - i];
        if (bytes) bytes[i] = rev_len[n - 1 - i];
    }
    return n;
}

void ta_encode(const uint32_t *cps, size_t n, char *out, size_t size)
{
    size_t pos = 0;
    for (size_t i = 0; i < n; i++) {
        char tmp[4];
        size_t l = utf8_encode(cps[i], tmp);
        if (pos + l + 1 > size) break;
        memcpy(out + pos, tmp, l);
        pos += l;
    }
    if (size) out[pos] = '\0';
}

bool ta_put(size_t del, const uint32_t *cps, size_t n)
{
    char out[64];
    ta_encode(cps, n, out, sizeof(out));
    return ime_replace(del, out);
}

char ta_key_char(const char *text)
{
    if (!text || !text[0] || text[1]) return 0;
    unsigned char c = (unsigned char)text[0];
    return (c > 0x20 && c < 0x7F) ? (char)c : 0;
}
