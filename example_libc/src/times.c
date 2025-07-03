/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the times wrapper for vlibc. Provides process CPU time information.
 */

#include "sys/times.h"
#include "errno.h"
#include <sys/syscall.h>
#include <unistd.h>
#include "syscall.h"
#include "sys/resource.h"

#ifndef CLK_TCK_FALLBACK
#define CLK_TCK_FALLBACK 100
#endif

clock_t times(struct tms *buf)
{
#ifdef SYS_times
    struct tms ktms;
    struct tms *arg = buf ? buf : &ktms;
    long ret = vlibc_syscall(SYS_times, (long)arg, 0, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return (clock_t)-1;
    }
    if (buf) {
        struct rusage ru;
        long hz = sysconf(_SC_CLK_TCK);
        if (hz <= 0)
            hz = CLK_TCK_FALLBACK;
        if (getrusage(RUSAGE_SELF, &ru) == 0) {
            buf->tms_utime = ru.ru_utime.tv_sec * hz +
                             (ru.ru_utime.tv_usec > 0);
            buf->tms_stime = ru.ru_stime.tv_sec * hz +
                             (ru.ru_stime.tv_usec > 0);
            buf->tms_cutime = ktms.tms_cutime;
            buf->tms_cstime = ktms.tms_cstime;
        } else if (arg != buf) {
            *buf = ktms;
        }
    }
    return (clock_t)ret;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern clock_t host_times(struct tms *) __asm("times");
    return host_times(buf);
#else
    (void)buf;
    errno = ENOSYS;
    return (clock_t)-1;
#endif
}
