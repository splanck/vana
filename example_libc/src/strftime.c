/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the strftime functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "time.h"
#include "string.h"

static const char *wd_short[7] = {
    "Sun","Mon","Tue","Wed","Thu","Fri","Sat"
};

static const char *mon_short[12] = {
    "Jan","Feb","Mar","Apr","May","Jun",
    "Jul","Aug","Sep","Oct","Nov","Dec"
};

static int fmt_int(char *buf, size_t size, int val, int width)
{
    char tmp[16];
    int pos = 0;
    if (val < 0)
        val = -val;
    do {
        tmp[pos++] = '0' + (val % 10);
        val /= 10;
    } while (val && pos < (int)sizeof(tmp));
    while (pos < width && pos < (int)sizeof(tmp))
        tmp[pos++] = '0';
    if (pos > (int)size)
        pos = (int)size;
    for (int i = 0; i < pos; ++i)
        buf[i] = tmp[pos - i - 1];
    return pos;
}

size_t strftime(char *s, size_t max, const char *format, const struct tm *tm)
{
    if (!s || !format || !tm || max == 0)
        return 0;
    size_t pos = 0;
    for (const char *p = format; *p; ++p) {
        if (*p != '%') {
            if (pos + 1 >= max)
                return 0;
            s[pos++] = *p;
            continue;
        }
        ++p;
        if (!*p)
            break;
        char buf[16];
        int len = 0;
        switch (*p) {
        case '%':
            if (pos + 1 >= max)
                return 0;
            s[pos++] = '%';
            break;
        case 'a': {
            const char *wd = (tm->tm_wday >= 0 && tm->tm_wday < 7) ?
                wd_short[tm->tm_wday] : "";
            len = (int)strlen(wd);
            if (pos + (size_t)len >= max)
                return 0;
            memcpy(s + pos, wd, (size_t)len);
            pos += (size_t)len;
            break;
        }
        case 'b': {
            const char *mn = (tm->tm_mon >= 0 && tm->tm_mon < 12) ?
                mon_short[tm->tm_mon] : "";
            len = (int)strlen(mn);
            if (pos + (size_t)len >= max)
                return 0;
            memcpy(s + pos, mn, (size_t)len);
            pos += (size_t)len;
            break;
        }
        case 'Y':
            len = fmt_int(buf, sizeof(buf), tm->tm_year + 1900, 4);
            if (pos + (size_t)len >= max)
                return 0;
            memcpy(s + pos, buf, (size_t)len);
            pos += (size_t)len;
            break;
        case 'm':
            len = fmt_int(buf, sizeof(buf), tm->tm_mon + 1, 2);
            if (pos + (size_t)len >= max)
                return 0;
            memcpy(s + pos, buf, (size_t)len);
            pos += (size_t)len;
            break;
        case 'd':
            len = fmt_int(buf, sizeof(buf), tm->tm_mday, 2);
            if (pos + (size_t)len >= max)
                return 0;
            memcpy(s + pos, buf, (size_t)len);
            pos += (size_t)len;
            break;
        case 'H':
            len = fmt_int(buf, sizeof(buf), tm->tm_hour, 2);
            if (pos + (size_t)len >= max)
                return 0;
            memcpy(s + pos, buf, (size_t)len);
            pos += (size_t)len;
            break;
        case 'M':
            len = fmt_int(buf, sizeof(buf), tm->tm_min, 2);
            if (pos + (size_t)len >= max)
                return 0;
            memcpy(s + pos, buf, (size_t)len);
            pos += (size_t)len;
            break;
        case 'S':
            len = fmt_int(buf, sizeof(buf), tm->tm_sec, 2);
            if (pos + (size_t)len >= max)
                return 0;
            memcpy(s + pos, buf, (size_t)len);
            pos += (size_t)len;
            break;
        case 'Z':
            len = 3;
            if (pos + (size_t)len >= max)
                return 0;
            memcpy(s + pos, "UTC", (size_t)len);
            pos += (size_t)len;
            break;
        case 'z':
            if (pos + 5 >= max)
                return 0;
            memcpy(s + pos, "+0000", 5);
            pos += 5;
            break;
        case 'w':
            len = fmt_int(buf, sizeof(buf), tm->tm_wday, 1);
            if (pos + (size_t)len >= max)
                return 0;
            memcpy(s + pos, buf, (size_t)len);
            pos += (size_t)len;
            break;
        case 'u':
            len = fmt_int(buf, sizeof(buf), tm->tm_wday == 0 ? 7 : tm->tm_wday, 1);
            if (pos + (size_t)len >= max)
                return 0;
            memcpy(s + pos, buf, (size_t)len);
            pos += (size_t)len;
            break;
        default:
            if (pos + 1 >= max)
                return 0;
            s[pos++] = '%';
            if (pos + 1 >= max)
                return 0;
            s[pos++] = *p;
            break;
        }
    }
    if (pos >= max)
        return 0;
    s[pos] = '\0';
    return pos;
}

