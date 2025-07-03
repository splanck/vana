/*
 * BSD 2-Clause License
 *
 * Purpose: Declarations for file control operations.
 */
#ifndef FCNTL_H
#define FCNTL_H

#include <sys/types.h>

int fcntl(int fd, int cmd, ...);

/* open(2) flags */
#ifndef O_RDONLY
#define O_RDONLY        00000000
#endif
#ifndef O_WRONLY
#define O_WRONLY        00000001
#endif
#ifndef O_RDWR
#define O_RDWR          00000002
#endif
#ifndef O_CREAT
#define O_CREAT         00000100
#endif
#ifndef O_EXCL
#define O_EXCL          00000200
#endif
#ifndef O_NOCTTY
#define O_NOCTTY        00000400
#endif
#ifndef O_TRUNC
#define O_TRUNC         00001000
#endif
#ifndef O_APPEND
#define O_APPEND        00002000
#endif
#ifndef O_NONBLOCK
#define O_NONBLOCK      00004000
#endif
#ifndef O_CLOEXEC
#define O_CLOEXEC       02000000
#endif

/* fcntl(2) commands */
#ifndef F_DUPFD
#define F_DUPFD         0
#endif
#ifndef F_GETFD
#define F_GETFD         1
#endif
#ifndef F_SETFD
#define F_SETFD         2
#endif
#ifndef F_GETFL
#define F_GETFL         3
#endif
#ifndef F_SETFL
#define F_SETFL         4
#endif
#ifndef F_DUPFD_CLOEXEC
#define F_DUPFD_CLOEXEC 1030
#endif

/* descriptor flags for F_GETFD/F_SETFD */
#ifndef FD_CLOEXEC
#define FD_CLOEXEC      1
#endif

/* advisory hints for posix_fadvise */
#ifndef POSIX_FADV_NORMAL
#define POSIX_FADV_NORMAL     0
#endif
#ifndef POSIX_FADV_RANDOM
#define POSIX_FADV_RANDOM     1
#endif
#ifndef POSIX_FADV_SEQUENTIAL
#define POSIX_FADV_SEQUENTIAL 2
#endif
#ifndef POSIX_FADV_WILLNEED
#define POSIX_FADV_WILLNEED   3
#endif
#ifndef POSIX_FADV_DONTNEED
#define POSIX_FADV_DONTNEED   4
#endif
#ifndef POSIX_FADV_NOREUSE
#define POSIX_FADV_NOREUSE    5
#endif

int posix_fadvise(int fd, off_t offset, off_t len, int advice);

#endif /* FCNTL_H */
