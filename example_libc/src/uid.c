/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the uid functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "unistd.h"
#include "errno.h"
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "syscall.h"

#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
extern uid_t host_getuid(void) __asm("getuid");
extern uid_t host_geteuid(void) __asm("geteuid");
extern gid_t host_getgid(void) __asm("getgid");
extern gid_t host_getegid(void) __asm("getegid");
#endif

/*
 * getuid() - return real user id using SYS_getuid or a host wrapper. On
 * failure -1 is returned and errno set.
 */
uid_t getuid(void)
{
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    return host_getuid();
#else
    long ret = vlibc_syscall(SYS_getuid, 0, 0, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return (uid_t)-1;
    }
    return (uid_t)ret;
#endif
}

/*
 * geteuid() - obtain effective user id via SYS_geteuid or a host wrapper.
 * Returns -1 on error with errno set.
 */
uid_t geteuid(void)
{
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    return host_geteuid();
#else
    long ret = vlibc_syscall(SYS_geteuid, 0, 0, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return (uid_t)-1;
    }
    return (uid_t)ret;
#endif
}

/*
 * getgid() - fetch real group id via SYS_getgid or host wrapper.
 */
gid_t getgid(void)
{
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    return host_getgid();
#else
    long ret = vlibc_syscall(SYS_getgid, 0, 0, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return (gid_t)-1;
    }
    return (gid_t)ret;
#endif
}

/*
 * getegid() - fetch effective group id via SYS_getegid or host wrapper.
 */
gid_t getegid(void)
{
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    return host_getegid();
#else
    long ret = vlibc_syscall(SYS_getegid, 0, 0, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return (gid_t)-1;
    }
    return (gid_t)ret;
#endif
}

/*
 * setuid() - change real and effective uid using SYS_setuid or a host
 * implementation. Returns 0 on success or -1 with errno set.
 */
int setuid(uid_t uid)
{
#if defined(SYS_setuid)
    long ret = vlibc_syscall(SYS_setuid, uid, 0, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_setuid(uid_t) __asm("setuid");
    return host_setuid(uid);
#else
    (void)uid;
    errno = ENOSYS;
    return -1;
#endif
}

/*
 * seteuid() - set effective uid via SYS_seteuid or host implementation.
 * Returns 0 on success or -1 with errno set.
 */
int seteuid(uid_t euid)
{
#if defined(SYS_seteuid)
    long ret = vlibc_syscall(SYS_seteuid, euid, 0, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_seteuid(uid_t) __asm("seteuid");
    return host_seteuid(euid);
#else
    (void)euid;
    errno = ENOSYS;
    return -1;
#endif
}

/*
 * setgid() - change real and effective group id using SYS_setgid or host
 * wrapper. Returns 0 on success or -1 with errno set.
 */
int setgid(gid_t gid)
{
#if defined(SYS_setgid)
    long ret = vlibc_syscall(SYS_setgid, gid, 0, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_setgid(gid_t) __asm("setgid");
    return host_setgid(gid);
#else
    (void)gid;
    errno = ENOSYS;
    return -1;
#endif
}

/*
 * setegid() - set effective group id via SYS_setegid or host wrapper.
 * Returns 0 on success or -1 with errno set.
 */
int setegid(gid_t egid)
{
#if defined(SYS_setegid)
    long ret = vlibc_syscall(SYS_setegid, egid, 0, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_setegid(gid_t) __asm("setegid");
    return host_setegid(egid);
#else
    (void)egid;
    errno = ENOSYS;
    return -1;
#endif
}
