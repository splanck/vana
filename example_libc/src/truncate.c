/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the truncate functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "io.h"
#include "errno.h"
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "syscall.h"

#ifndef SYS_truncate
extern int host_truncate(const char *path, off_t length) __asm__("truncate");
#endif
#ifndef SYS_ftruncate
extern int host_ftruncate(int fd, off_t length) __asm__("ftruncate");
#endif

int truncate(const char *path, off_t length)
{
#ifdef SYS_truncate
    long ret = vlibc_syscall(SYS_truncate, (long)path, (long)length, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#else
    return host_truncate(path, length);
#endif
}

int ftruncate(int fd, off_t length)
{
#ifdef SYS_ftruncate
    long ret = vlibc_syscall(SYS_ftruncate, fd, (long)length, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#else
    return host_ftruncate(fd, length);
#endif
}
