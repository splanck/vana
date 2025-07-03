/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms,
 * with or without modification, are permitted provided that the copyright
 * notice and this permission notice appear in all copies. This software is
 * provided "as is" without warranty.
 *
 * Purpose: Implements fsync and fdatasync for vlibc. Uses vlibc_syscall
 * when possible and falls back to the host implementations on BSD.
 */

#include "io.h"
#include "errno.h"
#include <sys/syscall.h>
#include <unistd.h>
#include "syscall.h"

#ifndef SYS_fsync
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
extern int host_fsync(int) __asm__("fsync");
#endif
#endif

#ifndef SYS_fdatasync
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
extern int host_fdatasync(int) __asm__("fdatasync");
#endif
#endif

int fsync(int fd)
{
#ifdef SYS_fsync
    long ret = vlibc_syscall(SYS_fsync, fd, 0, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#else
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    return host_fsync(fd);
#else
    (void)fd;
    errno = ENOSYS;
    return -1;
#endif
#endif
}

int fdatasync(int fd)
{
#ifdef SYS_fdatasync
    long ret = vlibc_syscall(SYS_fdatasync, fd, 0, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#else
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    return host_fdatasync(fd);
#else
    (void)fd;
    errno = ENOSYS;
    return -1;
#endif
#endif
}
