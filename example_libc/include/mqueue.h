/*
 * BSD 2-Clause License
 *
 * Purpose: Declarations for POSIX message queue wrappers.
 */
#ifndef MQUEUE_H
#define MQUEUE_H

#include <sys/types.h>
#include <fcntl.h>
#include "time.h"

#ifndef VLIBC_MQUEUE_NATIVE
typedef int mqd_t;
struct mq_attr {
    long mq_flags;
    long mq_maxmsg;
    long mq_msgsize;
    long mq_curmsgs;
    long __pad[4];
};

mqd_t mq_open(const char *name, int oflag, ...);
/* Open or create a POSIX message queue. */
int mq_close(mqd_t mqdes);
/* Close a message queue descriptor. */
int mq_unlink(const char *name);
/* Remove a named message queue. */
int mq_send(mqd_t mqdes, const char *msg_ptr, size_t msg_len, unsigned msg_prio);
/* Send a message to the queue. */
ssize_t mq_receive(mqd_t mqdes, char *msg_ptr, size_t msg_len,
                   unsigned *msg_prio);
/* Receive the oldest message from the queue. */
int mq_getattr(mqd_t mqdes, struct mq_attr *attr);
/* Query queue attributes such as flags and sizes. */
int mq_setattr(mqd_t mqdes, const struct mq_attr *attr, struct mq_attr *oattr);
/* Change queue flags and optionally obtain the previous settings. */
int mq_timedsend(mqd_t mqdes, const char *msg_ptr, size_t msg_len,
                 unsigned msg_prio, const struct timespec *timeout);
/* Send a message with an absolute timeout. */
ssize_t mq_timedreceive(mqd_t mqdes, char *msg_ptr, size_t msg_len,
                        unsigned *msg_prio,
                        const struct timespec *timeout);
/* Receive a message with an absolute timeout. */
#endif

#endif /* MQUEUE_H */
