/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the socket functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "sys/socket.h"
#include "errno.h"
#include <sys/syscall.h>
#include <unistd.h>
#include "syscall.h"
#include "vlibc_features.h"

/* Create a new socket */
int socket(int domain, int type, int protocol)
{
    long ret = vlibc_syscall(SYS_socket, domain, type, protocol, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
}

/* Bind a socket to a local address */
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    long ret = vlibc_syscall(SYS_bind, sockfd, (long)addr, addrlen, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
}

/* Listen for incoming connections */
int listen(int sockfd, int backlog)
{
    long ret = vlibc_syscall(SYS_listen, sockfd, backlog, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
}

/* Accept a connection on a listening socket */
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
#if VLIBC_HAVE_ACCEPT4
    long ret = vlibc_syscall(SYS_accept4, sockfd, (long)addr, (long)addrlen, 0, 0, 0);
#else
    long ret = vlibc_syscall(SYS_accept, sockfd, (long)addr, (long)addrlen, 0, 0, 0);
#endif
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
}

/* Accept a connection with additional flags */
int accept4(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags)
{
#if VLIBC_HAVE_ACCEPT4
    long ret = vlibc_syscall(SYS_accept4, sockfd, (long)addr, (long)addrlen, flags, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#else
    if (flags != 0) {
        errno = EINVAL;
        return -1;
    }
    return accept(sockfd, addr, addrlen);
#endif
}

/* Connect to a remote address */
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    long ret = vlibc_syscall(SYS_connect, sockfd, (long)addr, addrlen, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
}

/* Send data on a connected socket */
ssize_t send(int sockfd, const void *buf, size_t len, int flags)
{
    long ret = vlibc_syscall(SYS_sendto, sockfd, (long)buf, len, flags, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (ssize_t)ret;
}

/* Receive data from a connected socket */
ssize_t recv(int sockfd, void *buf, size_t len, int flags)
{
    long ret = vlibc_syscall(SYS_recvfrom, sockfd, (long)buf, len, flags, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (ssize_t)ret;
}

/* Send a message to a specific destination */
ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest, socklen_t addrlen)
{
    long ret = vlibc_syscall(SYS_sendto, sockfd, (long)buf, len, flags,
                             (long)dest, addrlen);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (ssize_t)ret;
}

/* Receive a message from a specific source */
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src, socklen_t *addrlen)
{
    long ret = vlibc_syscall(SYS_recvfrom, sockfd, (long)buf, len, flags,
                             (long)src, (long)addrlen);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (ssize_t)ret;
}

/* Retrieve the local address of a socket */
int getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
#ifdef SYS_getsockname
    long ret = vlibc_syscall(SYS_getsockname, sockfd, (long)addr,
                             (long)addrlen, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#else
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_getsockname(int, struct sockaddr *, socklen_t *)
        __asm__("getsockname");
    return host_getsockname(sockfd, addr, addrlen);
#else
    (void)sockfd; (void)addr; (void)addrlen; errno = ENOSYS; return -1;
#endif
#endif
}

/* Retrieve the remote address of a connected socket */
int getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
#ifdef SYS_getpeername
    long ret = vlibc_syscall(SYS_getpeername, sockfd, (long)addr,
                             (long)addrlen, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#else
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_getpeername(int, struct sockaddr *, socklen_t *)
        __asm__("getpeername");
    return host_getpeername(sockfd, addr, addrlen);
#else
    (void)sockfd; (void)addr; (void)addrlen; errno = ENOSYS; return -1;
#endif
#endif
}

/* Shutdown all or part of a full-duplex connection */
int shutdown(int sockfd, int how)
{
#ifdef SYS_shutdown
    long ret = vlibc_syscall(SYS_shutdown, sockfd, how, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#else
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_shutdown(int, int) __asm__("shutdown");
    return host_shutdown(sockfd, how);
#else
    (void)sockfd; (void)how; errno = ENOSYS; return -1;
#endif
#endif
}
