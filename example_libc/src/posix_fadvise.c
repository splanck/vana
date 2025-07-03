/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that the copyright notice and
 * this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements posix_fadvise for vlibc. When SYS_posix_fadvise is
 * available the syscall is invoked directly. On other systems the call
 * succeeds without applying any hint.
 */

#include "fcntl.h"
#include "errno.h"
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "syscall.h"

int posix_fadvise(int fd, off_t offset, off_t len, int advice)
{
#ifdef SYS_posix_fadvise
    long ret = vlibc_syscall(SYS_posix_fadvise, fd, (long)offset,
                             (long)len, advice, 0);
    if (ret < 0)
        return (int)-ret;
    return 0;
#else
    (void)fd; (void)offset; (void)len; (void)advice;
    return 0;
#endif
}
