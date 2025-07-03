/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the wchar_conv functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "wchar.h"
#include "string.h"
#include "stdlib.h"
#include "memory.h"
#include <errno.h>

extern size_t host_mbrtowc(wchar_t *, const char *, size_t, mbstate_t *) __asm__("mbrtowc");
extern size_t host_wcrtomb(char *, wchar_t, mbstate_t *) __asm__("wcrtomb");
extern size_t host_mbstowcs(wchar_t *, const char *, size_t) __asm__("mbstowcs");
extern size_t host_wcstombs(char *, const wchar_t *, size_t) __asm__("wcstombs");

/* Test whether the conversion state is in the initial state. */
int mbsinit(const mbstate_t *ps)
{
    (void)ps;
    return 1;
}

/* Return length in bytes of next multibyte character. */
size_t mbrlen(const char *s, size_t n, mbstate_t *ps)
{
    return mbrtowc(NULL, s, n, ps);
}

/* Convert multibyte sequence to wide char handling ASCII fast path. */
size_t mbrtowc(wchar_t *pwc, const char *s, size_t n, mbstate_t *ps)
{
    (void)ps;
    if (!s)
        return 0;
    if (n == 0)
        return (size_t)-2;
    unsigned char ch = (unsigned char)*s;
    if (ch < 0x80) {
        if (pwc)
            *pwc = (wchar_t)ch;
        return ch ? 1 : 0;
    }
    return host_mbrtowc(pwc, s, n, ps);
}

/* Convert wide char to multibyte sequence with ASCII optimisation. */
size_t wcrtomb(char *s, wchar_t wc, mbstate_t *ps)
{
    (void)ps;
    if (!s)
        return 1;
    if ((unsigned)wc < 0x80) {
        *s = (char)wc;
        return 1;
    }
    return host_wcrtomb(s, wc, ps);
}

/* Convert a single byte to a wide character */
wint_t btowc(int c)
{
    if (c == -1)
        return -1;
    wchar_t wc = 0;
    char b[2] = { (char)c, 0 };
    size_t r = mbrtowc(&wc, b, 1, NULL);
    if (r == (size_t)-1 || r == (size_t)-2)
        return -1;
    return wc;
}

/* Convert a wide character to a single byte */
int wctob(wchar_t wc)
{
    char buf[8];
    size_t r = wcrtomb(buf, wc, NULL);
    if (r == (size_t)-1 || r != 1)
        return -1;
    return (unsigned char)buf[0];
}


/* Check if multibyte string contains non-ASCII characters. */
static int has_non_ascii_mb(const char *s)
{
    while (*s) {
        if (((unsigned char)*s) >= 0x80)
            return 1;
        s++;
    }
    return 0;
}

/* Check if wide string contains non-ASCII characters. */
static int has_non_ascii_wc(const wchar_t *s)
{
    while (*s) {
        if ((unsigned)*s >= 0x80)
            return 1;
        s++;
    }
    return 0;
}

/* Convert a multibyte string to wide characters without iconv when possible. */
size_t mbstowcs(wchar_t *dst, const char *src, size_t n)
{
    if (has_non_ascii_mb(src))
        return host_mbstowcs(dst, src, n);

    size_t i = 0;
    if (dst) {
        for (; i < n && src[i]; i++)
            dst[i] = (unsigned char)src[i];
        if (i < n)
            dst[i] = 0;
    } else {
        while (src[i])
            i++;
    }
    return i;
}

/* Convert wide character string to multibyte without iconv when possible. */
size_t wcstombs(char *dst, const wchar_t *src, size_t n)
{
    if (has_non_ascii_wc(src))
        return host_wcstombs(dst, src, n);

    size_t i = 0;
    if (dst) {
        for (; i < n && src[i]; i++)
            dst[i] = (char)src[i];
        if (i < n)
            dst[i] = 0;
    } else {
        while (src[i])
            i++;
    }
    return i;
}

/* Stateful multibyte string to wide conversion. */
size_t mbsrtowcs(wchar_t *dst, const char **src, size_t n, mbstate_t *ps)
{
    if (!src)
        return 0;

    const char *s = *src;
    size_t count = 0;

    if (!dst)
        n = (size_t)-1;

    while (1) {
        wchar_t wc = 0;
        size_t r = mbrtowc(&wc, s, strlen(s) + 1, ps);
        if (r == (size_t)-1)
            return (size_t)-1;
        if (r == (size_t)-2) {
            *src = s;
            return count;
        }
        if (r == 0) {
            if (dst && count < n)
                dst[count] = 0;
            *src = NULL;
            return count;
        }
        if (count >= n) {
            *src = s;
            return count;
        }
        if (dst)
            dst[count] = wc;
        s += r;
        count++;
    }
}

/* Stateful wide string to multibyte conversion. */
size_t wcsrtombs(char *dst, const wchar_t **src, size_t n, mbstate_t *ps)
{
    if (!src)
        return 0;

    const wchar_t *s = *src;
    size_t count = 0;
    char buf[8];

    if (!dst)
        n = (size_t)-1;

    while (1) {
        size_t r = wcrtomb(buf, *s, ps);
        if (r == (size_t)-1)
            return (size_t)-1;
        if (*s == L'\0') {
            if (dst && count + r <= n)
                memcpy(dst + count, buf, r);
            *src = NULL;
            return count;
        }
        if (count + r > n) {
            *src = s;
            return count;
        }
        if (dst)
            memcpy(dst + count, buf, r);
        count += r;
        s++;
    }
}

/* Helper to convert a wide string to a multibyte buffer. */
static char *wcs_to_mb_tmp(const wchar_t *s, char *stack, size_t stack_sz, size_t *out_len)
{
    size_t len = wcstombs(NULL, s, 0);
    if (len == (size_t)-1) {
        errno = EILSEQ;
        *out_len = 0;
        return NULL;
    }
    *out_len = len;
    if (len < stack_sz) {
        wcstombs(stack, s, len + 1);
        return stack;
    }
    char *buf = malloc(len + 1);
    if (!buf) {
        *out_len = 0;
        return NULL;
    }
    wcstombs(buf, s, len + 1);
    return buf;
}

/*
 * wcstol() - convert a wide-character string to a long integer. A temporary
 * multibyte buffer is used so strtol can perform the heavy lifting.
 */
long wcstol(const wchar_t *nptr, wchar_t **endptr, int base)
{
    char stack[128];
    size_t len;
    char *mb = wcs_to_mb_tmp(nptr, stack, sizeof(stack), &len);
    if (!mb) {
        if (endptr)
            *endptr = (wchar_t *)nptr;
        return 0;
    }
    char *end;
    long v = strtol(mb, &end, base);
    if (endptr)
        *endptr = (wchar_t *)nptr + (end - mb);
    if (mb != stack)
        free(mb);
    return v;
}

/*
 * wcstoul() - convert a wide-character string to an unsigned long.
 * Uses a temporary multibyte buffer and strtoul.
 */
unsigned long wcstoul(const wchar_t *nptr, wchar_t **endptr, int base)
{
    char stack[128];
    size_t len;
    char *mb = wcs_to_mb_tmp(nptr, stack, sizeof(stack), &len);
    if (!mb) {
        if (endptr)
            *endptr = (wchar_t *)nptr;
        return 0;
    }
    char *end;
    unsigned long v = strtoul(mb, &end, base);
    if (endptr)
        *endptr = (wchar_t *)nptr + (end - mb);
    if (mb != stack)
        free(mb);
    return v;
}

/*
 * wcstoll() - convert a wide-character string to a long long. Utilises
 * strtoll via a temporary multibyte buffer.
 */
long long wcstoll(const wchar_t *nptr, wchar_t **endptr, int base)
{
    char stack[128];
    size_t len;
    char *mb = wcs_to_mb_tmp(nptr, stack, sizeof(stack), &len);
    if (!mb) {
        if (endptr)
            *endptr = (wchar_t *)nptr;
        return 0;
    }
    char *end;
    long long v = strtoll(mb, &end, base);
    if (endptr)
        *endptr = (wchar_t *)nptr + (end - mb);
    if (mb != stack)
        free(mb);
    return v;
}

/*
 * wcstoull() - convert a wide-character string to an unsigned long long.
 * Uses a temporary buffer and strtoull.
 */
unsigned long long wcstoull(const wchar_t *nptr, wchar_t **endptr, int base)
{
    char stack[128];
    size_t len;
    char *mb = wcs_to_mb_tmp(nptr, stack, sizeof(stack), &len);
    if (!mb) {
        if (endptr)
            *endptr = (wchar_t *)nptr;
        return 0;
    }
    char *end;
    unsigned long long v = strtoull(mb, &end, base);
    if (endptr)
        *endptr = (wchar_t *)nptr + (end - mb);
    if (mb != stack)
        free(mb);
    return v;
}

/*
 * wcstoimax() - convert a wide-character string to intmax_t.
 * A helper converts to multibyte then calls strtoimax.
 */
intmax_t wcstoimax(const wchar_t *nptr, wchar_t **endptr, int base)
{
    char stack[128];
    size_t len;
    char *mb = wcs_to_mb_tmp(nptr, stack, sizeof(stack), &len);
    if (!mb) {
        if (endptr)
            *endptr = (wchar_t *)nptr;
        return 0;
    }
    char *end;
    intmax_t v = strtoimax(mb, &end, base);
    if (endptr)
        *endptr = (wchar_t *)nptr + (end - mb);
    if (mb != stack)
        free(mb);
    return v;
}

/*
 * wcstoumax() - convert a wide-character string to uintmax_t.
 * Utilises strtoumax after converting to multibyte.
 */
uintmax_t wcstoumax(const wchar_t *nptr, wchar_t **endptr, int base)
{
    char stack[128];
    size_t len;
    char *mb = wcs_to_mb_tmp(nptr, stack, sizeof(stack), &len);
    if (!mb) {
        if (endptr)
            *endptr = (wchar_t *)nptr;
        return 0;
    }
    char *end;
    uintmax_t v = strtoumax(mb, &end, base);
    if (endptr)
        *endptr = (wchar_t *)nptr + (end - mb);
    if (mb != stack)
        free(mb);
    return v;
}

/*
 * wcstod() - convert a wide-character string to double using strtod.
 */
double wcstod(const wchar_t *nptr, wchar_t **endptr)
{
    char stack[128];
    size_t len;
    char *mb = wcs_to_mb_tmp(nptr, stack, sizeof(stack), &len);
    if (!mb) {
        if (endptr)
            *endptr = (wchar_t *)nptr;
        return 0.0;
    }
    char *end;
    double v = strtod(mb, &end);
    if (endptr)
        *endptr = (wchar_t *)nptr + (end - mb);
    if (mb != stack)
        free(mb);
    return v;
}

/*
 * wcstof() - convert a wide-character string to float.
 * Simply wraps wcstod for implementation.
 */
float wcstof(const wchar_t *nptr, wchar_t **endptr)
{
    return (float)wcstod(nptr, endptr);
}

/*
 * wcstold() - convert a wide-character string to long double via strtold.
 */
long double wcstold(const wchar_t *nptr, wchar_t **endptr)
{
    char stack[128];
    size_t len;
    char *mb = wcs_to_mb_tmp(nptr, stack, sizeof(stack), &len);
    if (!mb) {
        if (endptr)
            *endptr = (wchar_t *)nptr;
        return 0.0L;
    }
    char *end;
    long double v = strtold(mb, &end);
    if (endptr)
        *endptr = (wchar_t *)nptr + (end - mb);
    if (mb != stack)
        free(mb);
    return v;
}
