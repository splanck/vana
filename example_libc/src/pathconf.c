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
 * Purpose: Implements the pathconf functions for vlibc. Provides wrappers and helpers used by the standard library.
 */

#include "unistd.h"
#include "sys/statvfs.h"
#include "errno.h"
#include <limits.h>

/*
 * pathconf() - query configuration limits for a pathname.
 *
 * Only the _PC_NAME_MAX and _PC_PATH_MAX names are supported.  When
 * requesting NAME_MAX the function uses statvfs() to obtain the value
 * for PATH.  The limit is returned on success or -1 on failure with
 * errno set.
 */
long pathconf(const char *path, int name)
{
    struct statvfs sv;
    switch (name) {
    case _PC_NAME_MAX:
        if (statvfs(path, &sv) < 0)
            return -1;
        return (long)sv.f_namemax;
    case _PC_PATH_MAX:
#ifdef PATH_MAX
        return PATH_MAX;
#else
        return 4096;
#endif
    default:
        errno = EINVAL;
        return -1;
    }
}

/*
 * fpathconf() - query configuration limits for an open file descriptor.
 *
 * This variant operates on FD instead of a pathname and mirrors the
 * semantics of pathconf().  It supports _PC_NAME_MAX and _PC_PATH_MAX
 * using fstatvfs() when necessary.  The limit is returned or -1 on
 * error with errno set.
 */
long fpathconf(int fd, int name)
{
    struct statvfs sv;
    switch (name) {
    case _PC_NAME_MAX:
        if (fstatvfs(fd, &sv) < 0)
            return -1;
        return (long)sv.f_namemax;
    case _PC_PATH_MAX:
#ifdef PATH_MAX
        return PATH_MAX;
#else
        return 4096;
#endif
    default:
        errno = EINVAL;
        return -1;
    }
}
