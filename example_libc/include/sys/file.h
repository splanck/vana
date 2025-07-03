/*
 * BSD 2-Clause License
 *
 * Purpose: File locking constants and sendfile declarations.
 */
#ifndef SYS_FILE_H
#define SYS_FILE_H

#include <sys/types.h>
#include "../time.h"
#include "sys/uio.h"

#if defined(__has_include)
#  if __has_include("/usr/include/x86_64-linux-gnu/sys/file.h")
#    include "/usr/include/x86_64-linux-gnu/sys/file.h"
#  elif __has_include("/usr/include/sys/file.h")
#    include "/usr/include/sys/file.h"
#  endif
#endif

#ifndef LOCK_SH
#define LOCK_SH 1
#endif
#ifndef LOCK_EX
#define LOCK_EX 2
#endif
#ifndef LOCK_UN
#define LOCK_UN 8
#endif
#ifndef LOCK_NB
#define LOCK_NB 4
#endif

int chmod(const char *path, mode_t mode);
int chown(const char *path, uid_t owner, gid_t group);
mode_t umask(mode_t mask);
int fchmod(int fd, mode_t mode);
int fchmodat(int dirfd, const char *pathname, mode_t mode, int flags);
int fchown(int fd, uid_t owner, gid_t group);
int fchownat(int dirfd, const char *pathname, uid_t owner, gid_t group, int flags);
int lchown(const char *pathname, uid_t owner, gid_t group);
int flock(int fd, int operation);

struct utimbuf {
    time_t actime;
    time_t modtime;
};

struct timeval;
int utime(const char *path, const struct utimbuf *times);
int utimes(const char *path, const struct timeval times[2]);

struct sf_hdtr {
    struct iovec *headers;
    int hdr_cnt;
    struct iovec *trailers;
    int trl_cnt;
};
int sendfile(int fd, int s, off_t offset, size_t nbytes,
             struct sf_hdtr *hdtr, off_t *sbytes, int flags);

#endif /* SYS_FILE_H */
