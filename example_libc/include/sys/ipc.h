/*
 * BSD 2-Clause License
 *
 * Purpose: System V IPC key and permission definitions.
 */
#ifndef SYS_IPC_H
#define SYS_IPC_H

#include <sys/types.h>

#if defined(__has_include)
#  if __has_include("/usr/include/x86_64-linux-gnu/sys/ipc.h")
#    include "/usr/include/x86_64-linux-gnu/sys/ipc.h"
#    define VLIBC_SYS_IPC_NATIVE 1
#  elif __has_include("/usr/include/sys/ipc.h")
#    include "/usr/include/sys/ipc.h"
#    define VLIBC_SYS_IPC_NATIVE 1
#  endif
#endif

#ifndef VLIBC_SYS_IPC_NATIVE

typedef int key_t;

struct ipc_perm {
    key_t   __key;
    uid_t   uid;
    gid_t   gid;
    uid_t   cuid;
    gid_t   cgid;
    mode_t  mode;
    unsigned short __seq;
};

#define IPC_PRIVATE ((key_t)0)
#define IPC_CREAT   01000
#define IPC_EXCL    02000
#define IPC_NOWAIT  04000

#define IPC_RMID 0
#define IPC_SET  1
#define IPC_STAT 2

key_t ftok(const char *pathname, int proj_id);

#endif

#endif /* SYS_IPC_H */
