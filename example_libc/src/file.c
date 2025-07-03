/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the file functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "io.h"
#include "errno.h"
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include "sys/file.h"
#ifndef AT_FDCWD
#define AT_FDCWD -100
#endif
#include "syscall.h"

int unlink(const char *pathname)
{
    long ret = vlibc_syscall(SYS_unlinkat, AT_FDCWD, (long)pathname, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
}

int rename(const char *oldpath, const char *newpath)
{
    long ret = vlibc_syscall(SYS_renameat, AT_FDCWD, (long)oldpath, AT_FDCWD,
                             (long)newpath, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
}

/*
 * renameat wrapper calling SYS_renameat with BSD fallback when
 * the syscall is unavailable.
 */
int renameat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath)
{
#ifdef SYS_renameat
    long ret = vlibc_syscall(SYS_renameat, olddirfd, (long)oldpath,
                             newdirfd, (long)newpath, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#else
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_renameat(int, const char *, int, const char *) __asm__("renameat");
    return host_renameat(olddirfd, oldpath, newdirfd, newpath);
#else
    (void)olddirfd; (void)oldpath; (void)newdirfd; (void)newpath;
    errno = ENOSYS;
    return -1;
#endif
#endif
}

int link(const char *oldpath, const char *newpath)
{
    long ret = vlibc_syscall(SYS_linkat, AT_FDCWD, (long)oldpath, AT_FDCWD,
                             (long)newpath, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
}

/*
 * linkat wrapper issuing SYS_linkat with a BSD fallback when
 * the syscall is missing.
 */
int linkat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath,
           int flags)
{
#ifdef SYS_linkat
    long ret = vlibc_syscall(SYS_linkat, olddirfd, (long)oldpath, newdirfd,
                             (long)newpath, flags);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#else
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_linkat(int, const char *, int, const char *, int) __asm__("linkat");
    return host_linkat(olddirfd, oldpath, newdirfd, newpath, flags);
#else
    (void)olddirfd; (void)oldpath; (void)newdirfd; (void)newpath; (void)flags;
    errno = ENOSYS;
    return -1;
#endif
#endif
}

int symlink(const char *target, const char *linkpath)
{
    long ret = vlibc_syscall(SYS_symlinkat, (long)target, AT_FDCWD,
                             (long)linkpath, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
}

ssize_t readlink(const char *pathname, char *buf, size_t bufsiz)
{
    long ret = vlibc_syscall(SYS_readlinkat, AT_FDCWD, (long)pathname,
                             (long)buf, bufsiz, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (ssize_t)ret;
}

int chdir(const char *path)
{
    long ret = vlibc_syscall(SYS_chdir, (long)path, 0, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
}

int sendfile(int fd, int s, off_t offset, size_t nbytes,
             struct sf_hdtr *hdtr, off_t *sbytes, int flags)
{
#if defined(__NetBSD__)
    extern int host_sendfile(int, int, off_t, size_t,
                             struct sf_hdtr *, off_t *, int)
        __asm("sendfile");
    int hret = host_sendfile(fd, s, offset, nbytes, hdtr, sbytes, flags);
    if (hret >= 0)
        return hret;
    if (errno != ENOSYS && errno != EINVAL && errno != EOPNOTSUPP &&
        errno != ENOTSOCK) {
        return -1;
    }
#endif
#if defined(SYS_sendfile) && !defined(__linux__)
    long ret = vlibc_syscall(SYS_sendfile, fd, s, offset, nbytes,
                             (long)hdtr, (long)sbytes);
    if (ret >= 0)
        return (int)ret;
    if (ret != -ENOSYS && ret != -EINVAL && ret != -EOPNOTSUPP &&
        ret != -ENOTSOCK) {
        errno = -ret;
        return -1;
    }
#endif

    (void)hdtr; (void)flags;
    off_t sent = 0;
    char buf[8192];
    while (sent < (off_t)nbytes) {
        size_t chunk = nbytes - (size_t)sent;
        if (chunk > sizeof(buf))
            chunk = sizeof(buf);
        ssize_t r = pread(fd, buf, chunk, offset + sent);
        if (r <= 0)
            break;
        size_t off = 0;
        while (off < (size_t)r) {
            ssize_t w = write(s, buf + off, (size_t)r - off);
            if (w < 0) {
                if (errno == EINTR || errno == EAGAIN)
                    continue;
                if (sbytes)
                    *sbytes = sent;
                return -1;
            }
            off += (size_t)w;
            sent += w;
        }
    }
    if (sbytes) {
        *sbytes = sent;
        return 0;
    }
    return (int)sent;
}
