/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the sysconf functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "unistd.h"
#include "errno.h"

/*
 * On most platforms the system provides a sysconf() implementation.
 * Expose it so we can fall back to it when needed.
 */
extern long host_sysconf(int name) __asm("sysconf");
extern int host_getpagesize(void) __asm("getpagesize");

int getpagesize(void)
{
#ifdef PAGE_SIZE
    return PAGE_SIZE;
#elif defined(PAGESIZE)
    return PAGESIZE;
#else
# if defined(_SC_PAGESIZE)
    long r = host_sysconf(_SC_PAGESIZE);
    if (r > 0)
        return (int)r;
# elif defined(_SC_PAGE_SIZE)
    long r = host_sysconf(_SC_PAGE_SIZE);
    if (r > 0)
        return (int)r;
# endif
    /* default to 4096 if everything else fails */
    return 4096;
#endif
}

long sysconf(int name)
{
    switch (name) {
#ifdef _SC_PAGESIZE
    case _SC_PAGESIZE:
#endif
#if defined(_SC_PAGE_SIZE) && _SC_PAGE_SIZE != _SC_PAGESIZE
    case _SC_PAGE_SIZE:
#endif
        return getpagesize();
#ifdef _SC_OPEN_MAX
    case _SC_OPEN_MAX:
        return host_sysconf(name);
#endif
#ifdef _SC_NPROCESSORS_ONLN
    case _SC_NPROCESSORS_ONLN:
        return host_sysconf(name);
#endif
#ifdef _SC_NPROCESSORS_CONF
    case _SC_NPROCESSORS_CONF:
        return host_sysconf(name);
#endif
#ifdef _SC_CLK_TCK
    case _SC_CLK_TCK:
        return host_sysconf(name);
#endif
    default:
        return host_sysconf(name);
    }
}
