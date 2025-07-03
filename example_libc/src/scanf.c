/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the scanf functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "stdio.h"
#include "ctype.h"
#include "string.h"
#include "stdlib.h"

static const char *skip_ws(const char *s)
{
    while (*s && isspace((unsigned char)*s))
        s++;
    return s;
}

/*
 * vsscanf_impl is the workhorse for all sscanf variants. It parses the
 * supplied string according to the given format string using the provided
 * va_list and stores the results into the caller supplied pointers. The
 * function returns the number of successfully scanned fields.
 */
static int vsscanf_impl(const char *str, const char *fmt, va_list ap)
{
    const char *s = str;
    int count = 0;
    for (; *fmt; ++fmt) {
        if (isspace((unsigned char)*fmt)) {
            while (isspace((unsigned char)*fmt))
                fmt++;
            fmt--;
            s = skip_ws(s);
            continue;
        }
        if (*fmt != '%') {
            if (*s != *fmt)
                return count;
            if (*s)
                s++;
            continue;
        }
        fmt++;
        int long_mod = 0;
        if (*fmt == 'l') {
            long_mod = 1;
            fmt++;
        }
        if (*fmt == 'd') {
            s = skip_ws(s);
            char *end;
            long v = strtol(s, &end, 10);
            if (end == s)
                return count;
            *va_arg(ap, int *) = (int)v;
            s = end;
            count++;
        } else if (*fmt == 'u') {
            s = skip_ws(s);
            char *end;
            long v = strtol(s, &end, 10);
            if (end == s || v < 0)
                return count;
            *va_arg(ap, unsigned *) = (unsigned)v;
            s = end;
            count++;
        } else if (*fmt == 'x' || *fmt == 'X') {
            s = skip_ws(s);
            char *end;
            unsigned long v = strtoul(s, &end, 16);
            if (end == s)
                return count;
            *va_arg(ap, unsigned *) = (unsigned)v;
            s = end;
            count++;
        } else if (*fmt == 'o') {
            s = skip_ws(s);
            char *end;
            unsigned long v = strtoul(s, &end, 8);
            if (end == s)
                return count;
            *va_arg(ap, unsigned *) = (unsigned)v;
            s = end;
            count++;
        } else if (*fmt == 'f' || *fmt == 'F' ||
                   *fmt == 'e' || *fmt == 'E' ||
                   *fmt == 'g' || *fmt == 'G') {
            s = skip_ws(s);
            char *end;
            double v = strtod(s, &end);
            if (end == s)
                return count;
            if (long_mod)
                *va_arg(ap, double *) = v;
            else
                *va_arg(ap, float *) = (float)v;
            s = end;
            count++;
        } else if (*fmt == 's') {
            s = skip_ws(s);
            char *out = va_arg(ap, char *);
            if (!*s)
                return count;
            while (*s && !isspace((unsigned char)*s))
                *out++ = *s++;
            *out = '\0';
            count++;
        } else if (*fmt == '%') {
            if (*s != '%')
                return count;
            if (*s)
                s++;
        } else {
            return count;
        }
    }
    return count;
}

/*
 * vsscanf is the public interface that scans formatted input from a memory
 * buffer using a variable argument list.
 */
int vsscanf(const char *str, const char *format, va_list ap)
{
    return vsscanf_impl(str, format, ap);
}

/*
 * sscanf scans formatted input from a string. It forwards the variable
 * arguments to vsscanf_impl and returns the number of fields obtained.
 */
int sscanf(const char *str, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    int r = vsscanf_impl(str, format, ap);
    va_end(ap);
    return r;
}

/*
 * vfscanf_impl reads a line from the given FILE stream and then parses it
 * using vsscanf_impl. Only a small buffer is used and scanning stops at a
 * newline or EOF.
 */
static int vfscanf_impl(FILE *stream, const char *format, va_list ap)
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
    return vsscanf_impl(buf, format, ap);
}

/*
 * vfscanf is the stdio variant that accepts a va_list and reads from a
 * FILE stream.
 */
int vfscanf(FILE *stream, const char *format, va_list ap)
{
    return vfscanf_impl(stream, format, ap);
}

/*
 * fscanf reads formatted data from the given FILE stream. It is implemented
 * on top of vfscanf_impl and returns the number of successfully scanned
 * fields.
 */
int fscanf(FILE *stream, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    int r = vfscanf_impl(stream, format, ap);
    va_end(ap);
    return r;
}

int scanf(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    int r = vfscanf_impl(stdin, format, ap);
    va_end(ap);
    return r;
}

int vscanf(const char *format, va_list ap)
{
    return vfscanf_impl(stdin, format, ap);
}
