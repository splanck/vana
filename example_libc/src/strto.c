/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the strto functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "stdlib.h"
#include "string.h"
#include <stdint.h>
#include <limits.h>
#include <errno.h>

static int digit_val(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'z')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'Z')
        return c - 'A' + 10;
    return -1;
}

long strtol(const char *nptr, char **endptr, int base)
{
    const char *s = nptr;
    while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r' || *s == '\f' || *s == '\v')
        s++;

    int neg = 0;
    if (*s == '+' || *s == '-') {
        if (*s == '-')
            neg = 1;
        s++;
    }

    if (base == 0) {
        if (*s == '0') {
            if (s[1] == 'x' || s[1] == 'X') {
                base = 16;
                s += 2;
            } else {
                base = 8;
                s += 1;
            }
        } else {
            base = 10;
        }
    } else if (base == 16) {
        if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
            s += 2;
    }

    unsigned long val = 0;
    const char *start = s;
    int d;
    unsigned long cutoff = neg ? (unsigned long)LONG_MAX + 1UL : (unsigned long)LONG_MAX;
    unsigned long cutdiv = cutoff / (unsigned long)base;
    unsigned long cutlim = cutoff % (unsigned long)base;
    int overflow = 0;
    while ((d = digit_val(*s)) >= 0 && d < base) {
        if (!overflow) {
            if (val > cutdiv || (val == cutdiv && (unsigned long)d > cutlim)) {
                overflow = 1;
            } else {
                val = val * (unsigned long)base + (unsigned long)d;
            }
        }
        s++;
    }

    if (endptr)
        *endptr = (char *)(s != start ? s : nptr);

    if (s == start)
        return 0;

    if (overflow) {
        errno = ERANGE;
        return neg ? LONG_MIN : LONG_MAX;
    }

    long result = (long)val;
    if (neg)
        result = -result;
    return result;
}

unsigned long strtoul(const char *nptr, char **endptr, int base)
{
    const char *s = nptr;
    while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r' || *s == '\f' ||
           *s == '\v')
        s++;

    int neg = 0;
    if (*s == '+' || *s == '-') {
        if (*s == '-')
            neg = 1;
        s++;
    }

    if (base == 0) {
        if (*s == '0') {
            if (s[1] == 'x' || s[1] == 'X') {
                base = 16;
                s += 2;
            } else {
                base = 8;
                s += 1;
            }
        } else {
            base = 10;
        }
    } else if (base == 16) {
        if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
            s += 2;
    }

    unsigned long val = 0;
    const char *start = s;
    int d;
    unsigned long cutoff = ULONG_MAX;
    unsigned long cutdiv = cutoff / (unsigned long)base;
    unsigned long cutlim = cutoff % (unsigned long)base;
    int overflow = 0;
    while ((d = digit_val(*s)) >= 0 && d < base) {
        if (!overflow) {
            if (val > cutdiv || (val == cutdiv && (unsigned long)d > cutlim)) {
                overflow = 1;
            } else {
                val = val * (unsigned long)base + (unsigned long)d;
            }
        }
        s++;
    }

    if (endptr)
        *endptr = (char *)(s != start ? s : nptr);

    if (s == start)
        return 0;

    if (overflow) {
        errno = ERANGE;
        return ULONG_MAX;
    }

    if (neg)
        return (unsigned long)(-(long)val);
    return val;
}

long long strtoll(const char *nptr, char **endptr, int base)
{
    const char *s = nptr;
    while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r' || *s == '\f' ||
           *s == '\v')
        s++;

    int neg = 0;
    if (*s == '+' || *s == '-') {
        if (*s == '-')
            neg = 1;
        s++;
    }

    if (base == 0) {
        if (*s == '0') {
            if (s[1] == 'x' || s[1] == 'X') {
                base = 16;
                s += 2;
            } else {
                base = 8;
                s += 1;
            }
        } else {
            base = 10;
        }
    } else if (base == 16) {
        if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
            s += 2;
    }

    unsigned long long val = 0;
    const char *start = s;
    int d;
    unsigned long long cutoff = neg ? (unsigned long long)LLONG_MAX + 1ULL : (unsigned long long)LLONG_MAX;
    unsigned long long cutdiv = cutoff / (unsigned long long)base;
    unsigned long long cutlim = cutoff % (unsigned long long)base;
    int overflow = 0;
    while ((d = digit_val(*s)) >= 0 && d < base) {
        if (!overflow) {
            if (val > cutdiv || (val == cutdiv && (unsigned long long)d > cutlim)) {
                overflow = 1;
            } else {
                val = val * (unsigned long long)base + (unsigned long long)d;
            }
        }
        s++;
    }

    if (endptr)
        *endptr = (char *)(s != start ? s : nptr);

    if (s == start)
        return 0;

    if (overflow) {
        errno = ERANGE;
        return neg ? LLONG_MIN : LLONG_MAX;
    }

    long long result = (long long)val;
    if (neg)
        result = -result;
    return result;
}

unsigned long long strtoull(const char *nptr, char **endptr, int base)
{
    const char *s = nptr;
    while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r' || *s == '\f' ||
           *s == '\v')
        s++;

    int neg = 0;
    if (*s == '+' || *s == '-') {
        if (*s == '-')
            neg = 1;
        s++;
    }

    if (base == 0) {
        if (*s == '0') {
            if (s[1] == 'x' || s[1] == 'X') {
                base = 16;
                s += 2;
            } else {
                base = 8;
                s += 1;
            }
        } else {
            base = 10;
        }
    } else if (base == 16) {
        if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
            s += 2;
    }

    unsigned long long val = 0;
    const char *start = s;
    int d;
    unsigned long long cutoff = ULLONG_MAX;
    unsigned long long cutdiv = cutoff / (unsigned long long)base;
    unsigned long long cutlim = cutoff % (unsigned long long)base;
    int overflow = 0;
    while ((d = digit_val(*s)) >= 0 && d < base) {
        if (!overflow) {
            if (val > cutdiv || (val == cutdiv && (unsigned long long)d > cutlim)) {
                overflow = 1;
            } else {
                val = val * (unsigned long long)base + (unsigned long long)d;
            }
        }
        s++;
    }

    if (endptr)
        *endptr = (char *)(s != start ? s : nptr);

    if (s == start)
        return 0;

    if (overflow) {
        errno = ERANGE;
        return ULLONG_MAX;
    }

    if (neg)
        return (unsigned long long)(-(long long)val);
    return val;
}

int atoi(const char *nptr)
{
    return (int)strtol(nptr, NULL, 10);
}

static double pow10_int(int exp)
{
    double p = 1.0;
    if (exp >= 0) {
        for (int i = 0; i < exp; ++i)
            p *= 10.0;
    } else {
        for (int i = 0; i < -exp; ++i)
            p /= 10.0;
    }
    return p;
}

double strtod(const char *nptr, char **endptr)
{
    const char *s = nptr;
    while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r' ||
           *s == '\f' || *s == '\v')
        s++;

    int neg = 0;
    if (*s == '+' || *s == '-') {
        if (*s == '-')
            neg = 1;
        s++;
    }

    double val = 0.0;
    int seen = 0;
    while (*s >= '0' && *s <= '9') {
        val = val * 10.0 + (double)(*s - '0');
        s++;
        seen = 1;
    }

    if (*s == '.') {
        s++;
        double frac = 0.0;
        double place = 1.0;
        while (*s >= '0' && *s <= '9') {
            frac = frac * 10.0 + (double)(*s - '0');
            place *= 10.0;
            s++;
            seen = 1;
        }
        val += frac / place;
    }

    const char *exp_start = s;
    int exp = 0;
    int expneg = 0;
    if (*s == 'e' || *s == 'E') {
        s++;
        if (*s == '+' || *s == '-') {
            if (*s == '-')
                expneg = 1;
            s++;
        }
        const char *digits = s;
        while (*s >= '0' && *s <= '9') {
            exp = exp * 10 + (*s - '0');
            s++;
        }
        if (s == digits) {
            s = exp_start; /* no digits after 'e'; backtrack */
            exp = 0;
        } else {
            if (expneg)
                exp = -exp;
        }
    }

    if (endptr)
        *endptr = (char *)(seen ? s : nptr);

    if (!seen)
        return 0.0;

    if (exp)
        val *= pow10_int(exp);

    if (neg)
        val = -val;

    return val;
}

double atof(const char *nptr)
{
    return strtod(nptr, NULL);
}

float strtof(const char *nptr, char **endptr)
{
    return (float)strtod(nptr, endptr);
}

static long double pow10_ld(int exp)
{
    long double p = 1.0;
    if (exp >= 0) {
        for (int i = 0; i < exp; ++i)
            p *= 10.0;
    } else {
        for (int i = 0; i < -exp; ++i)
            p /= 10.0;
    }
    return p;
}

long double strtold(const char *nptr, char **endptr)
{
    const char *s = nptr;
    while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r' ||
           *s == '\f' || *s == '\v')
        s++;

    int neg = 0;
    if (*s == '+' || *s == '-') {
        if (*s == '-')
            neg = 1;
        s++;
    }

    long double val = 0.0;
    int seen = 0;
    while (*s >= '0' && *s <= '9') {
        val = val * 10.0 + (long double)(*s - '0');
        s++;
        seen = 1;
    }

    if (*s == '.') {
        s++;
        long double frac = 0.0;
        long double place = 1.0;
        while (*s >= '0' && *s <= '9') {
            frac = frac * 10.0 + (long double)(*s - '0');
            place *= 10.0;
            s++;
            seen = 1;
        }
        val += frac / place;
    }

    const char *exp_start = s;
    int exp = 0;
    int expneg = 0;
    if (*s == 'e' || *s == 'E') {
        s++;
        if (*s == '+' || *s == '-') {
            if (*s == '-')
                expneg = 1;
            s++;
        }
        const char *digits = s;
        while (*s >= '0' && *s <= '9') {
            exp = exp * 10 + (*s - '0');
            s++;
        }
        if (s == digits) {
            s = exp_start; /* no digits after 'e'; backtrack */
            exp = 0;
        } else {
            if (expneg)
                exp = -exp;
        }
    }

    if (endptr)
        *endptr = (char *)(seen ? s : nptr);

    if (!seen)
        return 0.0L;

    if (exp)
        val *= pow10_ld(exp);

    if (neg)
        val = -val;

    return val;
}

intmax_t strtoimax(const char *nptr, char **endptr, int base)
{
    return (intmax_t)strtoll(nptr, endptr, base);
}

uintmax_t strtoumax(const char *nptr, char **endptr, int base)
{
    return (uintmax_t)strtoull(nptr, endptr, base);
}

