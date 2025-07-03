/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the itimer functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "time.h"
#include "errno.h"
#include <sys/syscall.h>
#include <unistd.h>
#include "syscall.h"

int setitimer(int which, const struct itimerval *new, struct itimerval *old)
{
#ifdef SYS_setitimer
    long ret = vlibc_syscall(SYS_setitimer, which, (long)new, (long)old, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_setitimer(int, const struct itimerval *, struct itimerval *)
        __asm__("setitimer");
    return host_setitimer(which, new, old);
#else
    (void)which; (void)new; (void)old;
    errno = ENOSYS;
    return -1;
#endif
}

int getitimer(int which, struct itimerval *curr)
{
#ifdef SYS_getitimer
    long ret = vlibc_syscall(SYS_getitimer, which, (long)curr, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_getitimer(int, struct itimerval *) __asm__("getitimer");
    return host_getitimer(which, curr);
#else
    (void)which; (void)curr;
    errno = ENOSYS;
    return -1;
#endif
}
