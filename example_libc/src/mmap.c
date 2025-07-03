/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the mmap functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "sys/mman.h"
#include "errno.h"
#include <sys/syscall.h>
#include <unistd.h>
#include "syscall.h"

/*
 * Call the system implementations of mmap(2), munmap(2) and mprotect(2)
 * rather than issuing raw syscalls directly.  On BSD the raw syscall
 * interface may differ from Linux, so using the libc functions ensures
 * compatibility.
 */

/*
 * On Linux we can invoke the kernel syscalls directly.  BSD systems may
 * not provide the same raw interface, so fall back to the host C library
 * implementations when required.
 */

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
#ifdef SYS_mmap
    long ret = vlibc_syscall(SYS_mmap, (long)addr, length, prot,
                             flags, fd, offset);
    if (ret < 0) {
        errno = -ret;
        return MAP_FAILED;
    }
    return (void *)ret;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern void *host_mmap(void *, size_t, int, int, int, off_t) __asm__("mmap");
    return host_mmap(addr, length, prot, flags, fd, offset);
#else
    (void)addr; (void)length; (void)prot; (void)flags; (void)fd; (void)offset;
    errno = ENOSYS;
    return MAP_FAILED;
#endif
}

int munmap(void *addr, size_t length)
{
#ifdef SYS_munmap
    long ret = vlibc_syscall(SYS_munmap, (long)addr, length, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_munmap(void *, size_t) __asm__("munmap");
    return host_munmap(addr, length);
#else
    (void)addr; (void)length;
    errno = ENOSYS;
    return -1;
#endif
}

int mprotect(void *addr, size_t length, int prot)
{
#ifdef SYS_mprotect
    long ret = vlibc_syscall(SYS_mprotect, (long)addr, length, prot, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_mprotect(void *, size_t, int) __asm__("mprotect");
    return host_mprotect(addr, length, prot);
#else
    (void)addr; (void)length; (void)prot;
    errno = ENOSYS;
    return -1;
#endif
}
