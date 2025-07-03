/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that the copyright notice and this
 * permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements posix_fallocate for vlibc. Attempts to use the
 * posix_fallocate syscall when available and otherwise emulates it
 * using ftruncate and writes to ensure the space is allocated.
 */

#include "sys/stat.h"
#include "io.h"
#include "errno.h"
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "syscall.h"

#ifndef SYS_posix_fallocate
/* some systems expose fallocate instead of posix_fallocate */
#endif

int posix_fallocate(int fd, off_t offset, off_t len)
{
#ifdef SYS_posix_fallocate
    long ret = vlibc_syscall(SYS_posix_fallocate, fd, (long)offset,
                             (long)len, 0, 0, 0);
    if (ret < 0)
        return (int)-ret;
    return 0;
#else
    if (offset < 0 || len < 0)
        return EINVAL;

    off_t end = offset + len;
    if (end < offset)
        return EFBIG;

    struct stat st;
    if (fstat(fd, &st) < 0)
        return errno;

    if (st.st_size < end && ftruncate(fd, end) < 0)
        return errno;

    char zero[4096] = {0};
    off_t pos = offset;
    while (pos < end) {
        size_t chunk = end - pos;
        if (chunk > sizeof(zero))
            chunk = sizeof(zero);
        ssize_t r = pwrite(fd, zero, chunk, pos);
        if (r < 0)
            return errno;
        pos += r;
    }
    return 0;
#endif
}
