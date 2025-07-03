/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the statvfs functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "sys/statvfs.h"
#include "errno.h"
#include <sys/syscall.h>
#include <unistd.h>
#include "syscall.h"

#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)

extern int host_statvfs(const char *, struct statvfs *) __asm__("statvfs");
extern int host_fstatvfs(int, struct statvfs *) __asm__("fstatvfs");

int statvfs(const char *path, struct statvfs *buf)
{
    return host_statvfs(path, buf);
}

int fstatvfs(int fd, struct statvfs *buf)
{
    return host_fstatvfs(fd, buf);
}

#else

#include <sys/statfs.h>

static void cvt(struct statvfs *out, const struct statfs *in)
{
    out->f_bsize = in->f_bsize;
#ifdef _STATFS_F_FRSIZE
    out->f_frsize = in->f_frsize;
#else
    out->f_frsize = in->f_bsize;
#endif
    out->f_blocks = in->f_blocks;
    out->f_bfree = in->f_bfree;
    out->f_bavail = in->f_bavail;
    out->f_files = in->f_files;
    out->f_ffree = in->f_ffree;
    out->f_favail = in->f_ffree;
#ifdef __FSID_T_TYPE
    out->f_fsid = ((unsigned long)in->f_fsid.__val[0] & 0xffffffffUL) |
                  ((unsigned long)in->f_fsid.__val[1] << 32);
#else
    out->f_fsid = (unsigned long)in->f_fsid;
#endif
#ifdef _STATFS_F_FLAGS
    out->f_flag = in->f_flags;
#else
    out->f_flag = 0;
#endif
#ifdef _STATFS_F_NAMELEN
    out->f_namemax = in->f_namelen;
#else
    out->f_namemax = 255;
#endif
    out->f_type = in->f_type;
    for (int i = 0; i < 5; i++)
        out->__f_spare[i] = 0;
}

int statvfs(const char *path, struct statvfs *buf)
{
#ifdef SYS_statfs
    struct statfs s;
    long ret = vlibc_syscall(SYS_statfs, (long)path, (long)&s, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    cvt(buf, &s);
    return 0;
#else
    (void)path; (void)buf;
    errno = ENOSYS;
    return -1;
#endif
}

int fstatvfs(int fd, struct statvfs *buf)
{
#ifdef SYS_fstatfs
    struct statfs s;
    long ret = vlibc_syscall(SYS_fstatfs, fd, (long)&s, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    cvt(buf, &s);
    return 0;
#else
    (void)fd; (void)buf;
    errno = ENOSYS;
    return -1;
#endif
}

#endif
