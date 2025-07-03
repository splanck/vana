/*
 * BSD 2-Clause License
 *
 * Purpose: Scatter/gather I/O structures and calls.
 */
#ifndef SYS_UIO_H
#define SYS_UIO_H


#include <sys/types.h>
#include <stddef.h>

#if defined(__has_include)
#  if __has_include("/usr/include/x86_64-linux-gnu/sys/uio.h")
#    include "/usr/include/x86_64-linux-gnu/sys/uio.h"
#    define VLIBC_SYS_UIO_NATIVE 1
#  elif __has_include("/usr/include/sys/uio.h")
#    include "/usr/include/sys/uio.h"
#    define VLIBC_SYS_UIO_NATIVE 1
#  endif
#endif

#ifndef VLIBC_SYS_UIO_NATIVE
struct iovec {
    void *iov_base;
    size_t iov_len;
};
#endif

/* Vector read wrapper calling SYS_readv via vlibc_syscall and
 * falling back to the host implementation or an emulation on BSD. */
ssize_t readv(int fd, const struct iovec *iov, int iovcnt);
/* Vector write wrapper calling SYS_writev via vlibc_syscall and
 * falling back to the host implementation or an emulation on BSD. */
ssize_t writev(int fd, const struct iovec *iov, int iovcnt);

#endif /* SYS_UIO_H */
