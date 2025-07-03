/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms,
 * with or without modification, are permitted provided that the copyright
 * notice and this permission notice appear in all copies. This software is
 * provided "as is" without warranty.
 *
 * Purpose: Implements named semaphore wrappers for vlibc.
 */

#include "semaphore.h"
#include "errno.h"
#include "time.h"
#include <stdarg.h>
#include <stdlib.h>
#include <fcntl.h>
#include "memory.h"

#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)

extern sem_t *host_sem_open(const char *, int, ...) __asm("sem_open");
extern int host_sem_close(sem_t *) __asm("sem_close");
extern int host_sem_unlink(const char *) __asm("sem_unlink");
extern int host_sem_getvalue(sem_t *, int *) __asm("sem_getvalue");
extern int host_sem_timedwait(sem_t *, const struct timespec *) __asm("sem_timedwait");

sem_t *sem_open(const char *name, int oflag, ...)
{
    mode_t mode = 0;
    unsigned value = 0;
    if (oflag & O_CREAT) {
        va_list ap;
        va_start(ap, oflag);
        mode = va_arg(ap, mode_t);
        value = va_arg(ap, unsigned);
        va_end(ap);
    }
    return host_sem_open(name, oflag, mode, value);
}

int sem_close(sem_t *sem)
{
    return host_sem_close(sem);
}

int sem_unlink(const char *name)
{
    return host_sem_unlink(name);
}

int sem_getvalue(sem_t *sem, int *value)
{
    return host_sem_getvalue(sem, value);
}

int sem_timedwait(sem_t *sem, const struct timespec *abstime)
{
    return host_sem_timedwait(sem, abstime);
}

#else /* generic minimal fallback */

sem_t *sem_open(const char *name, int oflag, ...)
{
    (void)name; (void)oflag;
    unsigned value = 0;
    if (oflag & O_CREAT) {
        va_list ap;
        va_start(ap, oflag);
        (void)va_arg(ap, mode_t);
        value = va_arg(ap, unsigned);
        va_end(ap);
    }
    sem_t *sem = malloc(sizeof(*sem));
    if (!sem) {
        errno = ENOMEM;
        return SEM_FAILED;
    }
    sem_init(sem, 0, value);
    return sem;
}

int sem_close(sem_t *sem)
{
    if (!sem)
        return -1;
    sem_destroy(sem);
    free(sem);
    return 0;
}

int sem_unlink(const char *name)
{
    (void)name;
    return 0;
}

int sem_getvalue(sem_t *sem, int *value)
{
    if (!sem || !value)
        return EINVAL;
    *value = atomic_load_explicit(&sem->count, memory_order_relaxed);
    return 0;
}

int sem_timedwait(sem_t *sem, const struct timespec *abstime)
{
    if (!sem || !abstime)
        return EINVAL;
    for (;;) {
        if (sem_trywait(sem) == 0)
            return 0;
        struct timespec now;
        clock_gettime(CLOCK_REALTIME, &now);
        if (now.tv_sec > abstime->tv_sec ||
            (now.tv_sec == abstime->tv_sec && now.tv_nsec >= abstime->tv_nsec))
            return ETIMEDOUT;
        struct timespec ts = {0, 1000000};
        nanosleep(&ts, NULL);
    }
}

#endif
