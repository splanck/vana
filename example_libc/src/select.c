/*
 * BSD 2-Clause "Simplified" License
 *
 * Copyright (c) 2025
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Purpose: Implements the select and poll functions for vlibc.
 * Provides wrappers and helpers used by the standard library.
 */

/*
 * Linux wrappers for select(2) and poll(2) using raw syscalls. On BSD the
 * build relies on the system C library versions instead.
 */

#include "sys/select.h"
#include "time.h"
#include "errno.h"
#include <sys/syscall.h>
#include <unistd.h>
#include "syscall.h"

/* Wait for file descriptors to become ready. */
int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
           struct timeval *timeout)
{
#ifdef SYS_select
    long ret = vlibc_syscall(SYS_select, nfds, (long)readfds, (long)writefds,
                             (long)exceptfds, (long)timeout, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#elif defined(SYS_pselect6)
    struct timespec ts;
    struct timespec *pts = NULL;
    if (timeout) {
        ts.tv_sec = timeout->tv_sec;
        ts.tv_nsec = timeout->tv_usec * 1000;
        pts = &ts;
    }
    long ret = vlibc_syscall(SYS_pselect6, nfds, (long)readfds, (long)writefds,
                             (long)exceptfds, (long)pts, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    if (timeout && pts) {
        timeout->tv_sec = ts.tv_sec;
        timeout->tv_usec = ts.tv_nsec / 1000;
    }
    return (int)ret;
#else
    (void)nfds; (void)readfds; (void)writefds; (void)exceptfds; (void)timeout;
    errno = ENOSYS;
    return -1;
#endif
}

#include "poll.h"

/* Poll a set of file descriptors for the given timeout. */
int poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
#ifdef SYS_poll
    long ret = vlibc_syscall(SYS_poll, (long)fds, (long)nfds, timeout, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#elif defined(SYS_ppoll)
    struct timespec ts;
    struct timespec *pts = NULL;
    if (timeout >= 0) {
        ts.tv_sec = timeout / 1000;
        ts.tv_nsec = (timeout % 1000) * 1000000;
        pts = &ts;
    }
    long ret = vlibc_syscall(SYS_ppoll, (long)fds, (long)nfds, (long)pts, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#else
    (void)fds; (void)nfds; (void)timeout;
    errno = ENOSYS;
    return -1;
#endif
}
