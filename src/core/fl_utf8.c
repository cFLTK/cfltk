/*
 * cfltk - fl_utf8.c
 * See include/cfltk/fl_utf8.h for scope notes.
 * Translated from src/fl_utf.c (fl_utf8decode/fl_utf8encode; ERRORS_TO_CP1252
 * default, STRICT_RFC3629 off, matching upstream's own defaults) and the
 * fl_utf8len1 portion of src/fl_utf8.cxx.
 */
#include "cfltk/fl_utf8.h"

int fl_utf8len1(char c) {
    if (!(c & 0x80)) return 1;
    if (c & 0x40) {
        if (c & 0x20) {
            if (c & 0x10) return 4;
            return 3;
        }
        return 2;
    }
    return 1;
}

/* Microsoft CP1252 characters for the 0x80-0x9f range, which is where
 * CP1252 and ISO-8859-1 disagree (used as the fallback decoding for a
 * byte that isn't a valid UTF-8 continuation of anything). */
static const unsigned short cp1252[32] = {
    0x20ac, 0x0081, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021,
    0x02c6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008d, 0x017d, 0x008f,
    0x0090, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
    0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0x009d, 0x017e, 0x0178
};

unsigned fl_utf8decode(const char *p, const char *end, int *len) {
    unsigned char c = *(const unsigned char *)p;
    if (c < 0x80) {
        if (len) *len = 1;
        return c;
    } else if (c < 0xa0) {
        if (len) *len = 1;
        return cp1252[c - 0x80];
    } else if (c < 0xc2) {
        goto FAIL;
    }
    if ((end && p + 1 >= end) || (p[1] & 0xc0) != 0x80) goto FAIL;
    if (c < 0xe0) {
        if (len) *len = 2;
        return ((unsigned)(p[0] & 0x1f) << 6) + (unsigned)(p[1] & 0x3f);
    } else if (c == 0xe0) {
        if (((const unsigned char *)p)[1] < 0xa0) goto FAIL;
        goto UTF8_3;
    } else if (c < 0xf0) {
    UTF8_3:
        if ((end && p + 2 >= end) || (p[2] & 0xc0) != 0x80) goto FAIL;
        if (len) *len = 3;
        return ((unsigned)(p[0] & 0x0f) << 12) + ((unsigned)(p[1] & 0x3f) << 6) + (unsigned)(p[2] & 0x3f);
    } else if (c == 0xf0) {
        if (((const unsigned char *)p)[1] < 0x90) goto FAIL;
        goto UTF8_4;
    } else if (c < 0xf4) {
    UTF8_4:
        if ((end && p + 3 >= end) || (p[2] & 0xc0) != 0x80 || (p[3] & 0xc0) != 0x80) goto FAIL;
        if (len) *len = 4;
        return ((unsigned)(p[0] & 0x07) << 18) + ((unsigned)(p[1] & 0x3f) << 12) +
               ((unsigned)(p[2] & 0x3f) << 6) + (unsigned)(p[3] & 0x3f);
    } else if (c == 0xf4) {
        if (((const unsigned char *)p)[1] > 0x8f) goto FAIL;
        goto UTF8_4;
    } else {
    FAIL:
        if (len) *len = 1;
        return 0xfffd;
    }
}

int fl_utf8encode(unsigned ucs, char *buf) {
    if (ucs < 0x000080U) {
        buf[0] = (char)ucs;
        return 1;
    } else if (ucs < 0x000800U) {
        buf[0] = (char)(0xc0 | (ucs >> 6));
        buf[1] = (char)(0x80 | (ucs & 0x3f));
        return 2;
    } else if (ucs < 0x010000U) {
        buf[0] = (char)(0xe0 | (ucs >> 12));
        buf[1] = (char)(0x80 | ((ucs >> 6) & 0x3f));
        buf[2] = (char)(0x80 | (ucs & 0x3f));
        return 3;
    } else if (ucs <= 0x0010ffffU) {
        buf[0] = (char)(0xf0 | (ucs >> 18));
        buf[1] = (char)(0x80 | ((ucs >> 12) & 0x3f));
        buf[2] = (char)(0x80 | ((ucs >> 6) & 0x3f));
        buf[3] = (char)(0x80 | (ucs & 0x3f));
        return 4;
    } else {
        buf[0] = (char)0xef;
        buf[1] = (char)0xbf;
        buf[2] = (char)0xbd;
        return 3;
    }
}

int fl_tolower(unsigned int ucs) {
    if (ucs >= 'A' && ucs <= 'Z') return (int)(ucs - 'A' + 'a');
    return (int)ucs;
}

int fl_toupper(unsigned int ucs) {
    if (ucs >= 'a' && ucs <= 'z') return (int)(ucs - 'a' + 'A');
    return (int)ucs;
}

/* Byte length (1-4) of the UTF-8 sequence starting with byte c, or -1
 * if c cannot start a valid sequence (continuation byte, or one of the
 * bytes 0xf5-0xff that can never appear in valid UTF-8). Distinct from
 * fl_utf8len1() above (which always returns >=1, for callers that just
 * need to advance past bad bytes) - this one needs to distinguish
 * "invalid" for fl_utf8test()'s strict validation. */
static int utf8_seq_len(unsigned char c) {
    if (c < 0x80) return 1;
    if (c < 0xc2) return -1;
    if (c < 0xe0) return 2;
    if (c < 0xf0) return 3;
    if (c < 0xf5) return 4;
    return -1;
}

int fl_utf8test(const char *src, unsigned int srclen) {
    unsigned int i = 0;
    int result = 1;

    while (i < srclen) {
        unsigned char c = (unsigned char)src[i];
        int seqlen = utf8_seq_len(c);
        unsigned int cp;
        int j;

        if (seqlen < 0) return 0;
        if (seqlen == 1) { i++; continue; }
        if (i + (unsigned int)seqlen > srclen) return 0;
        for (j = 1; j < seqlen; j++) {
            if (((unsigned char)src[i + j] & 0xc0) != 0x80) return 0;
        }

        cp = (unsigned int)(c & (0xff >> (seqlen + 1)));
        for (j = 1; j < seqlen; j++)
            cp = (cp << 6) | ((unsigned char)src[i + j] & 0x3f);

        /* reject overlong encodings, surrogate halves, and out-of-range */
        if ((seqlen == 2 && cp < 0x80) ||
            (seqlen == 3 && cp < 0x800) ||
            (seqlen == 4 && cp < 0x10000) ||
            (cp >= 0xd800 && cp <= 0xdfff) ||
            cp > 0x10ffff)
            return 0;

        if (cp >= 0x10000) {
            if (result < 4) result = 4;
        } else if (cp >= 0x800) {
            if (result < 3) result = 3;
        } else {
            if (result < 2) result = 2;
        }

        i += (unsigned int)seqlen;
    }
    return result;
}

int fl_utf_nb_char(const unsigned char *str, int len) {
    int i, n = 0;
    for (i = 0; i < len; i++) {
        if ((str[i] & 0xc0) != 0x80) n++;
    }
    return n;
}
