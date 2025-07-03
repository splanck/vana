/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the mknod functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "sys/stat.h"
#include "errno.h"
#include <sys/syscall.h>
#include <unistd.h>
#include "syscall.h"

#ifndef SYS_mknod
extern int host_mknod(const char *path, mode_t mode, dev_t dev) __asm__("mknod");
#endif

#ifndef SYS_mknodat
extern int host_mknodat(int dirfd, const char *path, mode_t mode, dev_t dev) __asm__("mknodat");
#endif

int mknod(const char *path, mode_t mode, dev_t dev)
{
#ifdef SYS_mknod
    long ret = vlibc_syscall(SYS_mknod, (long)path, mode, dev, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#else
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    return host_mknod(path, mode, dev);
#else
    (void)path; (void)mode; (void)dev; errno = ENOSYS; return -1;
#endif
#endif
}

int mknodat(int dirfd, const char *path, mode_t mode, dev_t dev)
{
#ifdef SYS_mknodat
    long ret = vlibc_syscall(SYS_mknodat, dirfd, (long)path, mode, dev, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#else
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    return host_mknodat(dirfd, path, mode, dev);
#else
    (void)dirfd; (void)path; (void)mode; (void)dev;
    errno = ENOSYS;
    return -1;
#endif
#endif
}
