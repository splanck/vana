/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the wcsftime functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "wchar.h"
#include "time.h"
#include "stdlib.h"
#include "memory.h"

/*
 * wcsftime() - format time data as a wide-character string using strftime
 * as the backing implementation.
 */
size_t wcsftime(wchar_t *s, size_t max, const wchar_t *format, const struct tm *tm)
{
    if (!s || !format || !tm || max == 0)
        return 0;

    size_t flen = wcstombs(NULL, format, 0);
    char *fmt = malloc(flen + 1);
    if (!fmt)
        return 0;
    wcstombs(fmt, format, flen + 1);

    char *buf = malloc(max);
    if (!buf) {
        free(fmt);
        return 0;
    }

    size_t n = strftime(buf, max, fmt, tm);
    if (n > 0) {
        for (size_t i = 0; i < n && i < max; i++)
            s[i] = (unsigned char)buf[i];
        if (n < max)
            s[n] = 0;
    }

    free(buf);
    free(fmt);
    return n;
}
