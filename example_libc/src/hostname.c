/*
 * BSD 2-Clause "Simplified" License
 *
 * Copyright (c) 2025, vlibc authors
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
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
 * Purpose: Implements the hostname functions for vlibc.
 */

#include "env.h"
#include "errno.h"
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include "syscall.h"

/*
 * gethostname() - retrieve the current host name.
 *
 * The caller supplies NAME, a buffer of length LEN, in which the
 * system's host name will be stored.  The function wraps the
 * SYS_gethostname syscall when available and falls back to a host
 * helper otherwise.  A return value of 0 indicates success while -1
 * means an error occurred and errno is set.
 */
int gethostname(char *name, size_t len)
{
#ifdef SYS_gethostname
    long ret = vlibc_syscall(SYS_gethostname, (long)name, len, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
#else
    if (!name || len == 0) {
        errno = EINVAL;
        return -1;
    }

    int fd = open("/proc/sys/kernel/hostname", O_RDONLY);
    if (fd < 0)
        fd = open("/etc/hostname", O_RDONLY);
    if (fd < 0)
        return -1;

    ssize_t r = read(fd, name, len - 1);
    close(fd);
    if (r < 0)
        return -1;

    size_t n = (size_t)r;
    if (n > 0 && name[n - 1] == '\n')
        n--;
    name[n] = '\0';
    return 0;
#endif
}

/*
 * sethostname() - change the system host name.
 *
 * NAME specifies the new host name with length LEN.  When the
 * SYS_sethostname syscall is available it is invoked directly; on
 * other systems a host helper may be used.  The function returns 0 on
 * success or -1 when an error occurs, with errno indicating the cause.
 */
int sethostname(const char *name, size_t len)
{
#ifdef SYS_sethostname
    long ret = vlibc_syscall(SYS_sethostname, (long)name, len, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
#else
    extern int host_sethostname(const char *, size_t) __asm__("sethostname");
    return host_sethostname(name, len);
#endif
}
