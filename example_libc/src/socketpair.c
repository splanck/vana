/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the socketpair functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "sys/socket.h"
#include "errno.h"
#include <sys/syscall.h>
#include <unistd.h>
#include "syscall.h"

/* Create a pair of connected sockets */
int socketpair(int domain, int type, int protocol, int sv[2])
{
    long ret = vlibc_syscall(SYS_socketpair, domain, type, protocol,
                             (long)sv, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
}
