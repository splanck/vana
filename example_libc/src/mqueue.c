/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that the copyright notice and
 * this permission notice appear in all copies. This software is provided "as is"
 * without warranty.
 *
 * Purpose: Implements POSIX message queue wrappers for vlibc.
 *
 * Copyright (c) 2025
 */

#include "mqueue.h"
#include "errno.h"
#include <sys/syscall.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include "time.h"
#include "poll.h"
#include "fcntl.h"
#include "syscall.h"
#include <limits.h>

#if defined(SYS_mq_timedsend_time64) || defined(SYS_mq_timedsend) || \
    defined(SYS_mq_timedreceive_time64) || defined(SYS_mq_timedreceive) || \
    defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)

static inline __attribute__((unused))
int mq_wait(mqd_t mqdes, short events, const struct timespec *abstime)
{
    (void)mqdes;
    (void)events;
    (void)abstime;
    return 0;
}

#else

static __attribute__((unused))
int mq_wait(mqd_t mqdes, short events, const struct timespec *abstime)
{
    struct timespec now;
    while (1) {
        int timeout = -1;
        if (abstime) {
            clock_gettime(CLOCK_REALTIME, &now);
            if (now.tv_sec > abstime->tv_sec ||
                (now.tv_sec == abstime->tv_sec &&
                 now.tv_nsec >= abstime->tv_nsec)) {
                errno = ETIMEDOUT;
                return -1;
            }
            long ms = (abstime->tv_sec - now.tv_sec) * 1000 +
                       (abstime->tv_nsec - now.tv_nsec) / 1000000;
            if (ms < 0)
                ms = 0;
            /* Clamp to INT_MAX to avoid overflow when casting */
            if (ms > INT_MAX)
                ms = INT_MAX;
            timeout = (int)ms;
        }
        struct pollfd pfd = { mqdes, events, 0 };
        int r = poll(&pfd, 1, timeout);
        if (r < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (r == 0) {
            errno = ETIMEDOUT;
            return -1;
        }
        return 0;
    }
}

#endif

/* Open or create a POSIX message queue. */
mqd_t mq_open(const char *name, int oflag, ...)
{
    mode_t mode = 0;
    struct mq_attr *attr = NULL;
    if (oflag & O_CREAT) {
        va_list ap;
        va_start(ap, oflag);
        mode = va_arg(ap, mode_t);
        attr = va_arg(ap, struct mq_attr *);
        va_end(ap);
    }
#ifdef SYS_mq_open
    /* Linux mq_open syscall expects the queue name without the leading '/'. */
    const char *qname = name && name[0] == '/' ? name + 1 : name;
    long ret = vlibc_syscall(SYS_mq_open, (long)qname, oflag, mode, (long)attr, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return (mqd_t)-1;
    }
    return (mqd_t)ret;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern mqd_t host_mq_open(const char *, int, ...) __asm__("mq_open");
    if (oflag & O_CREAT)
        return host_mq_open(name, oflag, mode, attr);
    return host_mq_open(name, oflag);
#else
    (void)name; (void)oflag; (void)mode; (void)attr;
    errno = ENOSYS;
    return (mqd_t)-1;
#endif
}

/* Close an opened message queue descriptor. */
int mq_close(mqd_t mqdes)
{
#ifdef SYS_mq_close
    long ret = vlibc_syscall(SYS_mq_close, mqdes, 0, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_mq_close(mqd_t) __asm__("mq_close");
    return host_mq_close(mqdes);
#else
    /* On Linux there is no mq_close syscall; use close(2). */
    return close(mqdes);
#endif
}

/* Remove a message queue name from the system. */
int mq_unlink(const char *name)
{
#ifdef SYS_mq_unlink
    const char *qname = name && name[0] == '/' ? name + 1 : name;
    long ret = vlibc_syscall(SYS_mq_unlink, (long)qname, 0, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_mq_unlink(const char *) __asm__("mq_unlink");
    return host_mq_unlink(name);
#else
    (void)name;
    errno = ENOSYS;
    return -1;
#endif
}

/* Send a message to the queue. */
int mq_send(mqd_t mqdes, const char *msg_ptr, size_t msg_len, unsigned msg_prio)
{
#ifdef SYS_mq_send
    long ret = vlibc_syscall(SYS_mq_send, mqdes, (long)msg_ptr, msg_len, msg_prio, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#elif defined(SYS_mq_timedsend_time64)
    long ret = vlibc_syscall(SYS_mq_timedsend_time64, mqdes, (long)msg_ptr,
                             msg_len, msg_prio, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#elif defined(SYS_mq_timedsend)
    long ret = vlibc_syscall(SYS_mq_timedsend, mqdes, (long)msg_ptr,
                             msg_len, msg_prio, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_mq_send(mqd_t, const char *, size_t, unsigned) __asm__("mq_send");
    return host_mq_send(mqdes, msg_ptr, msg_len, msg_prio);
#else
    (void)mqdes; (void)msg_ptr; (void)msg_len; (void)msg_prio;
    errno = ENOSYS;
    return -1;
#endif
}

/* Receive the next message from the queue. */
ssize_t mq_receive(mqd_t mqdes, char *msg_ptr, size_t msg_len, unsigned *msg_prio)
{
#ifdef SYS_mq_receive
    long ret = vlibc_syscall(SYS_mq_receive, mqdes, (long)msg_ptr, msg_len, (long)msg_prio, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (ssize_t)ret;
#elif defined(SYS_mq_timedreceive_time64)
    long ret = vlibc_syscall(SYS_mq_timedreceive_time64, mqdes, (long)msg_ptr,
                             msg_len, (long)msg_prio, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (ssize_t)ret;
#elif defined(SYS_mq_timedreceive)
    long ret = vlibc_syscall(SYS_mq_timedreceive, mqdes, (long)msg_ptr,
                             msg_len, (long)msg_prio, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (ssize_t)ret;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern ssize_t host_mq_receive(mqd_t, char *, size_t, unsigned *) __asm__("mq_receive");
    return host_mq_receive(mqdes, msg_ptr, msg_len, msg_prio);
#else
    (void)mqdes; (void)msg_ptr; (void)msg_len; (void)msg_prio;
    errno = ENOSYS;
    return -1;
#endif
}

/* Retrieve message queue attributes. */
int mq_getattr(mqd_t mqdes, struct mq_attr *attr)
{
#ifdef SYS_mq_getsetattr
    long ret = vlibc_syscall(SYS_mq_getsetattr, mqdes, 0, (long)attr, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_mq_getattr(mqd_t, struct mq_attr *) __asm__("mq_getattr");
    return host_mq_getattr(mqdes, attr);
#else
    (void)mqdes; (void)attr;
    errno = ENOSYS;
    return -1;
#endif
}

/* Set message queue attributes. */
int mq_setattr(mqd_t mqdes, const struct mq_attr *attr, struct mq_attr *oattr)
{
#ifdef SYS_mq_getsetattr
    long ret = vlibc_syscall(SYS_mq_getsetattr, mqdes, (long)attr,
                             (long)oattr, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_mq_setattr(mqd_t, const struct mq_attr *, struct mq_attr *)
        __asm__("mq_setattr");
    return host_mq_setattr(mqdes, attr, oattr);
#else
    (void)mqdes; (void)attr; (void)oattr;
    errno = ENOSYS;
    return -1;
#endif
}

/* Send with a timeout. */
int mq_timedsend(mqd_t mqdes, const char *msg_ptr, size_t msg_len,
                 unsigned msg_prio, const struct timespec *abstime)
{
#ifdef SYS_mq_timedsend_time64
    long ret = vlibc_syscall(SYS_mq_timedsend_time64, mqdes, (long)msg_ptr,
                             msg_len, msg_prio, (long)abstime);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#elif defined(SYS_mq_timedsend)
    long ret = vlibc_syscall(SYS_mq_timedsend, mqdes, (long)msg_ptr,
                             msg_len, msg_prio, (long)abstime);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_mq_timedsend(mqd_t, const char *, size_t, unsigned,
                                 const struct timespec *) __asm__("mq_timedsend");
    return host_mq_timedsend(mqdes, msg_ptr, msg_len, msg_prio, abstime);
#else
    if (!abstime)
        return mq_send(mqdes, msg_ptr, msg_len, msg_prio);
    int flags = fcntl(mqdes, F_GETFL, 0);
    if (flags >= 0 && !(flags & O_NONBLOCK))
        fcntl(mqdes, F_SETFL, flags | O_NONBLOCK);
    for (;;) {
        int r = mq_send(mqdes, msg_ptr, msg_len, msg_prio);
        if (r == 0)
            break;
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            if (flags >= 0 && !(flags & O_NONBLOCK))
                fcntl(mqdes, F_SETFL, flags);
            return -1;
        }
        if (mq_wait(mqdes, POLLOUT, abstime) < 0) {
            if (flags >= 0 && !(flags & O_NONBLOCK))
                fcntl(mqdes, F_SETFL, flags);
            return -1;
        }
    }
    if (flags >= 0 && !(flags & O_NONBLOCK))
        fcntl(mqdes, F_SETFL, flags);
    return 0;
#endif
}

/* Receive with a timeout. */
ssize_t mq_timedreceive(mqd_t mqdes, char *msg_ptr, size_t msg_len,
                        unsigned *msg_prio,
                        const struct timespec *abstime)
{
#ifdef SYS_mq_timedreceive_time64
    long ret = vlibc_syscall(SYS_mq_timedreceive_time64, mqdes, (long)msg_ptr,
                             msg_len, (long)msg_prio, (long)abstime);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (ssize_t)ret;
#elif defined(SYS_mq_timedreceive)
    long ret = vlibc_syscall(SYS_mq_timedreceive, mqdes, (long)msg_ptr,
                             msg_len, (long)msg_prio, (long)abstime);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (ssize_t)ret;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern ssize_t host_mq_timedreceive(mqd_t, char *, size_t, unsigned *,
                                        const struct timespec *)
        __asm__("mq_timedreceive");
    return host_mq_timedreceive(mqdes, msg_ptr, msg_len, msg_prio, abstime);
#else
    if (!abstime)
        return mq_receive(mqdes, msg_ptr, msg_len, msg_prio);
    int flags = fcntl(mqdes, F_GETFL, 0);
    if (flags >= 0 && !(flags & O_NONBLOCK))
        fcntl(mqdes, F_SETFL, flags | O_NONBLOCK);
    ssize_t rcv;
    for (;;) {
        rcv = mq_receive(mqdes, msg_ptr, msg_len, msg_prio);
        if (rcv >= 0)
            break;
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            rcv = -1;
            break;
        }
        if (mq_wait(mqdes, POLLIN, abstime) < 0) {
            rcv = -1;
            break;
        }
    }
    if (flags >= 0 && !(flags & O_NONBLOCK))
        fcntl(mqdes, F_SETFL, flags);
    return rcv;
#endif
}
