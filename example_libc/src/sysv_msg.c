/*
 * BSD 2-Clause License
 *
 * Purpose: System V message queue wrappers for vlibc.
 */

#include "sys/msg.h"
#include "errno.h"
#include <sys/syscall.h>
#include <unistd.h>
#include "syscall.h"

/* Obtain a message queue identifier, creating one if necessary. */
int msgget(key_t key, int msgflg)
{
#ifdef SYS_msgget
    long ret = vlibc_syscall(SYS_msgget, key, msgflg, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_msgget(key_t, int) __asm__("msgget");
    return host_msgget(key, msgflg);
#else
    (void)key; (void)msgflg;
    errno = ENOSYS;
    return -1;
#endif
}

/* Send a message to the specified queue. */
int msgsnd(int msqid, const void *msgp, size_t msgsz, int msgflg)
{
#ifdef SYS_msgsnd
    long ret = vlibc_syscall(SYS_msgsnd, msqid, (long)msgp, msgsz, msgflg, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_msgsnd(int, const void *, size_t, int) __asm__("msgsnd");
    return host_msgsnd(msqid, msgp, msgsz, msgflg);
#else
    (void)msqid; (void)msgp; (void)msgsz; (void)msgflg;
    errno = ENOSYS;
    return -1;
#endif
}

/* Receive a message from a queue, possibly blocking. */
ssize_t msgrcv(int msqid, void *msgp, size_t msgsz, long msgtyp, int msgflg)
{
#ifdef SYS_msgrcv
    long ret = vlibc_syscall(SYS_msgrcv, msqid, (long)msgp, msgsz, msgtyp,
                             msgflg, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (ssize_t)ret;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern ssize_t host_msgrcv(int, void *, size_t, long, int) __asm__("msgrcv");
    return host_msgrcv(msqid, msgp, msgsz, msgtyp, msgflg);
#else
    (void)msqid; (void)msgp; (void)msgsz; (void)msgtyp; (void)msgflg;
    errno = ENOSYS;
    return -1;
#endif
}

/* Perform control operations on a message queue. */
int msgctl(int msqid, int cmd, struct msqid_ds *buf)
{
#ifdef SYS_msgctl
    long ret = vlibc_syscall(SYS_msgctl, msqid, cmd, (long)buf, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_msgctl(int, int, struct msqid_ds *) __asm__("msgctl");
    return host_msgctl(msqid, cmd, buf);
#else
    (void)msqid; (void)cmd; (void)buf;
    errno = ENOSYS;
    return -1;
#endif
}
