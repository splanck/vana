/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the uname functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "sys/utsname.h"
#include "errno.h"
#include <sys/syscall.h>
#include <unistd.h>
#include "syscall.h"

#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
#include <sys/sysctl.h>
#include "string.h"

int uname(struct utsname *u)
{
    size_t n;
    n = sizeof(u->sysname);
    if (sysctlbyname("kern.ostype", u->sysname, &n, NULL, 0) < 0)
        return -1;
    if (n >= sizeof(u->sysname))
        u->sysname[sizeof(u->sysname) - 1] = '\0';

    n = sizeof(u->nodename);
    if (sysctlbyname("kern.hostname", u->nodename, &n, NULL, 0) < 0)
        return -1;
    if (n >= sizeof(u->nodename))
        u->nodename[sizeof(u->nodename) - 1] = '\0';

    n = sizeof(u->release);
    if (sysctlbyname("kern.osrelease", u->release, &n, NULL, 0) < 0)
        return -1;
    if (n >= sizeof(u->release))
        u->release[sizeof(u->release) - 1] = '\0';

    n = sizeof(u->version);
    if (sysctlbyname("kern.version", u->version, &n, NULL, 0) < 0)
        return -1;
    if (n >= sizeof(u->version))
        u->version[sizeof(u->version) - 1] = '\0';

    n = sizeof(u->machine);
    if (sysctlbyname("hw.machine", u->machine, &n, NULL, 0) < 0)
        return -1;
    if (n >= sizeof(u->machine))
        u->machine[sizeof(u->machine) - 1] = '\0';

    return 0;
}

#else

int uname(struct utsname *u)
{
#ifdef SYS_uname
    long ret = vlibc_syscall(SYS_uname, (long)u, 0, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
#else
    extern int host_uname(struct utsname *) __asm__("uname");
    return host_uname(u);
#endif
}
#endif

