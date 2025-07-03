/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the wchar functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "wchar.h"
#include "memory.h"
#include "string.h"
#include "locale.h"

/* Convert multibyte sequence to wide character. */
int mbtowc(wchar_t *pwc, const char *s, size_t n)
{
    if (!s)
        return 0;
    if (n == 0)
        return -1;
    if (pwc)
        *pwc = (unsigned char)*s;
    return *s ? 1 : 0;
}

/* Convert wide character to multibyte sequence. */
int wctomb(char *s, wchar_t wc)
{
    if (!s)
        return 0;
    if (wc < 0 || wc > 0xFF)
        return -1;
    *s = (char)wc;
    return 1;
}

/* Return length in bytes of next multibyte character. */
int mblen(const char *s, size_t n)
{
    if (!s)
        return 0;
    if (n == 0)
        return -1;
    unsigned char c = (unsigned char)*s;
    if (c < 0x80)
        return c ? 1 : 0;
#if defined(__FreeBSD__) || defined(__NetBSD__) ||  \
    defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_mblen(const char *, size_t) __asm("mblen");
    return host_mblen(s, n);
#else
    (void)n;
    return -1;
#endif
}


/* Return the length of wide-character string. */
size_t wcslen(const wchar_t *s)
{
    const wchar_t *p = s;
    while (*p)
        p++;
    return (size_t)(p - s);
}

/* Return column width of wide character or -1 if non-printable. */
int wcwidth(wchar_t wc)
{
    if (wc == 0)
        return 0;
    if (wc >= 0x20 && wc < 0x7f)
        return 1;
    if (wc < 0x20 || wc == 0x7f)
        return -1;
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_wcwidth(wchar_t) __asm("wcwidth");
    return host_wcwidth(wc);
#else
    (void)wc;
    return -1;
#endif
}

/* Sum of column widths of up to n wide characters. */
int wcswidth(const wchar_t *s, size_t n)
{
    int width = 0;
    for (size_t i = 0; i < n && s[i]; i++) {
        int w = wcwidth(s[i]);
        if (w < 0)
            return -1;
        width += w;
    }
    return width;
}

/* Copy a wide-character string. */
wchar_t *wcscpy(wchar_t *dest, const wchar_t *src)
{
    wchar_t *d = dest;
    while ((*d++ = *src++) != 0)
        ;
    return dest;
}

/* Copy at most n wide characters, padding with zeros. */
wchar_t *wcsncpy(wchar_t *dest, const wchar_t *src, size_t n)
{
    wchar_t *d = dest;
    while (n && *src) {
        *d++ = *src++;
        --n;
    }
    while (n--) {
        *d++ = 0;
    }
    return dest;
}

/* Compare two wide-character strings. */
int wcscmp(const wchar_t *s1, const wchar_t *s2)
{
    while (*s1 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return (int)(*s1 - *s2);
}

/* Compare up to n wide characters. */
int wcsncmp(const wchar_t *s1, const wchar_t *s2, size_t n)
{
    while (n--) {
        wchar_t c1 = *s1++;
        wchar_t c2 = *s2++;
        if (c1 != c2)
            return (int)(c1 - c2);
        if (c1 == 0)
            break;
    }
    return 0;
}

/* Duplicate a wide-character string. */
wchar_t *wcsdup(const wchar_t *s)
{
    size_t len = wcslen(s);
    wchar_t *dup = malloc((len + 1) * sizeof(wchar_t));
    if (!dup)
        return NULL;
    wcscpy(dup, s);
    return dup;
}

/* Collate two wide-character strings. */
int wcscoll(const wchar_t *s1, const wchar_t *s2)
{
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    const char *loc = setlocale(LC_COLLATE, NULL);
    if (loc && strcmp(loc, "C") != 0 && strcmp(loc, "POSIX") != 0) {
        extern int host_wcscoll(const wchar_t *, const wchar_t *) __asm("wcscoll");
        return host_wcscoll(s1, s2);
    }
#endif
    return wcscmp(s1, s2);
}

/* Transform a wide-character string for collation. */
size_t wcsxfrm(wchar_t *dest, const wchar_t *src, size_t n)
{
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    const char *loc = setlocale(LC_COLLATE, NULL);
    if (loc && strcmp(loc, "C") != 0 && strcmp(loc, "POSIX") != 0) {
        extern size_t host_wcsxfrm(wchar_t *, const wchar_t *, size_t) __asm("wcsxfrm");
        return host_wcsxfrm(dest, src, n);
    }
#endif
    size_t len = wcslen(src);
    if (n) {
        size_t copy = len >= n ? n - 1 : len;
        if (dest) {
            wmemcpy(dest, src, copy);
            dest[copy] = L'\0';
        }
    }
    return len;
}

/* Helper to test if a character is in the delimiter set. */
static int is_wdelim(wchar_t c, const wchar_t *delim)
{
    for (const wchar_t *d = delim; *d; ++d) {
        if (*d == c)
            return 1;
    }
    return 0;
}

/* Tokenize a wide-character string similar to strtok_r. */
wchar_t *wcstok(wchar_t *str, const wchar_t *delim, wchar_t **saveptr)
{
    wchar_t *s;

    if (str)
        s = str;
    else if (saveptr && *saveptr)
        s = *saveptr;
    else
        return NULL;

    /* skip leading delimiters */
    while (*s && is_wdelim(*s, delim))
        s++;

    if (*s == L'\0') {
        if (saveptr)
            *saveptr = NULL;
        return NULL;
    }

    wchar_t *token = s;

    while (*s && !is_wdelim(*s, delim))
        s++;

    if (*s) {
        *s = L'\0';
        s++;
        if (saveptr)
            *saveptr = s;
    } else if (saveptr) {
        *saveptr = NULL;
    }

    return token;
}

/* Locate first occurrence of c in wide string s. */
wchar_t *wcschr(const wchar_t *s, wchar_t c)
{
    while (*s) {
        if (*s == c)
            return (wchar_t *)s;
        s++;
    }
    if (c == L'\0')
        return (wchar_t *)s;
    return NULL;
}

/* Locate last occurrence of c in wide string s. */
wchar_t *wcsrchr(const wchar_t *s, wchar_t c)
{
    const wchar_t *ret = NULL;
    do {
        if (*s == c)
            ret = s;
    } while (*s++);
    return (wchar_t *)ret;
}

/* Find substring needle in haystack. */
wchar_t *wcsstr(const wchar_t *haystack, const wchar_t *needle)
{
    if (!*needle)
        return (wchar_t *)haystack;
    size_t nlen = wcslen(needle);
    while (*haystack) {
        if (*haystack == *needle && wcsncmp(haystack, needle, nlen) == 0)
            return (wchar_t *)haystack;
        haystack++;
    }
    return NULL;
}

/* Search first n wide characters of s for c. */
wchar_t *wmemchr(const wchar_t *s, wchar_t c, size_t n)
{
    const wchar_t *p = s;
    while (n--) {
        if (*p == c)
            return (wchar_t *)p;
        p++;
    }
    return NULL;
}
