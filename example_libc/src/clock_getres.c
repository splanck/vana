/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the clock_getres functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "time.h"
#include "errno.h"
#include <sys/syscall.h>
#include <unistd.h>
#include "syscall.h"

/*
 * Return the resolution of the specified clock. When the
 * SYS_clock_getres syscall exists it is used directly. On
 * BSD platforms the host clock_getres implementation provides
 * the value. Otherwise ENOSYS is returned.
 */
int clock_getres(int clk_id, struct timespec *res)
{
#ifdef SYS_clock_getres
    long ret = vlibc_syscall(SYS_clock_getres, clk_id, (long)res, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_clock_getres(int, struct timespec *) __asm__("clock_getres");
    return host_clock_getres(clk_id, res);
#else
    (void)clk_id; (void)res;
    errno = ENOSYS;
    return -1;
#endif
}
