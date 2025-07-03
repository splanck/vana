/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the wscanf functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "stdio.h"
#include "wchar.h"
#include "wctype.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* Advance past leading whitespace characters */
static const wchar_t *skip_ws_w(const wchar_t *s)
{
    while (*s && iswspace(*s))
        s++;
    return s;
}

/* Convert wide string to long with base and optional end pointer */
static long wstrtol_wrap(const wchar_t *s, const wchar_t **end, int base)
{
    char buf[128];
    size_t len = wcstombs(buf, s, sizeof(buf) - 1);
    if (len == (size_t)-1) {
        errno = EILSEQ;
        if (end)
            *end = s;
        return 0;
    }
    buf[len] = '\0';
    char *endp;
    long v = strtol(buf, &endp, base);
    if (end)
        *end = s + (endp - buf);
    return v;
}

/* Convert wide string to unsigned long with base and optional end */
static unsigned long wstrtoul_wrap(const wchar_t *s, const wchar_t **end, int base)
{
    char buf[128];
    size_t len = wcstombs(buf, s, sizeof(buf) - 1);
    if (len == (size_t)-1) {
        errno = EILSEQ;
        if (end)
            *end = s;
        return 0;
    }
    buf[len] = '\0';
    char *endp;
    unsigned long v = strtoul(buf, &endp, base);
    if (end)
        *end = s + (endp - buf);
    return v;
}

/* Convert wide string to double and optionally set end */
static double wstrtod_wrap(const wchar_t *s, const wchar_t **end)
{
    char buf[128];
    size_t len = wcstombs(buf, s, sizeof(buf) - 1);
    if (len == (size_t)-1) {
        errno = EILSEQ;
        if (end)
            *end = s;
        return 0.0;
    }
    buf[len] = '\0';
    char *endp;
    double v = strtod(buf, &endp);
    if (end)
        *end = s + (endp - buf);
    return v;
}

/* Internal helper for wide string scanning */
static int vswscanf_impl(const wchar_t *str, const wchar_t *fmt, va_list ap)
{
    const wchar_t *s = str;
    int count = 0;
    for (; *fmt; ++fmt) {
        if (iswspace(*fmt)) {
            while (iswspace(*fmt))
                fmt++;
            fmt--;
            s = skip_ws_w(s);
            continue;
        }
        if (*fmt != L'%') {
            if (*s != *fmt)
                return count;
            if (*s)
                s++;
            continue;
        }
        fmt++;
        int long_mod = 0;
        if (*fmt == L'l') {
            long_mod = 1;
            fmt++;
        }
        if (*fmt == L'd') {
            s = skip_ws_w(s);
            const wchar_t *end;
            errno = 0;
            long v = wstrtol_wrap(s, &end, 10);
            if (errno == EILSEQ && end == s)
                return count;
            if (end == s)
                return count;
            *va_arg(ap, int *) = (int)v;
            s = end;
            count++;
        } else if (*fmt == L'u') {
            s = skip_ws_w(s);
            const wchar_t *end;
            errno = 0;
            long v = wstrtol_wrap(s, &end, 10);
            if (errno == EILSEQ && end == s)
                return count;
            if (end == s || v < 0)
                return count;
            *va_arg(ap, unsigned *) = (unsigned)v;
            s = end;
            count++;
        } else if (*fmt == L'x' || *fmt == L'X') {
            s = skip_ws_w(s);
            const wchar_t *end;
            errno = 0;
            unsigned long v = wstrtoul_wrap(s, &end, 16);
            if (errno == EILSEQ && end == s)
                return count;
            if (end == s)
                return count;
            *va_arg(ap, unsigned *) = (unsigned)v;
            s = end;
            count++;
        } else if (*fmt == L'o') {
            s = skip_ws_w(s);
            const wchar_t *end;
            errno = 0;
            unsigned long v = wstrtoul_wrap(s, &end, 8);
            if (errno == EILSEQ && end == s)
                return count;
            if (end == s)
                return count;
            *va_arg(ap, unsigned *) = (unsigned)v;
            s = end;
            count++;
        } else if (*fmt == L'f' || *fmt == L'F' ||
                   *fmt == L'e' || *fmt == L'E' ||
                   *fmt == L'g' || *fmt == L'G') {
            s = skip_ws_w(s);
            const wchar_t *end;
            errno = 0;
            double v = wstrtod_wrap(s, &end);
            if (errno == EILSEQ && end == s)
                return count;
            if (end == s)
                return count;
            if (long_mod)
                *va_arg(ap, double *) = v;
            else
                *va_arg(ap, float *) = (float)v;
            s = end;
            count++;
        } else if (*fmt == L's') {
            s = skip_ws_w(s);
            wchar_t *out = va_arg(ap, wchar_t *);
            if (!*s)
                return count;
            while (*s && !iswspace(*s))
                *out++ = *s++;
            *out = L'\0';
            count++;
        } else if (*fmt == L'%') {
            if (*s != L'%')
                return count;
            if (*s)
                s++;
        } else {
            return count;
        }
    }
    return count;
}

/* Parse wide string using format and argument list */
int vswscanf(const wchar_t *str, const wchar_t *format, va_list ap)
{
    return vswscanf_impl(str, format, ap);
}

/* Scan wide string with variable arguments */
int swscanf(const wchar_t *str, const wchar_t *format, ...)
{
    va_list ap;
    va_start(ap, format);
    int r = vswscanf_impl(str, format, ap);
    va_end(ap);
    return r;
}

/* Core worker reading from FILE stream */
static int vfwscanf_impl(FILE *stream, const wchar_t *format, va_list ap)
{
    char buf[256];
    size_t pos = 0;
    int c;
    while (pos + 1 < sizeof(buf) && (c = fgetc(stream)) != -1) {
        buf[pos++] = (char)c;
        if (c == '\n')
            break;
    }
    buf[pos] = '\0';
    wchar_t wbuf[256];
    mbstowcs(wbuf, buf, sizeof(wbuf)/sizeof(wchar_t));
    return vswscanf_impl(wbuf, format, ap);
}

/* Public wrapper for vfwscanf_impl */
int vfwscanf(FILE *stream, const wchar_t *format, va_list ap)
{
    return vfwscanf_impl(stream, format, ap);
}

/* Read formatted input from stdin using va_list */
int vwscanf(const wchar_t *format, va_list ap)
{
    return vfwscanf_impl(stdin, format, ap);
}

/* Read formatted input from FILE stream */
int fwscanf(FILE *stream, const wchar_t *format, ...)
{
    va_list ap;
    va_start(ap, format);
    int r = vfwscanf(stream, format, ap);
    va_end(ap);
    return r;
}

/* Read formatted input from stdin */
int wscanf(const wchar_t *format, ...)
{
    va_list ap;
    va_start(ap, format);
    int r = vfwscanf_impl(stdin, format, ap);
    va_end(ap);
    return r;
}


