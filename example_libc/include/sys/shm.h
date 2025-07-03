/*
 * BSD 2-Clause License
 *
 * Purpose: System V shared memory and POSIX shm_open helpers.
 */
#ifndef SYS_SHM_H
#define SYS_SHM_H

#include <sys/types.h>
#include "sys/ipc.h"

#if defined(__has_include)
#  if __has_include("/usr/include/x86_64-linux-gnu/sys/shm.h")
#    include "/usr/include/x86_64-linux-gnu/sys/shm.h"
#    define VLIBC_SYS_SHM_NATIVE 1
#  elif __has_include("/usr/include/sys/shm.h")
#    include "/usr/include/sys/shm.h"
#    define VLIBC_SYS_SHM_NATIVE 1
#  endif
#endif

#ifndef VLIBC_SYS_SHM_NATIVE

typedef unsigned long shmatt_t;

struct shmid_ds {
    struct ipc_perm shm_perm;
    size_t          shm_segsz;
    pid_t           shm_lpid;
    pid_t           shm_cpid;
    shmatt_t        shm_nattch;
    time_t          shm_atime;
    time_t          shm_dtime;
    time_t          shm_ctime;
};

#ifndef SHM_RDONLY
#define SHM_RDONLY  010000
#endif
#ifndef SHM_RND
#define SHM_RND     020000
#endif
#ifndef SHM_REMAP
#define SHM_REMAP   040000
#endif
#ifndef SHM_EXEC
#define SHM_EXEC    0100000
#endif

#ifndef SHM_LOCK
#define SHM_LOCK    11
#endif
#ifndef SHM_UNLOCK
#define SHM_UNLOCK  12
#endif

#ifndef SHM_FAILED
#define SHM_FAILED ((void *)-1)
#endif

int shmget(key_t key, size_t size, int shmflg);
/* Obtain a shared memory segment identifier. */
void *shmat(int shmid, const void *shmaddr, int shmflg);
/* Attach a shared memory segment to the process. */
int shmdt(const void *shmaddr);
/* Detach a shared memory segment from the process. */
int shmctl(int shmid, int cmd, struct shmid_ds *buf);
/* Control shared memory segment parameters. */

int shm_open(const char *name, int oflag, mode_t mode);
/* Open or create a POSIX shared memory object. */
int shm_unlink(const char *name);
/* Remove a POSIX shared memory object. */

#endif

#endif /* SYS_SHM_H */
