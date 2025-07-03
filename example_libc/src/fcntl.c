/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the fcntl functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "fcntl.h"
#include <stdarg.h>
#include "errno.h"
#include <sys/syscall.h>
#include <unistd.h>
#include "syscall.h"

/*
 * fcntl() - wrapper around the fcntl system call.
 * Handles optional third argument and propagates errno
 * on failure.
 */
int fcntl(int fd, int cmd, ...)
{
    long arg = 0;
    switch (cmd) {
    case F_DUPFD:
    case F_DUPFD_CLOEXEC:
    case F_SETFD:
    case F_SETFL:
        {
            va_list ap;
            va_start(ap, cmd);
            arg = va_arg(ap, long);
            va_end(ap);
        }
        break;
    default:
        /* commands without third argument */
        break;
    }
#ifdef SYS_fcntl
    long ret = vlibc_syscall(SYS_fcntl, fd, cmd, arg, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#else
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_fcntl(int, int, ...) __asm__("fcntl");
    return host_fcntl(fd, cmd, arg);
#else
    (void)fd; (void)cmd; (void)arg; errno = ENOSYS; return -1;
#endif
#endif
}
