/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that the copyright notice and
 * this permission notice appear in all copies. This software is provided "as is"
 * without warranty.
 *
 * Purpose: System V shared memory wrappers for vlibc.
 */

#include "sys/shm.h"
#include "errno.h"
#include <sys/syscall.h>
#include <unistd.h>
#include "syscall.h"

/*
 * shmget() - obtain a shared memory segment. Uses the vlibc_syscall wrapper
 * when available and falls back to the host implementation or returns ENOSYS
 * when unsupported.
 */
int shmget(key_t key, size_t size, int shmflg)
{
#ifdef SYS_shmget
    long ret = vlibc_syscall(SYS_shmget, key, size, shmflg, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_shmget(key_t, size_t, int) __asm__("shmget");
    return host_shmget(key, size, shmflg);
#else
    (void)key; (void)size; (void)shmflg;
    errno = ENOSYS;
    return -1;
#endif
}
/*
 * shmat() - attach a shared memory segment to the process. It performs the
 * direct syscall when possible and otherwise calls into the host or returns
 * ENOSYS when no implementation is available.
 */

void *shmat(int shmid, const void *shmaddr, int shmflg)
{
#ifdef SYS_shmat
    long ret = vlibc_syscall(SYS_shmat, shmid, (long)shmaddr, shmflg, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return (void *)-1;
    }
    return (void *)ret;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern void *host_shmat(int, const void *, int) __asm__("shmat");
    return host_shmat(shmid, shmaddr, shmflg);
#else
    (void)shmid; (void)shmaddr; (void)shmflg;
    errno = ENOSYS;
    return (void *)-1;
#endif
}

/*
 * shmdt() - detach a shared memory segment. Utilises the syscall when
 * available and falls back to the host or reports ENOSYS.
 */
int shmdt(const void *shmaddr)
{
#ifdef SYS_shmdt
    long ret = vlibc_syscall(SYS_shmdt, (long)shmaddr, 0, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_shmdt(const void *) __asm__("shmdt");
    return host_shmdt(shmaddr);
#else
    (void)shmaddr;
    errno = ENOSYS;
    return -1;
#endif
}

/*
 * shmctl() - control operations on a shared memory segment. Calls the
 * kernel syscall when supported and otherwise falls back to the host or
 * signals ENOSYS.
 */
int shmctl(int shmid, int cmd, struct shmid_ds *buf)
{
#ifdef SYS_shmctl
    long ret = vlibc_syscall(SYS_shmctl, shmid, cmd, (long)buf, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_shmctl(int, int, struct shmid_ds *) __asm__("shmctl");
    return host_shmctl(shmid, cmd, buf);
#else
    (void)shmid; (void)cmd; (void)buf;
    errno = ENOSYS;
    return -1;
#endif
}
