/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the chroot functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "unistd.h"
#include "errno.h"
#include <sys/syscall.h>
#include "syscall.h"

/*
 * chroot() - change the process root directory via SYS_chroot when available.
 * On platforms without the syscall a host helper may be used. Returns 0 on
 * success or -1 with errno set.
 */
int chroot(const char *path)
{
#ifdef SYS_chroot
    long ret = vlibc_syscall(SYS_chroot, (long)path, 0, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
#else
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_chroot(const char *) __asm__("chroot");
    return host_chroot(path);
#else
    (void)path;
    errno = ENOSYS;
    return -1;
#endif
#endif
}
