/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the time functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "time.h"
#include "errno.h"
#include <sys/syscall.h>
#include <unistd.h>
#include "syscall.h"
#if !defined(SYS_time) && !defined(SYS_clock_gettime)
extern time_t host_time(time_t *t) __asm__("time");
extern int host_gettimeofday(struct timeval *tv, void *tz)
    __asm__("gettimeofday");
#endif

#ifdef SYS_clock_gettime
#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif
#endif

/*
 * Return seconds since the Unix epoch.  The wrapper uses the
 * best available kernel interface: SYS_time, SYS_clock_gettime
 * with CLOCK_REALTIME, or as a last resort the host time()
 * implementation.
 */
time_t time(time_t *t)
{
#ifdef SYS_time
    long ret = vlibc_syscall(SYS_time, (long)t, 0, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return (time_t)-1;
    }
    time_t sec = (time_t)ret;
    if (t)
        *t = sec;
    return sec;
#elif defined(SYS_clock_gettime)
    struct timespec ts;
    long ret = vlibc_syscall(SYS_clock_gettime, CLOCK_REALTIME, (long)&ts, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return (time_t)-1;
    }
    if (t)
        *t = ts.tv_sec;
    return ts.tv_sec;
#else
    return host_time(t);
#endif
}

/*
 * Fill in a timeval structure with the current time of day.
 * Like time(), this wrapper prefers SYS_time or
 * SYS_clock_gettime when available and otherwise relies on the
 * host gettimeofday().
 */
int gettimeofday(struct timeval *tv, void *tz)
{
    (void)tz;
#ifdef SYS_time
    long ret = vlibc_syscall(SYS_time, 0, 0, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    time_t sec = (time_t)ret;
    if (tv) {
        tv->tv_sec = sec;
        tv->tv_usec = 0;
    }
    return 0;
#elif defined(SYS_clock_gettime)
    struct timespec ts;
    long ret = vlibc_syscall(SYS_clock_gettime, CLOCK_REALTIME, (long)&ts, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    if (tv) {
        tv->tv_sec = ts.tv_sec;
        tv->tv_usec = ts.tv_nsec / 1000;
    }
    return 0;
#else
    return host_gettimeofday(tv, tz);
#endif
}

/*
 * Schedule delivery of SIGALRM after the given number of
 * seconds.  When the SYS_alarm syscall is missing the
 * functionality is emulated using setitimer().
 */
unsigned int alarm(unsigned int seconds)
{
#ifdef SYS_alarm
    long ret = vlibc_syscall(SYS_alarm, seconds, 0, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return 0;
    }
    return (unsigned int)ret;
#else
    struct itimerval it = { {0, 0}, { (time_t)seconds, 0 } };
    struct itimerval old;
    if (setitimer(ITIMER_REAL, &it, &old) < 0)
        return 0;
    return (unsigned int)old.it_value.tv_sec;
#endif
}
