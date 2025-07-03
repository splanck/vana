/*
 * BSD 2-Clause License
 *
 * Purpose: System V semaphore wrappers for vlibc.
 */

#include "sys/sem.h"
#include "errno.h"
#include <sys/syscall.h>
#include <unistd.h>
#include <stdarg.h>
#include "syscall.h"

/* Obtain a semaphore set identifier, creating one if needed. */
int semget(key_t key, int nsems, int semflg)
{
#ifdef SYS_semget
    long ret = vlibc_syscall(SYS_semget, key, nsems, semflg, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_semget(key_t, int, int) __asm__("semget");
    return host_semget(key, nsems, semflg);
#else
    (void)key; (void)nsems; (void)semflg;
    errno = ENOSYS;
    return -1;
#endif
}

/* Perform operations on members of a semaphore set. */
int semop(int semid, struct sembuf *sops, size_t nsops)
{
#ifdef SYS_semop
    long ret = vlibc_syscall(SYS_semop, semid, (long)sops, nsops, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_semop(int, struct sembuf *, size_t) __asm__("semop");
    return host_semop(semid, sops, nsops);
#else
    (void)semid; (void)sops; (void)nsops;
    errno = ENOSYS;
    return -1;
#endif
}

/* General control operation on a semaphore set. */
int semctl(int semid, int semnum, int cmd, ...)
{
    unsigned long arg = 0;
    va_list ap;
    va_start(ap, cmd);
    arg = va_arg(ap, unsigned long);
    va_end(ap);
#ifdef SYS_semctl
    long ret = vlibc_syscall(SYS_semctl, semid, semnum, cmd, arg, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_semctl(int, int, int, ...) __asm__("semctl");
    return host_semctl(semid, semnum, cmd, arg);
#else
    (void)semid; (void)semnum; (void)cmd; (void)arg;
    errno = ENOSYS;
    return -1;
#endif
}
