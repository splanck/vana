/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that the copyright notice and this
 * permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the pthread spin lock functions for vlibc.
 */

#include "pthread.h"
#include <errno.h>
#include <stdatomic.h>

/* Initialize a spin lock object. Only process-private locks are supported. */
int pthread_spin_init(pthread_spinlock_t *lock, int pshared)
{
    (void)pshared;
    if (!lock)
        return EINVAL;
    atomic_flag_clear(&lock->locked);
    return 0;
}

/* Acquire the spin lock, busy-waiting until available. */
int pthread_spin_lock(pthread_spinlock_t *lock)
{
    if (!lock)
        return EINVAL;
    while (atomic_flag_test_and_set_explicit(&lock->locked, memory_order_acquire))
        ;
    return 0;
}

/* Try to acquire the spin lock without blocking. */
int pthread_spin_trylock(pthread_spinlock_t *lock)
{
    if (!lock)
        return EINVAL;
    if (atomic_flag_test_and_set_explicit(&lock->locked, memory_order_acquire))
        return EBUSY;
    return 0;
}

/* Release the spin lock. */
int pthread_spin_unlock(pthread_spinlock_t *lock)
{
    if (!lock)
        return EINVAL;
    atomic_flag_clear_explicit(&lock->locked, memory_order_release);
    return 0;
}

/* Destroy a spin lock object (no-op). */
int pthread_spin_destroy(pthread_spinlock_t *lock)
{
    (void)lock;
    return 0;
}
