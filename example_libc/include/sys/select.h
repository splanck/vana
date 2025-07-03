/*
 * BSD 2-Clause License
 *
 * Purpose: fd_set helpers and select() declaration.
 */
#ifndef SYS_SELECT_H
#define SYS_SELECT_H

#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include "../time.h"

#ifndef FD_SETSIZE
#define FD_SETSIZE 1024
#endif

typedef struct {
    unsigned long fds_bits[FD_SETSIZE / (8 * sizeof(unsigned long))];
} fd_set;

#define __FD_WORDS (FD_SETSIZE / (8 * sizeof(unsigned long)))

#define FD_ZERO(set) memset((set), 0, sizeof(fd_set))
#define FD_SET(fd, set) ((set)->fds_bits[(fd) / (8 * sizeof(unsigned long))] |= (1UL << ((fd) % (8 * sizeof(unsigned long)))))
#define FD_CLR(fd, set) ((set)->fds_bits[(fd) / (8 * sizeof(unsigned long))] &= ~(1UL << ((fd) % (8 * sizeof(unsigned long)))))
#define FD_ISSET(fd, set) (((set)->fds_bits[(fd) / (8 * sizeof(unsigned long))] >> ((fd) % (8 * sizeof(unsigned long)))) & 1UL)

/*
 * Wait for activity on file descriptors. On BSD targets this function
 * simply calls the C library's select(2) as the raw syscall interface may
 * differ.
 */
int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
           struct timeval *timeout);

#endif /* SYS_SELECT_H */
