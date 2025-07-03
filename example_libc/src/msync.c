/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the msync functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "sys/mman.h"
#include "errno.h"
#include <sys/syscall.h>
#include <unistd.h>
#include "syscall.h"

int msync(void *addr, size_t length, int flags)
{
#ifdef SYS_msync
    long ret = vlibc_syscall(SYS_msync, (long)addr, length, flags, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_msync(void *, size_t, int) __asm__("msync");
    return host_msync(addr, length, flags);
#else
    (void)addr; (void)length; (void)flags;
    errno = ENOSYS;
    return -1;
#endif
}
