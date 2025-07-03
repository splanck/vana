/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms,
 * with or without modification, are permitted provided that the copyright
 * notice and this permission notice appear in all copies. This software is
 * provided "as is" without warranty.
 *
 * Purpose: Implements the sync function for vlibc. Flushes all buffered file
 * data to disk using the SYS_sync syscall when available or the host
 * implementation on BSD systems.
 */

#include "unistd.h"
#include "errno.h"
#include <sys/syscall.h>
#include "syscall.h"

void sync(void)
{
#ifdef SYS_sync
    long ret = vlibc_syscall(SYS_sync, 0, 0, 0, 0, 0, 0);
    (void)ret; /* ret is ignored as sync returns void */
#else
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    extern void host_sync(void) __asm__("sync");
    host_sync();
#else
    errno = ENOSYS;
#endif
#endif
}
