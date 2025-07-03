/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms,
 * with or without modification, are permitted provided that the copyright
 * notice and this permission notice appear in all copies. This software is
 * provided "as is" without warranty.
 *
 * Purpose: Implements basic visual encoding helpers for vlibc.
 */

#include "vis.h"
#include "string.h"
#include <ctype.h>
#include <stdio.h>

/* Return non-zero if character requires octal escaping */
/* simple printable check independent of ctype table */
static int is_print(int c)
{
    return c >= 0x20 && c <= 0x7e;
}

static int need_octal(int c, int flag)
{
    if (flag & VIS_OCTAL)
        return 1;
    if (!is_print(c))
        return 1;
    if ((c == ' ' && (flag & (VIS_SP|VIS_WHITE))) ||
        (c == '\t' && (flag & (VIS_TAB|VIS_WHITE))) ||
        (c == '\n' && (flag & (VIS_NL|VIS_WHITE))))
        return 1;
    return 0;
}

/*
 * nvis() - encode a character using visual escapes.
 * Writes the representation of 'c' into 'dst' respecting
 * the destination length and returns the number of bytes
 * written or -1 on overflow.
 */
static int isoctal(int c)
{
    return c >= '0' && c <= '7';
}

int nvis(char *dst, size_t dlen, int c, int flag, int nextc)
{
    char *start = dst;
    unsigned char ch = (unsigned char)c;

    if (!need_octal(ch, flag)) {
        if (ch == '\\' && !(flag & VIS_NOSLASH)) {
            if (dlen < 2 + 1)
                return -1;
            *dst++ = '\\';
            *dst++ = '\\';
        } else {
            if (dlen < 1 + 1)
                return -1;
            *dst++ = ch;
        }
        *dst = '\0';
        return dst - start;
    }

    if (flag & VIS_CSTYLE) {
        switch (ch) {
        case '\n':
            if (dlen < 2 + 1)
                return -1;
            *dst++ = '\\';
            *dst++ = 'n';
            *dst = '\0';
            return dst - start;
        case '\r':
            if (dlen < 2 + 1)
                return -1;
            *dst++ = '\\';
            *dst++ = 'r';
            *dst = '\0';
            return dst - start;
        case '\b':
            if (dlen < 2 + 1)
                return -1;
            *dst++ = '\\';
            *dst++ = 'b';
            *dst = '\0';
            return dst - start;
        case '\a':
            if (dlen < 2 + 1)
                return -1;
            *dst++ = '\\';
            *dst++ = 'a';
            *dst = '\0';
            return dst - start;
        case '\v':
            if (dlen < 2 + 1)
                return -1;
            *dst++ = '\\';
            *dst++ = 'v';
            *dst = '\0';
            return dst - start;
        case '\t':
            if (dlen < 2 + 1)
                return -1;
            *dst++ = '\\';
            *dst++ = 't';
            *dst = '\0';
            return dst - start;
        case '\f':
            if (dlen < 2 + 1)
                return -1;
            *dst++ = '\\';
            *dst++ = 'f';
            *dst = '\0';
            return dst - start;
        case '\0':
            if (dlen < (isoctal(nextc) ? 4 : 2) + 1)
                return -1;
            *dst++ = '\\';
            *dst++ = '0';
            if (isoctal(nextc)) {
                *dst++ = '0';
                *dst++ = '0';
            }
            *dst = '\0';
            return dst - start;
        case '\\':
            if (dlen < 2 + 1)
                return -1;
            *dst++ = '\\';
            *dst++ = '\\';
            *dst = '\0';
            return dst - start;
        default:
            if (dlen < 4 + 1)
                return -1;
            snprintf(dst, dlen, "\\%03o", ch);
            return strlen(dst);
        }
    }

    if (dlen < 4 + 1)
        return -1;
    if (!(flag & VIS_NOSLASH))
        *dst++ = '\\';
    *dst++ = ((ch >> 6) & 07) + '0';
    *dst++ = ((ch >> 3) & 07) + '0';
    *dst++ = (ch & 07) + '0';
    *dst = '\0';
    return dst - start;
}

/*
 * vis() - convenience wrapper around nvis() without a
 * destination length limit.
 */
int vis(char *dst, int c, int flag, int nextc)
{
    return nvis(dst, (size_t)-1, c, flag, nextc);
}

/* simple state machine for decoding */
#define S_GROUND 0
#define S_ESCAPE 1
#define S_OCT1   2
#define S_OCT2   3

/*
 * unvis() - decode a byte stream produced by vis.
 * Maintains state across calls and returns UNVIS_* codes
 * describing the result of each step.
 */
int unvis(char *cp, int c, int *state, int flag)
{
    unsigned char ch = (unsigned char)c;
    int st = *state & 0xff;
    int val = (*state >> 8) & 0xff;
    (void)flag;

    switch (st) {
    case S_GROUND:
        if (ch == '\\') {
            *state = S_ESCAPE;
            return UNVIS_NOCHAR;
        }
        *cp = ch;
        return UNVIS_VALID;

    case S_ESCAPE:
        if (ch >= '0' && ch <= '7') {
            val = ch - '0';
            *state = (val << 8) | S_OCT1;
            return UNVIS_NOCHAR;
        }
        *state = S_GROUND;
        switch (ch) {
        case 'n': *cp = '\n'; return UNVIS_VALID;
        case 'r': *cp = '\r'; return UNVIS_VALID;
        case 'b': *cp = '\b'; return UNVIS_VALID;
        case 'a': *cp = '\a'; return UNVIS_VALID;
        case 'v': *cp = '\v'; return UNVIS_VALID;
        case 't': *cp = '\t'; return UNVIS_VALID;
        case 'f': *cp = '\f'; return UNVIS_VALID;
        case '\\': *cp = '\\'; return UNVIS_VALID;
        default:
            *cp = ch;
            return UNVIS_VALID;
        }

    case S_OCT1:
        if (ch >= '0' && ch <= '7') {
            val = (val << 3) + (ch - '0');
            *state = (val << 8) | S_OCT2;
            return UNVIS_NOCHAR;
        }
        /* FALLTHROUGH */
    case S_OCT2:
        *state = S_GROUND;
        *cp = val;
        return UNVIS_VALID;
    }

    *state = S_GROUND;
    return UNVIS_SYNBAD;
}
