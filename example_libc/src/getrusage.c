/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the getrusage functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "sys/resource.h"
#include "errno.h"
#include <sys/syscall.h>
#include <unistd.h>
#include "syscall.h"

/*
 * getrusage() - retrieve resource usage statistics for the given target.
 * Performs a direct syscall when possible or falls back to the host libc
 * implementation.  Returns 0 on success and -1 on error with errno set.
 */
int getrusage(int who, struct rusage *usage)
{
#if defined(SYS_getrusage)
    long ret = vlibc_syscall(SYS_getrusage, who, (long)usage, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_getrusage(int, struct rusage *) __asm("getrusage");
    return host_getrusage(who, usage);
#else
    (void)who; (void)usage;
    errno = ENOSYS;
    return -1;
#endif
}
