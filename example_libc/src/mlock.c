/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms,
 * with or without modification, are permitted provided that the copyright
 * notice and this permission notice appear in all copies. This software is
 * provided "as is" without warranty.
 *
 * Purpose: Implements memory locking helpers for vlibc. Uses vlibc_syscall
 * when possible and falls back to the host implementations on BSD.
 */

#include "sys/mman.h"
#include "errno.h"
#include <sys/syscall.h>
#include <unistd.h>
#include "syscall.h"

int mlock(const void *addr, size_t length)
{
#ifdef SYS_mlock
    long ret = vlibc_syscall(SYS_mlock, (long)addr, length, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_mlock(const void *, size_t) __asm__("mlock");
    return host_mlock(addr, length);
#else
    (void)addr; (void)length;
    errno = ENOSYS;
    return -1;
#endif
}

int munlock(const void *addr, size_t length)
{
#ifdef SYS_munlock
    long ret = vlibc_syscall(SYS_munlock, (long)addr, length, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_munlock(const void *, size_t) __asm__("munlock");
    return host_munlock(addr, length);
#else
    (void)addr; (void)length;
    errno = ENOSYS;
    return -1;
#endif
}

int mlockall(int flags)
{
#ifdef SYS_mlockall
    long ret = vlibc_syscall(SYS_mlockall, flags, 0, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_mlockall(int) __asm__("mlockall");
    return host_mlockall(flags);
#else
    (void)flags;
    errno = ENOSYS;
    return -1;
#endif
}

int munlockall(void)
{
#ifdef SYS_munlockall
    long ret = vlibc_syscall(SYS_munlockall, 0, 0, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_munlockall(void) __asm__("munlockall");
    return host_munlockall();
#else
    errno = ENOSYS;
    return -1;
#endif
}

int madvise(void *addr, size_t length, int advice)
{
#ifdef SYS_madvise
    long ret = vlibc_syscall(SYS_madvise, (long)addr, length, advice, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_madvise(void *, size_t, int) __asm__("madvise");
    return host_madvise(addr, length, advice);
#else
    (void)addr; (void)length; (void)advice;
    errno = ENOSYS;
    return -1;
#endif
}

