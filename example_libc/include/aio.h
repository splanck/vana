/*
 * BSD 2-Clause License
 *
 * Purpose: Asynchronous I/O control structures and helpers.
 */
#ifndef AIO_H
#define AIO_H

#include <sys/types.h>
#include "signal.h"
#include "time.h"

#ifndef VLIBC_AIO_NATIVE

struct aiocb {
    int aio_fildes;           /* File descriptor */
    int aio_lio_opcode;       /* Operation to perform */
    int aio_reqprio;          /* Priority offset (unused) */
    volatile void *aio_buf;   /* Data buffer */
    size_t aio_nbytes;        /* Length of transfer */
    struct sigevent aio_sigevent; /* Completion signal */
    off_t aio_offset;         /* File offset */
    long __reserved[2];       /* internal use */
};

enum {
    AIO_CANCELED,
    AIO_NOTCANCELED,
    AIO_ALLDONE
};

enum {
    LIO_READ,
    LIO_WRITE,
    LIO_NOP
};

enum {
    LIO_WAIT,
    LIO_NOWAIT
};

int aio_read(struct aiocb *aiocbp);
int aio_write(struct aiocb *aiocbp);
int aio_error(const struct aiocb *aiocbp);
ssize_t aio_return(struct aiocb *aiocbp);
int aio_suspend(const struct aiocb *const list[], int nent,
                const struct timespec *timeout);
int aio_cancel(int fildes, struct aiocb *aiocbp);
int lio_listio(int mode, struct aiocb *const list[], int nent,
               struct sigevent *sig);

#endif /* !VLIBC_AIO_NATIVE */

#endif /* AIO_H */
