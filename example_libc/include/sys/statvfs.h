/*
 * BSD 2-Clause License
 *
 * Purpose: Filesystem statistics interface.
 */
#ifndef SYS_STATVFS_H
#define SYS_STATVFS_H

#include <sys/types.h>

#if defined(__has_include)
#  if __has_include("/usr/include/x86_64-linux-gnu/sys/statvfs.h")
#    include "/usr/include/x86_64-linux-gnu/sys/statvfs.h"
#  elif __has_include("/usr/include/sys/statvfs.h")
#    include "/usr/include/sys/statvfs.h"
#  endif
#endif

struct statvfs;

int statvfs(const char *path, struct statvfs *buf);
int fstatvfs(int fd, struct statvfs *buf);

#endif /* SYS_STATVFS_H */
