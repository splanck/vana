/*
 * BSD 2-Clause License
 *
 * Purpose: Declarations for counting semaphore primitives.
 */
#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <stdatomic.h>
#include "time.h"

typedef struct {
    atomic_int count;
} sem_t;

#ifndef SEM_FAILED
#define SEM_FAILED ((sem_t *)-1)
#endif

int sem_init(sem_t *sem, int pshared, unsigned value);
/* Initialize a counting semaphore with the given starting value. */
int sem_destroy(sem_t *sem);
/* Destroy the semaphore object (no-op for this implementation). */
int sem_wait(sem_t *sem);
/* Decrement the semaphore count, blocking until it becomes positive. */
int sem_trywait(sem_t *sem);
/* Try to decrement the semaphore without blocking. */
int sem_post(sem_t *sem);
/* Increment the semaphore count and wake any waiting threads. */

sem_t *sem_open(const char *name, int oflag, ...);
/* Open or create a named semaphore object. */
int sem_close(sem_t *sem);
/* Close a named semaphore object. */
int sem_unlink(const char *name);
/* Remove a named semaphore. */
int sem_getvalue(sem_t *sem, int *value);
/* Retrieve the current semaphore count. */
int sem_timedwait(sem_t *sem, const struct timespec *abstime);
/* Wait with an absolute timeout. */

#endif /* SEMAPHORE_H */
