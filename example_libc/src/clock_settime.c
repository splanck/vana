/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the clock_settime function for vlibc. Provides wrappers and helpers used by the standard library.
 */

#include "time.h"
#include "errno.h"
#include <sys/syscall.h>
#include <unistd.h>
#include "syscall.h"

int clock_settime(int clk_id, const struct timespec *ts)
{
#ifdef SYS_clock_settime
    long ret = vlibc_syscall(SYS_clock_settime, clk_id, (long)ts, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    if (clk_id != CLOCK_REALTIME || !ts) {
        errno = EINVAL;
        return -1;
    }
    struct timeval tv;
    tv.tv_sec = ts->tv_sec;
    tv.tv_usec = ts->tv_nsec / 1000;
    extern int host_settimeofday(const struct timeval *, const void *) __asm__("settimeofday");
    return host_settimeofday(&tv, NULL);
#else
    (void)clk_id; (void)ts;
    errno = ENOSYS;
    return -1;
#endif
}
