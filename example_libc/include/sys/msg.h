/*
 * BSD 2-Clause License
 *
 * Purpose: System V message queue interfaces.
 */
#ifndef SYS_MSG_H
#define SYS_MSG_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <stddef.h>

#if defined(__has_include)
#  if __has_include("/usr/include/x86_64-linux-gnu/sys/msg.h")
#    include "/usr/include/x86_64-linux-gnu/sys/msg.h"
#    define VLIBC_SYS_MSG_NATIVE 1
#  elif __has_include("/usr/include/sys/msg.h")
#    include "/usr/include/sys/msg.h"
#    define VLIBC_SYS_MSG_NATIVE 1
#  endif
#endif

#ifndef VLIBC_SYS_MSG_NATIVE

struct msqid_ds {
    struct ipc_perm msg_perm;
    size_t          msg_cbytes;
    size_t          msg_qnum;
    size_t          msg_qbytes;
    pid_t           msg_lspid;
    pid_t           msg_lrpid;
    time_t          msg_stime;
    time_t          msg_rtime;
    time_t          msg_ctime;
};

struct msgbuf {
    long mtype;
    char mtext[1];
};

#ifndef MSG_NOERROR
#define MSG_NOERROR 010000
#endif

int msgget(key_t key, int msgflg);
int msgsnd(int msqid, const void *msgp, size_t msgsz, int msgflg);
ssize_t msgrcv(int msqid, void *msgp, size_t msgsz,
               long msgtyp, int msgflg);
int msgctl(int msqid, int cmd, struct msqid_ds *buf);

#endif

#endif /* SYS_MSG_H */
