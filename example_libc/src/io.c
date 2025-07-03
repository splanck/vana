/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the io functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "io.h"
#include "errno.h"
#include <sys/types.h>
#include "sys/uio.h"
#include <sys/syscall.h>
#include <unistd.h>
#include "syscall.h"
#include <fcntl.h>
#include <stdarg.h>
#include "sys/stat.h"

#ifndef AT_FDCWD
#define AT_FDCWD -100
#endif

/*
 * Wrapper for open(2) that issues the SYS_open system call
 * via vlibc_syscall. When SYS_open is unavailable it falls
 * back to SYS_openat with AT_FDCWD.
 */
int open(const char *path, int flags, ...)
{
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list ap;
        va_start(ap, flags);
        mode = va_arg(ap, mode_t);
        va_end(ap);
    }
#ifdef SYS_open
    long ret = vlibc_syscall(SYS_open, (long)path, flags, mode, 0, 0, 0);
#else
    long ret = vlibc_syscall(SYS_openat, AT_FDCWD, (long)path, flags, mode, 0, 0);
#endif
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
}

/*
 * Read from a file descriptor using the SYS_read system call
 * via vlibc_syscall.
 */
ssize_t read(int fd, void *buf, size_t count)
{
    long ret = vlibc_syscall(SYS_read, fd, (long)buf, count, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (ssize_t)ret;
}

/*
 * Write to a file descriptor using the SYS_write system call
 * via vlibc_syscall.
 */
ssize_t write(int fd, const void *buf, size_t count)
{
    long ret = vlibc_syscall(SYS_write, fd, (long)buf, count, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (ssize_t)ret;
}

/*
 * Vector read wrapper. Uses SYS_readv via vlibc_syscall when
 * available. On BSD or when the syscall is missing, the host
 * readv function or a simple loop emulates the behavior.
 */
ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
{
#ifdef SYS_readv
    long ret = vlibc_syscall(SYS_readv, fd, (long)iov, iovcnt, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (ssize_t)ret;
#else
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    extern ssize_t host_readv(int, const struct iovec *, int) __asm__("readv");
    return host_readv(fd, iov, iovcnt);
#else
    ssize_t total = 0;
    for (int i = 0; i < iovcnt; i++) {
        const char *base = (const char *)iov[i].iov_base;
        size_t len = iov[i].iov_len;
        size_t off = 0;
        while (off < len) {
            ssize_t r = read(fd, (void *)(base + off), len - off);
            if (r < 0)
                return total ? total : -1;
            if (r == 0)
                return total;
            off += (size_t)r;
            total += r;
        }
    }
    return total;
#endif
#endif
}

/*
 * Vector write wrapper. Uses SYS_writev via vlibc_syscall when
 * available. On BSD or when the syscall is missing, the host
 * writev implementation or a manual loop is used instead.
 */
ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
{
#ifdef SYS_writev
    long ret = vlibc_syscall(SYS_writev, fd, (long)iov, iovcnt, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (ssize_t)ret;
#else
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    extern ssize_t host_writev(int, const struct iovec *, int) __asm__("writev");
    return host_writev(fd, iov, iovcnt);
#else
    ssize_t total = 0;
    for (int i = 0; i < iovcnt; i++) {
        const char *base = (const char *)iov[i].iov_base;
        size_t len = iov[i].iov_len;
        size_t off = 0;
        while (off < len) {
            ssize_t w = write(fd, base + off, len - off);
            if (w < 0) {
                if (errno == EINTR)
                    continue;
                if (errno == EAGAIN)
                    return total ? total : -1;
                return total ? total : -1;
            }
            off += (size_t)w;
            total += w;
        }
    }
    return total;
#endif
#endif
}

/*
 * Positional read wrapper. Invokes SYS_pread or SYS_pread64
 * through vlibc_syscall and falls back to the host pread
 * function on BSD when unavailable.
 */
ssize_t pread(int fd, void *buf, size_t count, off_t offset)
{
#ifdef SYS_pread
    long ret = vlibc_syscall(SYS_pread, fd, (long)buf, count, offset, 0, 0);
#elif defined(SYS_pread64)
    long ret = vlibc_syscall(SYS_pread64, fd, (long)buf, count, offset, 0, 0);
#else
    extern ssize_t host_pread(int, void *, size_t, off_t) __asm__("pread");
    return host_pread(fd, buf, count, offset);
#endif
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (ssize_t)ret;
}

/*
 * Positional write wrapper. Invokes SYS_pwrite or SYS_pwrite64
 * through vlibc_syscall and falls back to the host pwrite
 * function on BSD when unavailable.
 */
ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset)
{
#ifdef SYS_pwrite
    long ret = vlibc_syscall(SYS_pwrite, fd, (long)buf, count, offset, 0, 0);
#elif defined(SYS_pwrite64)
    long ret = vlibc_syscall(SYS_pwrite64, fd, (long)buf, count, offset, 0, 0);
#else
    extern ssize_t host_pwrite(int, const void *, size_t, off_t) __asm__("pwrite");
    return host_pwrite(fd, buf, count, offset);
#endif
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (ssize_t)ret;
}

/*
 * Vector positional read wrapper. Uses SYS_preadv or SYS_preadv2 when
 * available and otherwise falls back to the host implementation or a
 * simple loop using pread.
 */
ssize_t preadv(int fd, const struct iovec *iov, int iovcnt, off_t offset)
{
#ifdef SYS_preadv
    long ret = vlibc_syscall(SYS_preadv, fd, (long)iov, iovcnt, offset, 0, 0);
#elif defined(SYS_preadv2)
    long ret = vlibc_syscall(SYS_preadv2, fd, (long)iov, iovcnt, offset, 0, 0);
#else
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    extern ssize_t host_preadv(int, const struct iovec *, int, off_t) __asm__("preadv");
    return host_preadv(fd, iov, iovcnt, offset);
#else
    ssize_t total = 0;
    for (int i = 0; i < iovcnt; i++) {
        const char *base = (const char *)iov[i].iov_base;
        size_t len = iov[i].iov_len;
        size_t off = 0;
        while (off < len) {
            ssize_t r = pread(fd, (void *)(base + off), len - off, offset + total);
            if (r < 0)
                return total ? total : -1;
            if (r == 0)
                return total;
            off += (size_t)r;
            total += r;
        }
    }
    return total;
#endif
#endif
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (ssize_t)ret;
}

/*
 * Vector positional write wrapper. Uses SYS_pwritev or SYS_pwritev2 when
 * available and falls back to the host implementation or a manual loop
 * around pwrite.
 */
ssize_t pwritev(int fd, const struct iovec *iov, int iovcnt, off_t offset)
{
#ifdef SYS_pwritev
    long ret = vlibc_syscall(SYS_pwritev, fd, (long)iov, iovcnt, offset, 0, 0);
#elif defined(SYS_pwritev2)
    long ret = vlibc_syscall(SYS_pwritev2, fd, (long)iov, iovcnt, offset, 0, 0);
#else
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    extern ssize_t host_pwritev(int, const struct iovec *, int, off_t) __asm__("pwritev");
    return host_pwritev(fd, iov, iovcnt, offset);
#else
    ssize_t total = 0;
    for (int i = 0; i < iovcnt; i++) {
        const char *base = (const char *)iov[i].iov_base;
        size_t len = iov[i].iov_len;
        size_t off = 0;
        while (off < len) {
            ssize_t w = pwrite(fd, base + off, len - off, offset + total);
            if (w < 0) {
                if (errno == EINTR)
                    continue;
                if (errno == EAGAIN)
                    return total ? total : -1;
                return total ? total : -1;
            }
            off += (size_t)w;
            total += w;
        }
    }
    return total;
#endif
#endif
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (ssize_t)ret;
}

/* Close a file descriptor via SYS_close using vlibc_syscall. */
int close(int fd)
{
    long ret = vlibc_syscall(SYS_close, fd, 0, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
}

/*
 * Wrapper for openat(2). Uses vlibc_syscall with SYS_openat
 * and falls back to the host openat on BSD if the syscall
 * is unavailable.
 */
int openat(int dirfd, const char *path, int flags, ...)
{
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list ap;
        va_start(ap, flags);
        mode = va_arg(ap, mode_t);
        va_end(ap);
    }
#ifdef SYS_openat
    long ret = vlibc_syscall(SYS_openat, dirfd, (long)path, flags, mode, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#else
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_openat(int, const char *, int, ...) __asm__("openat");
    return host_openat(dirfd, path, flags, mode);
#else
    (void)dirfd; (void)path; (void)flags; (void)mode;
    errno = ENOSYS;
    return -1;
#endif
#endif
}

/*
 * fstatat wrapper issuing SYS_fstatat through vlibc_syscall.
 * On BSD when the syscall is missing, it calls the host
 * fstatat implementation.
 */
int fstatat(int dirfd, const char *path, struct stat *buf, int flags)
{
#ifdef SYS_fstatat
    long ret = vlibc_syscall(SYS_fstatat, dirfd, (long)path, (long)buf, flags, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#else
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_fstatat(int, const char *, struct stat *, int) __asm__("fstatat");
    return host_fstatat(dirfd, path, buf, flags);
#else
    (void)dirfd; (void)path; (void)buf; (void)flags;
    errno = ENOSYS;
    return -1;
#endif
#endif
}

/*
 * unlinkat wrapper using vlibc_syscall with SYS_unlinkat.
 * BSD builds call the host unlinkat when the syscall is
 * not available.
 */
int unlinkat(int dirfd, const char *pathname, int flags)
{
#ifdef SYS_unlinkat
    long ret = vlibc_syscall(SYS_unlinkat, dirfd, (long)pathname, flags, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#else
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_unlinkat(int, const char *, int) __asm__("unlinkat");
    return host_unlinkat(dirfd, pathname, flags);
#else
    (void)dirfd; (void)pathname; (void)flags;
    errno = ENOSYS;
    return -1;
#endif
#endif
}

/*
 * Create a directory relative to dirfd using vlibc_syscall
 * with SYS_mkdirat. Falls back to the host mkdirat call on
 * BSD systems when the syscall is missing.
 */
int mkdirat(int dirfd, const char *pathname, mode_t mode)
{
#ifdef SYS_mkdirat
    long ret = vlibc_syscall(SYS_mkdirat, dirfd, (long)pathname, mode, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#else
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_mkdirat(int, const char *, mode_t) __asm__("mkdirat");
    return host_mkdirat(dirfd, pathname, mode);
#else
    (void)dirfd; (void)pathname; (void)mode;
    errno = ENOSYS;
    return -1;
#endif
#endif
}

/*
 * symlinkat wrapper issuing SYS_symlinkat via vlibc_syscall.
 * On BSD the host symlinkat is used when the syscall is not
 * available.
 */
int symlinkat(const char *target, int dirfd, const char *linkpath)
{
#ifdef SYS_symlinkat
    long ret = vlibc_syscall(SYS_symlinkat, (long)target, dirfd, (long)linkpath, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#else
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_symlinkat(const char *, int, const char *) __asm__("symlinkat");
    return host_symlinkat(target, dirfd, linkpath);
#else
    (void)target; (void)dirfd; (void)linkpath;
    errno = ENOSYS;
    return -1;
#endif
#endif
}

/*
 * Change the current working directory using a descriptor. Always
 * invoke the SYS_fchdir syscall when available.
 */
int fchdir(int fd)
{
#ifdef SYS_fchdir
    long ret = vlibc_syscall(SYS_fchdir, fd, 0, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#else
    (void)fd; errno = ENOSYS; return -1;
#endif
}
