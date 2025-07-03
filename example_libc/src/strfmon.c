/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements a minimal strfmon for vlibc. Provides basic POSIX compliant monetary formatting for ASCII locales.
 */

#include "monetary.h"
#include "string.h"
#include "stdio.h"
#include "errno.h"
#include <stdarg.h>
#include <stdint.h>

/*
 * Convert a positive double to a decimal string with the given precision.
 * Returns the number of characters written not including the null byte or
 * -1 on error. The buffer is always null terminated when size > 0.
 */
static int fmt_double(char *buf, size_t size, double val, int prec)
{
    if (size == 0)
        return -1;

    if (prec < 0)
        prec = 0;
    if (prec > 9)
        prec = 9; /* avoid overflow of scaling factor */

    /* round to the requested precision */
    double r = 0.5;
    for (int i = 0; i < prec; i++)
        r /= 10.0;
    val += r;

    uint64_t intpart = (uint64_t)val;
    double frac = val - (double)intpart;

    uint64_t scale = 1;
    for (int i = 0; i < prec; i++)
        scale *= 10;
    uint64_t fracpart = (uint64_t)(frac * (double)scale);

    /* convert integer part */
    char tmp[32];
    size_t pos = 0;
    do {
        tmp[pos++] = (char)('0' + intpart % 10);
        intpart /= 10;
    } while (intpart && pos < sizeof(tmp));
    if (pos >= sizeof(tmp))
        return -1;

    /* ensure buffer is large enough */
    size_t needed = pos + (prec ? prec + 1 : 0);
    if (needed >= size)
        return -1;

    /* output integer part */
    size_t out = 0;
    for (size_t i = 0; i < pos; i++)
        buf[out++] = tmp[pos - i - 1];

    /* output fractional part */
    if (prec) {
        buf[out++] = '.';
        for (int i = prec - 1; i >= 0; i--) {
            buf[out + i] = (char)('0' + (fracpart % 10));
            fracpart /= 10;
        }
        out += (size_t)prec;
    }

    buf[out] = '\0';
    return (int)out;
}

ssize_t strfmon(char *s, size_t max, const char *format, ...)
{
    if (!s || !format || max == 0) {
        errno = EINVAL;
        return -1;
    }

    va_list ap;
    va_start(ap, format);
    char *out = s;
    size_t remaining = max;
    size_t written = 0;

    for (const char *p = format; *p; ++p) {
        if (*p != '%') {
            if (remaining <= 1) {
                errno = E2BIG;
                va_end(ap);
                return -1;
            }
            *out++ = *p;
            remaining--;
            written++;
            continue;
        }

        ++p;
        if (*p == '%') {
            if (remaining <= 1) {
                errno = E2BIG;
                va_end(ap);
                return -1;
            }
            *out++ = '%';
            remaining--;
            written++;
            continue;
        }

        int left_align = 0;
        int width = 0;
        int prec = 2;

        for (;; ++p) {
            if (*p == '-') {
                left_align = 1;
            } else if (*p == '+' || *p == '(' || *p == '!' || *p == '^' || *p == '=') {
                /* ignored flags */
            } else {
                break;
            }
        }

        while (*p >= '0' && *p <= '9') {
            width = width * 10 + (*p - '0');
            ++p;
        }

        if (*p == '.') {
            ++p;
            prec = 0;
            while (*p >= '0' && *p <= '9') {
                prec = prec * 10 + (*p - '0');
                ++p;
            }
        }

        char conv = *p;
        if (conv != 'n' && conv != 'i') {
            errno = EINVAL;
            va_end(ap);
            return -1;
        }

        double val = va_arg(ap, double);
        int neg = val < 0.0;
        if (neg)
            val = -val;

        char numbuf[64];
        int n = fmt_double(numbuf, sizeof(numbuf), val, prec);
        if (n < 0) {
            errno = E2BIG;
            va_end(ap);
            return -1;
        }

        char curbuf[80];
        n = snprintf(curbuf, sizeof(curbuf), neg ? "-$%s" : "$%s", numbuf);
        if (n < 0 || (size_t)n >= sizeof(curbuf)) {
            errno = E2BIG;
            va_end(ap);
            return -1;
        }

        size_t len = (size_t)n;
        size_t pad = 0;
        if (width > 0 && (size_t)width > len)
            pad = (size_t)width - len;

        if (!left_align) {
            if (remaining <= pad + len) {
                errno = E2BIG;
                va_end(ap);
                return -1;
            }
            for (size_t i = 0; i < pad; ++i) {
                *out++ = ' ';
                remaining--;
                written++;
            }
            memcpy(out, curbuf, len);
            out += len;
            remaining -= len;
            written += len;
        } else {
            if (remaining <= pad + len) {
                errno = E2BIG;
                va_end(ap);
                return -1;
            }
            memcpy(out, curbuf, len);
            out += len;
            remaining -= len;
            written += len;
            for (size_t i = 0; i < pad; ++i) {
                *out++ = ' ';
                remaining--;
                written++;
            }
        }
    }

    if (remaining == 0) {
        errno = E2BIG;
        va_end(ap);
        return -1;
    }

    *out = '\0';
    va_end(ap);
    return (ssize_t)written;
}

