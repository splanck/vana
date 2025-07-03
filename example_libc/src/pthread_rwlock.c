/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the pthread_rwlock functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "pthread.h"
#include <stdatomic.h>
#include <errno.h>
#include "time.h"

/* Initialize a read-write lock object. */
int pthread_rwlock_init(pthread_rwlock_t *rwlock, void *attr)
{
    (void)attr;
    atomic_store(&rwlock->readers, 0);
    atomic_store(&rwlock->writer, 0);
    return 0;
}

/* Acquire the lock for reading; multiple readers may hold it. */
int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock)
{
    for (;;) {
        while (atomic_load_explicit(&rwlock->writer, memory_order_acquire))
            ;
        atomic_fetch_add_explicit(&rwlock->readers, 1, memory_order_acquire);
        if (!atomic_load_explicit(&rwlock->writer, memory_order_acquire))
            break;
        atomic_fetch_sub_explicit(&rwlock->readers, 1, memory_order_release);
    }
    return 0;
}

/* Acquire the lock for writing, waiting until no readers exist. */
int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock)
{
    while (atomic_exchange_explicit(&rwlock->writer, 1, memory_order_acquire))
        ;
    while (atomic_load_explicit(&rwlock->readers, memory_order_acquire) != 0) {
        struct timespec ts = {0, 1000000};
        nanosleep(&ts, NULL);
    }
    return 0;
}

/* Release a read or write hold on the lock. */
int pthread_rwlock_unlock(pthread_rwlock_t *rwlock)
{
    if (atomic_load_explicit(&rwlock->writer, memory_order_acquire))
        atomic_store_explicit(&rwlock->writer, 0, memory_order_release);
    else
        atomic_fetch_sub_explicit(&rwlock->readers, 1, memory_order_release);
    return 0;
}

/* Destroy a read-write lock object (no-op). */
int pthread_rwlock_destroy(pthread_rwlock_t *rwlock)
{
    (void)rwlock;
    return 0;
}
