/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the printf functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "stdio.h"
#include "io.h"
#include "memory.h"
#include "errno.h"
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>

static int uint_to_base(unsigned long value, unsigned base, int upper,
                        char *buf, size_t size)
{
    const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    char tmp[32];
    size_t i = 0;
    do {
        tmp[i++] = digits[value % base];
        value /= base;
    } while (value && i < sizeof(tmp));
    if (i > size)
        i = size;
    for (size_t j = 0; j < i; ++j)
        buf[j] = tmp[i - j - 1];
    return (int)i;
}

static int ull_to_base(unsigned long long value, unsigned base, int upper,
                       char *buf, size_t size)
{
    const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    char tmp[64];
    size_t i = 0;
    do {
        tmp[i++] = digits[value % base];
        value /= base;
    } while (value && i < sizeof(tmp));
    if (i > size)
        i = size;
    for (size_t j = 0; j < i; ++j)
        buf[j] = tmp[i - j - 1];
    return (int)i;
}


static void out_char(char *dst, size_t size, size_t *pos, char c)
{
    if (*pos + 1 < size)
        dst[*pos] = c;
    (*pos)++;
}

static void out_str(char *dst, size_t size, size_t *pos,
                    const char *s, size_t len)
{
    for (size_t i = 0; i < len; i++)
        out_char(dst, size, pos, s[i]);
}

/*
 * vsnprintf_impl handles the core of formatted output. It parses the format
 * string and builds a temporary result in the caller provided buffer. Only a
 * small subset of printf formatting is implemented.
 */
static int vsnprintf_impl(char *str, size_t size, const char *fmt, va_list ap)
{
    size_t pos = 0;
    for (const char *p = fmt; *p; ++p) {
        if (*p != '%') {
            out_char(str, size, &pos, *p);
            continue;
        }
        ++p;
        if (*p == '%') {
            out_char(str, size, &pos, '%');
            continue;
        }

        int width = 0;
        int precision = -1;
        while (*p >= '0' && *p <= '9') {
            width = width * 10 + (*p - '0');
            ++p;
        }
        if (*p == '.') {
            ++p;
            precision = 0;
            while (*p >= '0' && *p <= '9') {
                precision = precision * 10 + (*p - '0');
                ++p;
            }
        }

        int length = 0;
        if (*p == 'l') {
            length = 1;
            ++p;
            if (*p == 'l') {
                length = 2;
                ++p;
            }
        } else if (*p == 'j') {
            length = 3;
            ++p;
        } else if (*p == 'z') {
            length = 4;
            ++p;
        }

        char spec = *p;
        char buf[64];
        int len = 0;
        const char *prefix = "";
        int prefix_len = 0;
        int sign = 0;

        switch (spec) {
        case 's': {
            const char *s = va_arg(ap, const char *);
            if (!s)
                s = "(null)";
            size_t slen = strlen(s);
            if (precision >= 0 && (size_t)precision < slen)
                slen = (size_t)precision;
            if (width > 0 && (int)slen < width) {
                for (int i = 0; i < width - (int)slen; i++)
                    out_char(str, size, &pos, ' ');
            }
            out_str(str, size, &pos, s, slen);
            break;
        }
        case 'd': {
            if (length == 3) {
                intmax_t v = va_arg(ap, intmax_t);
                unsigned long long uv = (unsigned long long)v;
                sign = v < 0;
                if (sign)
                    uv = 0ull - uv;
                len = ull_to_base(uv, 10, 0, buf, sizeof(buf));
            } else if (length == 2) {
                long long v = va_arg(ap, long long);
                unsigned long long uv = (unsigned long long)v;
                sign = v < 0;
                if (sign)
                    uv = 0ull - uv;
                len = ull_to_base(uv, 10, 0, buf, sizeof(buf));
            } else if (length == 4) {
                ssize_t v = va_arg(ap, ssize_t);
                unsigned long long uv = (unsigned long long)v;
                sign = v < 0;
                if (sign)
                    uv = 0ull - uv;
                len = ull_to_base(uv, 10, 0, buf, sizeof(buf));
            } else if (length == 1) {
                long v = va_arg(ap, long);
                unsigned long uv = (unsigned long)v;
                sign = v < 0;
                if (sign)
                    uv = 0ul - uv;
                len = uint_to_base(uv, 10, 0, buf, sizeof(buf));
            } else {
                int v = va_arg(ap, int);
                unsigned int uv = (unsigned int)v;
                sign = v < 0;
                if (sign)
                    uv = 0u - uv;
                len = uint_to_base(uv, 10, 0, buf, sizeof(buf));
            }
            break;
        }
        case 'u': {
            if (length == 3) {
                uintmax_t v = va_arg(ap, uintmax_t);
                len = ull_to_base((unsigned long long)v, 10, 0, buf, sizeof(buf));
            } else if (length == 2) {
                unsigned long long v = va_arg(ap, unsigned long long);
                len = ull_to_base(v, 10, 0, buf, sizeof(buf));
            } else if (length == 4) {
                size_t v = va_arg(ap, size_t);
                len = ull_to_base((unsigned long long)v, 10, 0, buf, sizeof(buf));
            } else if (length == 1) {
                unsigned long v = va_arg(ap, unsigned long);
                len = uint_to_base(v, 10, 0, buf, sizeof(buf));
            } else {
                unsigned int v = va_arg(ap, unsigned int);
                len = uint_to_base(v, 10, 0, buf, sizeof(buf));
            }
            break;
        }
        case 'x':
        case 'X': {
            if (length == 3) {
                uintmax_t v = va_arg(ap, uintmax_t);
                len = ull_to_base((unsigned long long)v, 16, spec == 'X', buf, sizeof(buf));
            } else if (length == 2) {
                unsigned long long v = va_arg(ap, unsigned long long);
                len = ull_to_base(v, 16, spec == 'X', buf, sizeof(buf));
            } else if (length == 4) {
                size_t v = va_arg(ap, size_t);
                len = ull_to_base((unsigned long long)v, 16, spec == 'X', buf, sizeof(buf));
            } else if (length == 1) {
                unsigned long v = va_arg(ap, unsigned long);
                len = uint_to_base(v, 16, spec == 'X', buf, sizeof(buf));
            } else {
                unsigned int v = va_arg(ap, unsigned int);
                len = uint_to_base(v, 16, spec == 'X', buf, sizeof(buf));
            }
            break;
        }
        case 'o': {
            if (length == 3) {
                uintmax_t v = va_arg(ap, uintmax_t);
                len = ull_to_base((unsigned long long)v, 8, 0, buf, sizeof(buf));
            } else if (length == 2) {
                unsigned long long v = va_arg(ap, unsigned long long);
                len = ull_to_base(v, 8, 0, buf, sizeof(buf));
            } else if (length == 4) {
                size_t v = va_arg(ap, size_t);
                len = ull_to_base((unsigned long long)v, 8, 0, buf, sizeof(buf));
            } else if (length == 1) {
                unsigned long v = va_arg(ap, unsigned long);
                len = uint_to_base(v, 8, 0, buf, sizeof(buf));
            } else {
                unsigned int v = va_arg(ap, unsigned int);
                len = uint_to_base(v, 8, 0, buf, sizeof(buf));
            }
            break;
        }
        case 'p': {
            uintptr_t v = (uintptr_t)va_arg(ap, void *);
            prefix = "0x";
            prefix_len = 2;
            len = ull_to_base((unsigned long long)v, 16, 0, buf, sizeof(buf));
            break;
        }
        case 'c': {
            buf[0] = (char)va_arg(ap, int);
            len = 1;
            break;
        }
        default:
            out_char(str, size, &pos, '%');
            if (spec)
                out_char(str, size, &pos, spec);
            continue;
        }

        int num_len = len;
        if (precision > num_len)
            num_len = precision;
        int total = prefix_len + (sign ? 1 : 0) + num_len;
        if (width > total) {
            for (int i = 0; i < width - total; i++)
                out_char(str, size, &pos, ' ');
        }
        if (sign)
            out_char(str, size, &pos, '-');
        out_str(str, size, &pos, prefix, (size_t)prefix_len);
        for (int i = 0; i < num_len - len; i++)
            out_char(str, size, &pos, '0');
        out_str(str, size, &pos, buf, (size_t)len);
    }

    if (size > 0) {
        if (pos >= size)
            str[size - 1] = '\0';
        else
            str[pos] = '\0';
    }
    return (int)pos;
}

int vsnprintf(char *str, size_t size, const char *format, va_list ap)
{
    return vsnprintf_impl(str, size, format, ap);
}

int snprintf(char *str, size_t size, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    int r = vsnprintf(str, size, format, ap);
    va_end(ap);
    return r;
}

int vsprintf(char *str, const char *format, va_list ap)
{
    return vsnprintf(str, (size_t)-1, format, ap);
}

int sprintf(char *str, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    int r = vsprintf(str, format, ap);
    va_end(ap);
    return r;
}

/*
 * vfdprintf formats to a fixed-size buffer and writes the result to a
 * file descriptor. It is the low level helper used by printf and friends.
 */
static int vfdprintf(int fd, const char *format, va_list ap)
{
    char buf[1024];
    int len = vsnprintf(buf, sizeof(buf), format, ap);
    if (len > 0 && fd >= 0) {
        size_t off = 0;
        while (off < (size_t)len) {
            ssize_t w = write(fd, buf + off, (size_t)len - off);
            if (w < 0) {
                if (errno == EINTR || errno == EAGAIN)
                    continue;
                return -1;
            }
            off += (size_t)w;
        }
    }
    return len;
}

int vdprintf(int fd, const char *format, va_list ap)
{
    return vfdprintf(fd, format, ap);
}

int dprintf(int fd, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    int r = vdprintf(fd, format, ap);
    va_end(ap);
    return r;
}

/*
 * vfprintf sends formatted output to a FILE stream. Memory-backed streams
 * are handled specially by formatting into a temporary buffer and then
 * writing with fwrite(). For regular files, vfdprintf performs the work.
 */
int vfprintf(FILE *stream, const char *format, va_list ap)
{
    if (stream && stream->is_mem) {
        va_list copy;
        va_copy(copy, ap);
        int len = vsnprintf(NULL, 0, format, copy);
        va_end(copy);
        if (len < 0)
            return -1;
        char *buf = malloc((size_t)len + 1);
        if (!buf) {
            errno = ENOMEM;
            return -1;
        }
        vsnprintf(buf, (size_t)len + 1, format, ap);
        size_t w = fwrite(buf, 1, (size_t)len, stream);
        free(buf);
        return (int)w;
    }
    return vfdprintf(stream ? stream->fd : -1, format, ap);
}

int vprintf(const char *format, va_list ap)
{
    return vfdprintf(1, format, ap);
}

int fprintf(FILE *stream, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    int r = vfprintf(stream, format, ap);
    va_end(ap);
    return r;
}

/*
 * printf formats the arguments according to the format string and writes the
 * result to standard output. It delegates most of the work to vprintf.
 */
int printf(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    int r = vprintf(format, ap);
    va_end(ap);
    return r;
}

int vasprintf(char **strp, const char *format, va_list ap)
{
    va_list copy;
    va_copy(copy, ap);
    int len = vsnprintf(NULL, 0, format, copy);
    va_end(copy);
    if (len < 0) {
        if (strp)
            *strp = NULL;
        return -1;
    }
    char *buf = malloc((size_t)len + 1);
    if (!buf) {
        if (strp)
            *strp = NULL;
        errno = ENOMEM;
        return -1;
    }
    int r = vsnprintf(buf, (size_t)len + 1, format, ap);
    if (r < 0) {
        free(buf);
        if (strp)
            *strp = NULL;
        return -1;
    }
    if (strp)
        *strp = buf;
    else
        free(buf);
    return r;
}

int asprintf(char **strp, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    int r = vasprintf(strp, format, ap);
    va_end(ap);
    return r;
}

