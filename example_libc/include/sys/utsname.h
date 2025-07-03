/*
 * BSD 2-Clause License
 *
 * Purpose: System identification structure and uname.
 */
#ifndef SYS_UTSNAME_H
#define SYS_UTSNAME_H

#include <sys/types.h>

#if defined(__has_include)
#  if __has_include("/usr/include/x86_64-linux-gnu/sys/utsname.h")
#    include "/usr/include/x86_64-linux-gnu/sys/utsname.h"
#    define VLIBC_SYS_UTSNAME_NATIVE 1
#  elif __has_include("/usr/include/sys/utsname.h")
#    include "/usr/include/sys/utsname.h"
#    define VLIBC_SYS_UTSNAME_NATIVE 1
#  endif
#endif

#ifndef VLIBC_SYS_UTSNAME_NATIVE
#ifndef _UTSNAME_LENGTH
#define _UTSNAME_LENGTH 65
#endif

struct utsname {
    char sysname[_UTSNAME_LENGTH];
    char nodename[_UTSNAME_LENGTH];
    char release[_UTSNAME_LENGTH];
    char version[_UTSNAME_LENGTH];
    char machine[_UTSNAME_LENGTH];
};
#endif

int uname(struct utsname *buf);

#endif /* SYS_UTSNAME_H */
