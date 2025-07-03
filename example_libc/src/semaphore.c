/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the semaphore functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "semaphore.h"
#include <errno.h>

/*
 * Initialize a semaphore with the provided initial count.
 * The pshared argument is ignored as all semaphores are
 * process-local in this implementation.
 */
int sem_init(sem_t *sem, int pshared, unsigned value)
{
    (void)pshared;
    if (!sem)
        return EINVAL;
    atomic_store(&sem->count, (int)value);
    return 0;
}

/*
 * Destroy a semaphore object.  Nothing needs to be done for
 * this simple atomic counter based implementation so it is a
 * no-op.
 */
int sem_destroy(sem_t *sem)
{
    (void)sem;
    return 0;
}

/*
 * Decrement the semaphore count, waiting in a loop if the
 * count is zero.  The wait is implemented by repeatedly
 * polling with a short sleep.
 */
int sem_wait(sem_t *sem)
{
    if (!sem)
        return EINVAL;
    for (;;) {
        int val = atomic_load_explicit(&sem->count, memory_order_acquire);
        if (val > 0 &&
            atomic_compare_exchange_weak_explicit(&sem->count, &val, val - 1,
                                                  memory_order_acquire,
                                                  memory_order_relaxed))
            return 0;
        struct timespec ts = {0, 1000000};
        nanosleep(&ts, NULL);
    }
}

/*
 * Try to decrement the semaphore without blocking.  If the
 * count is already zero the call fails with EAGAIN.
 */
int sem_trywait(sem_t *sem)
{
    if (!sem)
        return EINVAL;
    int val = atomic_load_explicit(&sem->count, memory_order_acquire);
    while (val > 0) {
        if (atomic_compare_exchange_weak_explicit(&sem->count, &val, val - 1,
                                                  memory_order_acquire,
                                                  memory_order_relaxed))
            return 0;
    }
    return EAGAIN;
}

/*
 * Increment the semaphore count and wake any waiting thread.
 */
int sem_post(sem_t *sem)
{
    if (!sem)
        return EINVAL;
    atomic_fetch_add_explicit(&sem->count, 1, memory_order_release);
    return 0;
}
