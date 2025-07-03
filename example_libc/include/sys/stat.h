/*
 * BSD 2-Clause License
 *
 * Purpose: Prototypes for file status queries.
 */
#ifndef SYS_STAT_H
#define SYS_STAT_H

#include <sys/types.h>

#if defined(__has_include)
#  if __has_include("/usr/include/x86_64-linux-gnu/sys/stat.h")
#    include "/usr/include/x86_64-linux-gnu/sys/stat.h"
#  elif __has_include("/usr/include/sys/stat.h")
#    include "/usr/include/sys/stat.h"
#  endif
#endif

struct stat;

int stat(const char *path, struct stat *buf);
int fstat(int fd, struct stat *buf);
int fstatat(int dirfd, const char *path, struct stat *buf, int flags);
int lstat(const char *path, struct stat *buf);
int mknod(const char *path, mode_t mode, dev_t dev);
int mkfifo(const char *path, mode_t mode);
int mkfifoat(int dirfd, const char *path, mode_t mode);
/* Create special files relative to a directory via SYS_mknodat. */
int mknodat(int dirfd, const char *path, mode_t mode, dev_t dev);
/* Reserve space in a file ensuring allocation of the specified range. */
int posix_fallocate(int fd, off_t offset, off_t len);

#endif /* SYS_STAT_H */
