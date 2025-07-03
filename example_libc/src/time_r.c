/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the time_r functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "time.h"
#include "env.h"
#include "stdio.h"

int __vlibc_tzoff = 0;

static int parse_offset(const char *s)
{
    if (!s || !*s)
        return 0;
    while (*s && !((*s >= '0' && *s <= '9') || *s == '+' || *s == '-'))
        s++;
    int sign = 1;
    if (*s == '+') {
        sign = 1;
        s++;
    } else if (*s == '-') {
        sign = -1;
        s++;
    }
    int h = 0, m = 0;
    while (*s >= '0' && *s <= '9') {
        h = h * 10 + (*s - '0');
        s++;
    }
    if (*s == ':') {
        s++;
        while (*s >= '0' && *s <= '9') {
            m = m * 10 + (*s - '0');
            s++;
            if (m >= 60)
                break;
        }
    } else if (*s && *(s + 1) && (*s >= '0' && *s <= '9')) {
        m = (*s - '0') * 10 + (*(s + 1) - '0');
    }
    return sign * (h * 3600 + m * 60);
}

static int is_leap(int year)
{
    if ((year % 4) != 0)
        return 0;
    if ((year % 100) != 0)
        return 1;
    return (year % 400) == 0;
}

static const int days_per_month[2][12] = {
    {31,28,31,30,31,30,31,31,30,31,30,31},
    {31,29,31,30,31,30,31,31,30,31,30,31}
};

static void convert_tm(time_t t, struct tm *out)
{
    if (t < 0)
        t = 0;

    int sec = t % 60; t /= 60;
    int min = t % 60; t /= 60;
    int hour = t % 24; t /= 24;
    int days = (int)t;

    int wday = (days + 4) % 7; /* 1970-01-01 was Thursday */
    int year = 1970;
    while (1) {
        int ydays = is_leap(year) ? 366 : 365;
        if (days >= ydays) {
            days -= ydays;
            year++;
        } else {
            break;
        }
    }
    int yday = days;
    const int *ml = days_per_month[is_leap(year)];
    int mon = 0;
    while (days >= ml[mon]) {
        days -= ml[mon];
        mon++;
    }
    int mday = days + 1;

    out->tm_sec = sec;
    out->tm_min = min;
    out->tm_hour = hour;
    out->tm_mday = mday;
    out->tm_mon = mon;
    out->tm_year = year - 1900;
    out->tm_wday = wday;
    out->tm_yday = yday;
    out->tm_isdst = 0;
}

/*
 * Convert a time value to UTC broken-down form. The result is
 * stored in the user supplied structure and timezone handling
 * is not performed.
 */
struct tm *gmtime_r(const time_t *timep, struct tm *result)
{
    if (!result)
        return NULL;
    time_t t = timep ? *timep : time(NULL);
    convert_tm(t, result);
    return result;
}

/*
 * Convert a time value to local broken-down form applying the
 * configured timezone offset. tzset() controls the offset by
 * parsing the TZ environment variable or /etc/localtime.
 */
struct tm *localtime_r(const time_t *timep, struct tm *result)
{
    if (!result)
        return NULL;
    time_t t = timep ? *timep : time(NULL);
    t += __vlibc_tzoff;
    convert_tm(t, result);
    return result;
}

static void load_tz(const char *tz)
{
    __vlibc_tzoff = parse_offset(tz);
}

void tzset(void)
{
    const char *tz = getenv("TZ");
    if (!tz || !*tz) {
        FILE *f = fopen("/etc/localtime", "r");
        if (f) {
            char buf[64];
            if (fgets(buf, sizeof(buf), f))
                tz = buf;
            fclose(f);
        }
    }
    if (tz)
        load_tz(tz);
    else
        __vlibc_tzoff = 0;
}
