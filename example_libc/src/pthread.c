/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the pthread functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "pthread.h"
#include <errno.h>
#include <stdatomic.h>
#include "time.h"

/*
 * Reference the host's pthread symbols directly by their GLIBC versioned
 * names so that the wrappers call into the real implementation rather than
 * recursively invoking themselves.
 */
extern pthread_t host_pthread_self(void);
extern int host_pthread_equal(pthread_t, pthread_t);
extern void host_pthread_exit(void *);
extern int host_pthread_cancel(pthread_t);
extern int host_pthread_create(pthread_t *, const void *,
                               void *(*)(void *), void *);
extern int host_pthread_join(pthread_t, void **);
extern int host_pthread_detach(pthread_t);
extern void host_pthread_testcancel(void);

/* Map the host_* symbols to the versioned glibc pthread symbols. */
__asm__(".symver host_pthread_self,pthread_self@GLIBC_2.2.5");
__asm__(".symver host_pthread_equal,pthread_equal@GLIBC_2.2.5");
__asm__(".symver host_pthread_exit,pthread_exit@GLIBC_2.2.5");
__asm__(".symver host_pthread_cancel,pthread_cancel@GLIBC_2.2.5");
__asm__(".symver host_pthread_create,pthread_create@GLIBC_2.2.5");
__asm__(".symver host_pthread_join,pthread_join@GLIBC_2.2.5");
__asm__(".symver host_pthread_detach,pthread_detach@GLIBC_2.2.5");
__asm__(".symver host_pthread_testcancel,pthread_testcancel@GLIBC_2.2.5");

/* Initialize a mutex implemented as a simple spinlock. */
int pthread_mutex_init(pthread_mutex_t *mutex, void *attr)
{
    int type = PTHREAD_MUTEX_NORMAL;
    if (attr)
        type = ((pthread_mutexattr_t *)attr)->type;

    atomic_flag_clear(&mutex->locked);
    mutex->type = type;
    mutex->owner = 0;
    mutex->recursion = 0;
    return 0;
}

/* Acquire the spinlock based mutex. */
int pthread_mutex_lock(pthread_mutex_t *mutex)
{
    pthread_t self = pthread_self();
    if (mutex->type == PTHREAD_MUTEX_RECURSIVE && mutex->owner == self) {
        mutex->recursion++;
        return 0;
    }

    while (atomic_flag_test_and_set_explicit(&mutex->locked,
                                             memory_order_acquire))
        ;

    mutex->owner = self;
    mutex->recursion = 1;
    return 0;
}

/* Try to acquire the mutex without blocking. */
int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
    pthread_t self = pthread_self();
    if (mutex->type == PTHREAD_MUTEX_RECURSIVE && mutex->owner == self) {
        mutex->recursion++;
        return 0;
    }

    if (atomic_flag_test_and_set_explicit(&mutex->locked,
                                          memory_order_acquire))
        return EBUSY;

    mutex->owner = self;
    mutex->recursion = 1;
    return 0;
}

/* Release the mutex. */
int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    if (mutex->type == PTHREAD_MUTEX_RECURSIVE) {
        if (mutex->owner != pthread_self())
            return EPERM;
        if (--mutex->recursion > 0)
            return 0;
        mutex->owner = 0;
    } else {
        mutex->owner = 0;
        mutex->recursion = 0;
    }

    atomic_flag_clear_explicit(&mutex->locked, memory_order_release);
    return 0;
}

/* Initialize a mutex attribute object. */
int pthread_mutexattr_init(pthread_mutexattr_t *attr)
{
    if (!attr)
        return EINVAL;
    attr->type = PTHREAD_MUTEX_NORMAL;
    return 0;
}

/* Destroy a mutex attribute object (no-op). */
int pthread_mutexattr_destroy(pthread_mutexattr_t *attr)
{
    (void)attr;
    return 0;
}

/* Set the mutex type to normal or recursive. */
int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type)
{
    if (!attr)
        return EINVAL;
    if (type != PTHREAD_MUTEX_NORMAL && type != PTHREAD_MUTEX_RECURSIVE)
        return EINVAL;
    attr->type = type;
    return 0;
}

/* Retrieve the configured mutex type. */
int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *type)
{
    if (!attr || !type)
        return EINVAL;
    *type = attr->type;
    return 0;
}

/* Destroy a mutex (no resources to release). */
int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
    (void)mutex;
    return 0;
}

/* Initialize a condition variable. */
int pthread_cond_init(pthread_cond_t *cond, void *attr)
{
    (void)attr;
    atomic_store(&cond->seq, 0);
    atomic_store(&cond->next, 0);
    return 0;
}

/* Wait for a condition signal while temporarily releasing the mutex. */
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
    int ticket = atomic_fetch_add_explicit(&cond->next, 1,
                                           memory_order_acquire);
    pthread_mutex_unlock(mutex);
    while (atomic_load_explicit(&cond->seq, memory_order_acquire) <= ticket) {
        struct timespec ts = {0, 1000000};
        nanosleep(&ts, NULL);
    }
    pthread_mutex_lock(mutex);
    return 0;
}

/* Like pthread_cond_wait but stop waiting after "abstime". */
int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
                           const struct timespec *abstime)
{
    if (!abstime)
        return pthread_cond_wait(cond, mutex);

    int ticket = atomic_fetch_add_explicit(&cond->next, 1,
                                           memory_order_acquire);
    pthread_mutex_unlock(mutex);

    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    while (atomic_load_explicit(&cond->seq, memory_order_acquire) <= ticket) {
        if (now.tv_sec > abstime->tv_sec ||
            (now.tv_sec == abstime->tv_sec &&
             now.tv_nsec >= abstime->tv_nsec)) {
            pthread_mutex_lock(mutex);
            return ETIMEDOUT;
        }
        struct timespec ts = {0, 1000000};
        nanosleep(&ts, NULL);
        clock_gettime(CLOCK_REALTIME, &now);
    }

    pthread_mutex_lock(mutex);
    return 0;
}

/* Wake one thread waiting on the condition. */
int pthread_cond_signal(pthread_cond_t *cond)
{
    int next = atomic_load_explicit(&cond->next, memory_order_acquire);
    int seq = atomic_load_explicit(&cond->seq, memory_order_acquire);
    if (seq < next)
        atomic_fetch_add_explicit(&cond->seq, 1, memory_order_release);
    return 0;
}

/* Wake all threads waiting on the condition. */
int pthread_cond_broadcast(pthread_cond_t *cond)
{
    int next = atomic_load_explicit(&cond->next, memory_order_acquire);
    atomic_store_explicit(&cond->seq, next, memory_order_release);
    return 0;
}

/* Destroy a condition variable (no resources to release). */
int pthread_cond_destroy(pthread_cond_t *cond)
{
    (void)cond;
    return 0;
}

#define KEY_MAX 64

static pthread_mutex_t key_lock = PTHREAD_MUTEX_INITIALIZER;
static void (*key_destructors[KEY_MAX])(void *);
static __thread void *key_values[KEY_MAX];

/* Create a thread-specific data key. */
int pthread_key_create(pthread_key_t *key, void (*destructor)(void *))
{
    if (!key)
        return EINVAL;
    pthread_mutex_lock(&key_lock);
    for (unsigned i = 0; i < KEY_MAX; ++i) {
        if (!key_destructors[i]) {
            key_destructors[i] = destructor ? destructor : (void *)1;
            *key = i;
            pthread_mutex_unlock(&key_lock);
            return 0;
        }
    }
    pthread_mutex_unlock(&key_lock);
    return EAGAIN;
}

/* Delete a thread-specific data key. */
int pthread_key_delete(pthread_key_t key)
{
    if (key >= KEY_MAX)
        return EINVAL;
    pthread_mutex_lock(&key_lock);
    key_destructors[key] = NULL;
    key_values[key] = NULL;
    pthread_mutex_unlock(&key_lock);
    return 0;
}

/* Store a per-thread value associated with a key. */
int pthread_setspecific(pthread_key_t key, const void *value)
{
    if (key >= KEY_MAX || !key_destructors[key])
        return EINVAL;
    key_values[key] = (void *)value;
    return 0;
}

/* Retrieve the thread-specific value for the key. */
void *pthread_getspecific(pthread_key_t key)
{
    if (key >= KEY_MAX || !key_destructors[key])
        return NULL;
    return key_values[key];
}

/* Create a new thread executing start_routine. */
int vlibc_pthread_create(pthread_t *thread, const void *attr,
                         void *(*start_routine)(void *), void *arg)
{
    if (!thread || !start_routine)
        return EINVAL;

    int ret = host_pthread_create(thread, NULL, start_routine, arg);
    if (ret == 0 && attr) {
        const pthread_attr_t *a = attr;
        if (a->detachstate == PTHREAD_CREATE_DETACHED)
            host_pthread_detach(*thread);
    }
    return ret;
}

/* Wait for the given thread to terminate. */
int vlibc_pthread_join(pthread_t thread, void **retval)
{
    return host_pthread_join(thread, retval);
}

/* Mark the thread as detached. */
int vlibc_pthread_detach(pthread_t thread)
{
    return host_pthread_detach(thread);
}

static pthread_mutex_t once_lock = PTHREAD_MUTEX_INITIALIZER;

/* Execute the initialization routine exactly once. */
int pthread_once(pthread_once_t *once_control, void (*init_routine)(void))
{
    if (atomic_load_explicit(&once_control->done, memory_order_acquire))
        return 0;
    pthread_mutex_lock(&once_lock);
    if (!atomic_load_explicit(&once_control->done, memory_order_acquire)) {
        init_routine();
        atomic_store_explicit(&once_control->done, 1, memory_order_release);
    }
    pthread_mutex_unlock(&once_lock);
    return 0;
}

/* Return the calling thread's ID. */
pthread_t vlibc_pthread_self(void)
{
    return host_pthread_self();
}

/* Compare two thread IDs for equality. */
int vlibc_pthread_equal(pthread_t a, pthread_t b)
{
    return host_pthread_equal(a, b);
}

/* Terminate the calling thread. */
void vlibc_pthread_exit(void *retval)
{
    host_pthread_exit(retval);
}

/* Request cancellation of a thread. */
int vlibc_pthread_cancel(pthread_t thread)
{
    return host_pthread_cancel(thread);
}

void vlibc_pthread_testcancel(void)
{
    host_pthread_testcancel();
}
