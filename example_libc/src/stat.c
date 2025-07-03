/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the stat functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "sys/stat.h"
#include "errno.h"
#include <sys/syscall.h>
#include <unistd.h>
#include "syscall.h"

int stat(const char *path, struct stat *buf)
{
    long ret = vlibc_syscall(SYS_stat, (long)path, (long)buf, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
}

int fstat(int fd, struct stat *buf)
{
    long ret = vlibc_syscall(SYS_fstat, fd, (long)buf, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
}

int lstat(const char *path, struct stat *buf)
{
    long ret = vlibc_syscall(SYS_lstat, (long)path, (long)buf, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
}
