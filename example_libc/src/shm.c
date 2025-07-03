/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements POSIX shared memory wrappers for vlibc.
 */

#include "sys/shm.h"
#include "errno.h"
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "stdio.h"
#include "stdlib.h"
#include "syscall.h"

int shm_open(const char *name, int oflag, mode_t mode)
{
#ifdef SYS_shm_open
    long ret = vlibc_syscall(SYS_shm_open, (long)name, oflag, mode, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_shm_open(const char *, int, mode_t) __asm__("shm_open");
    return host_shm_open(name, oflag, mode);
#else
    if (!name || name[0] == '\0') {
        errno = EINVAL;
        return -1;
    }
    if (name[0] != '/') {
        errno = EINVAL;
        return -1;
    }
    size_t len = strlen(name) + sizeof("/dev/shm") + 1;
    char *path = malloc(len);
    if (!path) {
        errno = ENOMEM;
        return -1;
    }
    snprintf(path, len, "/dev/shm%s", name);
    int fd = open(path, oflag | O_CLOEXEC, mode);
    free(path);
#ifdef O_CLOEXEC
    if (fd >= 0 && !(oflag & O_CLOEXEC))
        fcntl(fd, F_SETFD, FD_CLOEXEC);
#endif
    return fd;
#endif
}

int shm_unlink(const char *name)
{
#ifdef SYS_shm_unlink
    long ret = vlibc_syscall(SYS_shm_unlink, (long)name, 0, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_shm_unlink(const char *) __asm__("shm_unlink");
    return host_shm_unlink(name);
#else
    if (!name || name[0] == '\0') {
        errno = EINVAL;
        return -1;
    }
    if (name[0] != '/') {
        errno = EINVAL;
        return -1;
    }
    size_t len = strlen(name) + sizeof("/dev/shm") + 1;
    char *path = malloc(len);
    if (!path) {
        errno = ENOMEM;
        return -1;
    }
    snprintf(path, len, "/dev/shm%s", name);
    int ret = unlink(path);
    free(path);
    return ret;
#endif
}
