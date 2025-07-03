/*
 * BSD 2-Clause License
 *
 * Purpose: Declarations for unbuffered I/O primitives.
 */
#ifndef IO_H
#define IO_H

#include <sys/types.h>
#include "sys/uio.h"

/* Open a file using vlibc_syscall with SYS_open or SYS_openat. */
int open(const char *path, int flags, ...);
/* openat wrapper that falls back to the host implementation on BSD. */
int openat(int dirfd, const char *path, int flags, ...);
/* Read via SYS_read using vlibc_syscall. */
ssize_t read(int fd, void *buf, size_t count);
/* Write via SYS_write using vlibc_syscall. */
ssize_t write(int fd, const void *buf, size_t count);
/* Positional read with SYS_pread/SYS_pread64 or host pread. */
ssize_t pread(int fd, void *buf, size_t count, off_t offset);
/* Positional write with SYS_pwrite/SYS_pwrite64 or host pwrite. */
ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset);
/* Vector positional read using SYS_preadv/preadv2 or a loop fallback. */
ssize_t preadv(int fd, const struct iovec *iov, int iovcnt, off_t offset);
/* Vector positional write using SYS_pwritev/pwritev2 or a loop fallback. */
ssize_t pwritev(int fd, const struct iovec *iov, int iovcnt, off_t offset);
/* Close a descriptor using SYS_close. */
int close(int fd);
off_t lseek(int fd, off_t offset, int whence);
int dup(int oldfd);
int dup2(int oldfd, int newfd);
int dup3(int oldfd, int newfd, int flags);
int pipe(int pipefd[2]);
int pipe2(int pipefd[2], int flags);
int ftruncate(int fd, off_t length);
int truncate(const char *path, off_t length);
int unlink(const char *pathname);
/* Remove a pathname using SYS_unlinkat or host unlinkat on BSD. */
int unlinkat(int dirfd, const char *pathname, int flags);
int rename(const char *oldpath, const char *newpath);
/* Rename relative to directory FDs using SYS_renameat with BSD fallback. */
int renameat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath);
int link(const char *oldpath, const char *newpath);
/* Create a hard link relative to directory FDs via SYS_linkat. */
int linkat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flags);
int symlink(const char *target, const char *linkpath);
/* Create a symlink relative to dirfd via SYS_symlinkat with host fallback. */
int symlinkat(const char *target, int dirfd, const char *linkpath);
ssize_t readlink(const char *pathname, char *buf, size_t bufsiz);
int mkdir(const char *pathname, mode_t mode);
/* Create a directory relative to dirfd using SYS_mkdirat or host mkdirat. */
int mkdirat(int dirfd, const char *pathname, mode_t mode);
int rmdir(const char *pathname);
int chdir(const char *path);
/* Check permissions for pathname; returns 0 or -1 and sets errno. */
int access(const char *pathname, int mode);
/* Dirfd-relative permission check returning 0 or -1 with errno set. */
int faccessat(int dirfd, const char *pathname, int mode, int flags);
/* Flush all pending writes for a descriptor using SYS_fsync or host fsync. */
int fsync(int fd);
/* Synchronize file data only via SYS_fdatasync or host fdatasync. */
int fdatasync(int fd);

#endif /* IO_H */
