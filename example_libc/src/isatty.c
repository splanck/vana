/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the isatty functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "unistd.h"
#include "errno.h"
#include <sys/syscall.h>
#include <unistd.h>
#include "syscall.h"
#include <sys/ioctl.h>
#ifndef __has_include
#define __has_include(x) 0
#endif
#if __has_include(<termios.h>)
#include <termios.h>
#endif

/*
 * isatty() - determine if a file descriptor refers to a terminal.
 * When the SYS_isatty system call is available it is invoked directly.
 * Otherwise tcgetattr() from <termios.h> is used if present. If neither
 * method can be used the function sets errno to ENOTTY and returns 0.
 */
int isatty(int fd)
{
#ifdef SYS_isatty
    long ret = vlibc_syscall(SYS_isatty, fd, 0, 0, 0, 0, 0);
    if (ret >= 0) {
        if (ret == 0)
            errno = ENOTTY;
        return (int)ret;
    }
    if (ret != -ENOSYS) {
        errno = -ret;
        return 0;
    }
#endif
#if __has_include(<termios.h>)
    struct termios t;
#ifdef SYS_ioctl
    long r = vlibc_syscall(SYS_ioctl, fd, TCGETS, (long)&t, 0, 0, 0);
    if (r < 0) {
        errno = -r;
        return 0;
    }
    return 1;
#else
    if (ioctl(fd, TCGETS, &t) == -1)
        return 0;
    return 1;
#endif
#else
    (void)fd;
    errno = ENOTTY;
    return 0;
#endif
}
