/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements wide memory routines for vlibc.
 */

#include "wchar.h"

/* Fill n wide characters of s with c. */
wchar_t *wmemset(wchar_t *s, wchar_t c, size_t n)
{
    wchar_t *p = s;
    while (n--)
        *p++ = c;
    return s;
}

/* Copy n wide characters from src to dest. */
wchar_t *wmemcpy(wchar_t *dest, const wchar_t *src, size_t n)
{
    wchar_t *d = dest;
    const wchar_t *s2 = src;
    while (n--)
        *d++ = *s2++;
    return dest;
}

/* Move n wide characters from src to dest with overlap handling. */
wchar_t *wmemmove(wchar_t *dest, const wchar_t *src, size_t n)
{
    wchar_t *d = dest;
    const wchar_t *s2 = src;
    if (d == s2 || n == 0)
        return dest;
    if (d < s2) {
        while (n--)
            *d++ = *s2++;
    } else {
        d += n;
        s2 += n;
        while (n--)
            *--d = *--s2;
    }
    return dest;
}

/* Compare two wide character arrays. */
int wmemcmp(const wchar_t *s1, const wchar_t *s2, size_t n)
{
    while (n--) {
        if (*s1 != *s2)
            return (int)(*s1 - *s2);
        s1++; s2++;
    }
    return 0;
}
