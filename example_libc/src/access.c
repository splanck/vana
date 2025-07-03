/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the access functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "io.h"
#include "errno.h"
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "syscall.h"

/*
 * Wrapper for access(2). Passes pathname and mode directly to SYS_access
 * when available or to a host fallback. Returns 0 on success or -1 with
 * errno set to the negative syscall result.
 */
int access(const char *pathname, int mode)
{
#ifdef SYS_access
    long ret = vlibc_syscall(SYS_access, (long)pathname, mode, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
#else
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_access(const char *, int) __asm__("access");
    return host_access(pathname, mode);
#else
    (void)pathname; (void)mode; errno = ENOSYS; return -1;
#endif
#endif
}

/*
 * Wrapper for faccessat(2). The dirfd, pathname, mode and flags
 * arguments are forwarded to SYS_faccessat or a host fallback.
 * Returns 0 on success or -1 with errno reflecting the syscall
 * error value.
 */
int faccessat(int dirfd, const char *pathname, int mode, int flags)
{
#ifdef SYS_faccessat
    long ret = vlibc_syscall(SYS_faccessat, dirfd, (long)pathname, mode, flags, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
#else
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_faccessat(int, const char *, int, int) __asm__("faccessat");
    return host_faccessat(dirfd, pathname, mode, flags);
#else
    (void)dirfd; (void)pathname; (void)mode; (void)flags; errno = ENOSYS; return -1;
#endif
#endif
}
