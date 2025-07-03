/*
 * BSD 2-Clause License
 *
 * Purpose: System V semaphore interfaces.
 */
#ifndef SYS_SEM_H
#define SYS_SEM_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <stddef.h>

#if defined(__has_include)
#  if __has_include("/usr/include/x86_64-linux-gnu/sys/sem.h")
#    include "/usr/include/x86_64-linux-gnu/sys/sem.h"
#    define VLIBC_SYS_SEM_NATIVE 1
#  elif __has_include("/usr/include/sys/sem.h")
#    include "/usr/include/sys/sem.h"
#    define VLIBC_SYS_SEM_NATIVE 1
#  endif
#endif

#ifndef VLIBC_SYS_SEM_NATIVE

struct sembuf {
    unsigned short sem_num;
    short          sem_op;
    short          sem_flg;
};

struct semid_ds {
    struct ipc_perm sem_perm;
    unsigned short  sem_nsems;
    time_t          sem_otime;
    time_t          sem_ctime;
};

#ifndef SEM_UNDO
#define SEM_UNDO 0x1000
#endif

#ifndef _SEM_SEMUN_UNDEFINED
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};
#endif

int semget(key_t key, int nsems, int semflg);
int semop(int semid, struct sembuf *sops, size_t nsops);
int semctl(int semid, int semnum, int cmd, ...);

#endif

#endif /* SYS_SEM_H */
