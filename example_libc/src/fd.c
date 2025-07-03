/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the fd functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "io.h"
#include "errno.h"
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include "syscall.h"
#include "vlibc_features.h"

#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif
#ifndef O_NONBLOCK
#define O_NONBLOCK 0
#endif

/*
 * lseek() - reposition the offset of the open file descriptor FD.
 * Returns the resulting offset or (off_t)-1 on error with errno set.
 */
off_t lseek(int fd, off_t offset, int whence)
{
    long ret = vlibc_syscall(SYS_lseek, fd, (long)offset, whence, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return (off_t)-1;
    }
    return (off_t)ret;
}

/*
 * dup() - duplicate OLDFD, returning a new file descriptor.
 */
int dup(int oldfd)
{
    long ret = vlibc_syscall(SYS_dup, oldfd, 0, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
}

/*
 * dup2() - duplicate OLDFD to NEWFD. When VLIBC_HAVE_DUP3 is defined
 * we use dup3; otherwise dup2 is employed.
 */
int dup2(int oldfd, int newfd)
{
#if VLIBC_HAVE_DUP3
    long ret = vlibc_syscall(SYS_dup3, oldfd, newfd, 0, 0, 0, 0);
#else
    long ret = vlibc_syscall(SYS_dup2, oldfd, newfd, 0, 0, 0, 0);
#endif
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
}

/*
 * pipe() - create a unidirectional data channel returning two file
 * descriptors in PIPEFD.
 */
int pipe(int pipefd[2])
{
#if VLIBC_HAVE_PIPE2
    long ret = vlibc_syscall(SYS_pipe2, (long)pipefd, 0, 0, 0, 0, 0);
#else
    long ret = vlibc_syscall(SYS_pipe, (long)pipefd, 0, 0, 0, 0, 0);
#endif
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
}

/*
 * dup3() - duplicate OLDFD to NEWFD with the given FLAGS.  When the
 * kernel provides dup3 this directly invokes it; otherwise behaviour
 * is emulated.
 */
int dup3(int oldfd, int newfd, int flags)
{
#if VLIBC_HAVE_DUP3
    long ret = vlibc_syscall(SYS_dup3, oldfd, newfd, flags, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#else
    if (flags & ~(O_CLOEXEC)) {
        errno = EINVAL;
        return -1;
    }
#ifdef F_DUPFD_CLOEXEC
    if (flags & O_CLOEXEC)
        return fcntl(oldfd, F_DUPFD_CLOEXEC, newfd);
    else
        return fcntl(oldfd, F_DUPFD, newfd);
#else
    int fd = dup2(oldfd, newfd);
    if (fd < 0)
        return -1;
    if (flags & O_CLOEXEC)
        fcntl(fd, F_SETFD, FD_CLOEXEC);
    return fd;
#endif
#endif
}

/*
 * pipe2() - create a pipe with FLAGS controlling non-blocking and
 * close-on-exec behaviour.  Falls back to pipe() when pipe2 is not
 * available.
 */
int pipe2(int pipefd[2], int flags)
{
#if VLIBC_HAVE_PIPE2
    long ret = vlibc_syscall(SYS_pipe2, (long)pipefd, flags, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
#else
    if (pipe(pipefd) < 0)
        return -1;
    int remain = flags;
#ifdef O_CLOEXEC
    if (flags & O_CLOEXEC) {
        fcntl(pipefd[0], F_SETFD, FD_CLOEXEC);
        fcntl(pipefd[1], F_SETFD, FD_CLOEXEC);
        remain &= ~O_CLOEXEC;
    }
#endif
#ifdef O_NONBLOCK
    if (flags & O_NONBLOCK) {
        int fl0 = fcntl(pipefd[0], F_GETFL);
        int fl1 = fcntl(pipefd[1], F_GETFL);
        fcntl(pipefd[0], F_SETFL, fl0 | O_NONBLOCK);
        fcntl(pipefd[1], F_SETFL, fl1 | O_NONBLOCK);
        remain &= ~O_NONBLOCK;
    }
#endif
    if (remain != 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        errno = EINVAL;
        return -1;
    }
    return 0;
#endif
}
