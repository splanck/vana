/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the socket_msg functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "sys/socket.h"
#include "errno.h"
#include <sys/syscall.h>
#include <unistd.h>
#include "syscall.h"

/*
 * sendmsg() - send a message along with optional ancillary data.
 * Uses SYS_sendmsg when available and falls back to a host
 * implementation when necessary. Returns bytes sent or -1
 * on error with errno set.
 */
ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags)
{
#ifdef SYS_sendmsg
    long ret = vlibc_syscall(SYS_sendmsg, sockfd, (long)msg, flags, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (ssize_t)ret;
#else
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    extern ssize_t host_sendmsg(int, const struct msghdr *, int) __asm__("sendmsg");
    return host_sendmsg(sockfd, msg, flags);
#else
    (void)sockfd; (void)msg; (void)flags;
    errno = ENOSYS;
    return -1;
#endif
#endif
}

/*
 * recvmsg() - receive a message and any ancillary data from a socket.
 * Uses SYS_recvmsg when available and falls back to a host
 * implementation when necessary. Returns bytes received or -1
 * on error with errno set.
 */
ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags)
{
#ifdef SYS_recvmsg
    long ret = vlibc_syscall(SYS_recvmsg, sockfd, (long)msg, flags, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (ssize_t)ret;
#else
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    extern ssize_t host_recvmsg(int, struct msghdr *, int) __asm__("recvmsg");
    return host_recvmsg(sockfd, msg, flags);
#else
    (void)sockfd; (void)msg; (void)flags;
    errno = ENOSYS;
    return -1;
#endif
#endif
}

