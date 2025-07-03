/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the clock_gettime functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "time.h"
#include "errno.h"
#include <sys/syscall.h>
#include <unistd.h>
#include "syscall.h"

/*
 * Fetch the current time for the provided clock ID. When
 * SYS_clock_gettime is present the syscall is invoked. If not
 * available the function falls back to gettimeofday() which
 * only supplies CLOCK_REALTIME.
 */
int clock_gettime(int clk_id, struct timespec *ts)
{
#ifdef SYS_clock_gettime
    long ret = vlibc_syscall(SYS_clock_gettime, clk_id, (long)ts, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
#else
    struct timeval tv;
    if (gettimeofday(&tv, NULL) < 0)
        return -1;
    if (ts) {
        ts->tv_sec = tv.tv_sec;
        ts->tv_nsec = tv.tv_usec * 1000;
    }
    return 0;
#endif
}
