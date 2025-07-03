/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the rlimit functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "sys/resource.h"
#include "errno.h"
#include <sys/syscall.h>
#include <unistd.h>
#include "syscall.h"

int getrlimit(int resource, struct rlimit *rlim)
{
#if defined(SYS_getrlimit)
    long ret = vlibc_syscall(SYS_getrlimit, resource, (long)rlim, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_getrlimit(int, struct rlimit *) __asm("getrlimit");
    return host_getrlimit(resource, rlim);
#else
    (void)resource; (void)rlim;
    errno = ENOSYS;
    return -1;
#endif
}

int setrlimit(int resource, const struct rlimit *rlim)
{
#if defined(SYS_setrlimit)
    long ret = vlibc_syscall(SYS_setrlimit, resource, (long)rlim, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_setrlimit(int, const struct rlimit *) __asm("setrlimit");
    return host_setrlimit(resource, rlim);
#else
    (void)resource; (void)rlim;
    errno = ENOSYS;
    return -1;
#endif
}
