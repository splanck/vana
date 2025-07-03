/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the time_conv functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "time.h"
#include "stdio.h"
#include "time.h"  /* for __vlibc_tzoff */

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

static __thread struct tm tm_buf;

/*
 * Wrapper around gmtime_r() that uses a thread-local
 * buffer to hold the result.
 */
struct tm *gmtime(const time_t *timep)
{
    return gmtime_r(timep, &tm_buf);
}

/*
 * Wrapper around localtime_r() using a thread-local buffer.
 * Timezone handling matches localtime_r.
 */
struct tm *localtime(const time_t *timep)
{
    return localtime_r(timep, &tm_buf);
}

/*
 * Convert broken-down local time to seconds since the epoch.
 * Daylight saving time information is ignored. The configured
 * timezone offset is subtracted so the returned time value is
 * in UTC.
 */
time_t mktime(struct tm *tm)
{
    if (!tm)
        return (time_t)-1;

    int year = tm->tm_year + 1900;
    time_t days = 0;
    for (int y = 1970; y < year; y++)
        days += is_leap(y) ? 366 : 365;

    const int *ml = days_per_month[is_leap(year)];
    for (int m = 0; m < tm->tm_mon; m++)
        days += ml[m];
    days += tm->tm_mday - 1;

    tm->tm_yday = (int)days;
    tm->tm_wday = (int)((days + 4) % 7);
    tm->tm_isdst = 0;

    time_t t = days * 86400 + tm->tm_hour * 3600 + tm->tm_min * 60 + tm->tm_sec;
    t -= __vlibc_tzoff;
    return t;
}

/* Non-standard conversion from broken-down UTC to time_t. */
time_t timegm(struct tm *tm)
{
    if (!tm)
        return (time_t)-1;

    int year = tm->tm_year + 1900;
    time_t days = 0;
    for (int y = 1970; y < year; y++)
        days += is_leap(y) ? 366 : 365;

    const int *ml = days_per_month[is_leap(year)];
    for (int m = 0; m < tm->tm_mon; m++)
        days += ml[m];
    days += tm->tm_mday - 1;

    tm->tm_yday = (int)days;
    tm->tm_wday = (int)((days + 4) % 7);
    tm->tm_isdst = 0;

    time_t t = days * 86400 + tm->tm_hour * 3600 + tm->tm_min * 60 + tm->tm_sec;
    return t;
}

/*
 * Format a time value in a human readable form using localtime().
 * The result is stored in a thread-local buffer.
 */
char *ctime(const time_t *timep)
{
    static __thread char buf[32];
    struct tm *tm = localtime(timep);
    static const char *wd[7] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    static const char *mn[12] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
    if (!tm)
        return NULL;
    char dd[3] = {0}, hh[3] = {0}, mm[3] = {0}, ss[3] = {0};
    dd[0] = '0' + (tm->tm_mday / 10);
    dd[1] = '0' + (tm->tm_mday % 10);
    hh[0] = '0' + (tm->tm_hour / 10);
    hh[1] = '0' + (tm->tm_hour % 10);
    mm[0] = '0' + (tm->tm_min / 10);
    mm[1] = '0' + (tm->tm_min % 10);
    ss[0] = '0' + (tm->tm_sec / 10);
    ss[1] = '0' + (tm->tm_sec % 10);
    snprintf(buf, sizeof(buf), "%s %s %s %s:%s:%s %d\n",
             wd[tm->tm_wday], mn[tm->tm_mon], dd, hh, mm, ss,
             tm->tm_year + 1900);
    return buf;
}

/*
 * Reentrant conversion of a broken-down time structure to the
 * standard ASCII representation. The caller supplies the buffer
 * which must hold at least 26 characters.
 */
char *asctime_r(const struct tm *tm, char *buf)
{
    static const char *wd[7] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    static const char *mn[12] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug",
                                 "Sep","Oct","Nov","Dec"};
    if (!tm || !buf)
        return NULL;
    char dd[3] = {0}, hh[3] = {0}, mm[3] = {0}, ss[3] = {0};
    dd[0] = '0' + (tm->tm_mday / 10);
    dd[1] = '0' + (tm->tm_mday % 10);
    hh[0] = '0' + (tm->tm_hour / 10);
    hh[1] = '0' + (tm->tm_hour % 10);
    mm[0] = '0' + (tm->tm_min / 10);
    mm[1] = '0' + (tm->tm_min % 10);
    ss[0] = '0' + (tm->tm_sec / 10);
    ss[1] = '0' + (tm->tm_sec % 10);
    snprintf(buf, 26, "%s %s %s %s:%s:%s %d\n",
             wd[tm->tm_wday], mn[tm->tm_mon], dd, hh, mm, ss,
             tm->tm_year + 1900);
    return buf;
}

/*
 * Wrapper around asctime_r() using a thread-local buffer.
 */
char *asctime(const struct tm *tm)
{
    static __thread char buf[32];
    return asctime_r(tm, buf);
}

/*
 * Return the difference between two time values in seconds as a
 * double precision floating point number.
 */
double difftime(time_t end, time_t start)
{
    return (double)end - (double)start;
}
