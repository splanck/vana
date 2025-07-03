/*
 * BSD 2-Clause License
 *
 * Purpose: Declarations for minimal threading support.
 */
#ifndef PTHREAD_H
#define PTHREAD_H

#include <sys/types.h>
#include <stdatomic.h>
#include "time.h"

/*
 * vlibc threads are thin wrappers around the host pthread
 * implementation.  Keep the type compatible with glibc's
 * definition to ease interoperability.
 */
typedef unsigned long pthread_t;

typedef struct {
    atomic_flag locked;
    int type;             /* mutex behavior */
    pthread_t owner;      /* thread holding the lock */
    unsigned recursion;   /* recursion depth for recursive mutexes */
} pthread_mutex_t;

typedef struct {
    int type;
} pthread_mutexattr_t;

#define PTHREAD_MUTEX_NORMAL 0
#define PTHREAD_MUTEX_RECURSIVE 1
#define PTHREAD_MUTEX_INITIALIZER { ATOMIC_FLAG_INIT, PTHREAD_MUTEX_NORMAL, 0, 0 }

typedef struct {
    atomic_int seq;  /* number of signals issued */
    atomic_int next; /* next ticket for waiting threads */
} pthread_cond_t;

typedef struct {
    atomic_int readers;
    atomic_int writer;
} pthread_rwlock_t;

typedef struct {
    unsigned count;
    atomic_uint waiting;
    atomic_uint phase;
    pthread_mutex_t lock;
    pthread_cond_t cond;
} pthread_barrier_t;

#define PTHREAD_BARRIER_SERIAL_THREAD -1

typedef unsigned int pthread_key_t;

typedef struct {
    atomic_int done;
} pthread_once_t;

#define PTHREAD_ONCE_INIT { ATOMIC_VAR_INIT(0) }
#define PTHREAD_CANCELED ((void *)-1)

typedef struct {
    int detachstate;
    size_t stacksize;
} pthread_attr_t;

#define PTHREAD_CREATE_JOINABLE 0
#define PTHREAD_CREATE_DETACHED 1

int pthread_attr_init(pthread_attr_t *attr);
int pthread_attr_destroy(pthread_attr_t *attr);
int pthread_attr_setdetachstate(pthread_attr_t *attr, int state);
int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *state);
int pthread_attr_setstacksize(pthread_attr_t *attr, size_t size);
int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *size);

int vlibc_pthread_create(pthread_t *thread, const void *attr,
                         void *(*start_routine)(void *), void *arg);
#define pthread_create vlibc_pthread_create
/* Spawn a new thread executing "start_routine" with "arg" via the
 * host's pthread implementation.  Returns 0 on success or an errno
 * style value on failure. */
int vlibc_pthread_join(pthread_t thread, void **retval);
#define pthread_join vlibc_pthread_join
/* Wait for the given thread to finish and retrieve its return value.
 * The return code mirrors the underlying pthread_join result. */
int vlibc_pthread_detach(pthread_t thread);
#define pthread_detach vlibc_pthread_detach
/* Mark the thread as detached so its resources are released on exit.
 * Returns 0 on success or an error number. */
pthread_t vlibc_pthread_self(void);
#define pthread_self vlibc_pthread_self
/* Obtain the identifier of the calling thread. */
int vlibc_pthread_equal(pthread_t a, pthread_t b);
#define pthread_equal vlibc_pthread_equal
/* Compare two thread identifiers for equality. */
void vlibc_pthread_exit(void *retval) __attribute__((noreturn));
#define pthread_exit vlibc_pthread_exit
/* Terminate the calling thread and make "retval" available to pthread_join. */
int vlibc_pthread_cancel(pthread_t thread);
#define pthread_cancel vlibc_pthread_cancel
/* Request cancellation of "thread". The target ends at the next cancellation
 * point and "pthread_join" returns PTHREAD_CANCELED. */
void vlibc_pthread_testcancel(void);
#define pthread_testcancel vlibc_pthread_testcancel
/* Check for any pending cancellation request for the calling thread and exit if
 * cancellation is pending. */

int pthread_mutex_init(pthread_mutex_t *mutex, void *attr);
/* Initialize a mutex using a simple spinlock. */
int pthread_mutex_destroy(pthread_mutex_t *mutex);
/* No-op for our lightweight mutex type. */
int pthread_mutex_lock(pthread_mutex_t *mutex);
/* Acquire the mutex, spinning until it becomes available. */
int pthread_mutex_trylock(pthread_mutex_t *mutex);
/* Attempt to acquire the mutex without blocking. */
int pthread_mutex_unlock(pthread_mutex_t *mutex);
/* Release the mutex. */

int pthread_mutexattr_init(pthread_mutexattr_t *attr);
/* Initialize mutex attribute object with default type. */
int pthread_mutexattr_destroy(pthread_mutexattr_t *attr);
/* Destroy a mutex attribute object (no-op). */
int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type);
/* Set the mutex behavior (normal or recursive). */
int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *type);
/* Retrieve the mutex type stored in the attribute object. */

int pthread_cond_init(pthread_cond_t *cond, void *attr);
/* Initialize a condition variable. */
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
/* Wait for a signal while releasing and re-acquiring the mutex. */
int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
                           const struct timespec *abstime);
/* Like pthread_cond_wait but with an absolute timeout. */
int pthread_cond_signal(pthread_cond_t *cond);
/* Wake one waiter on the condition variable. */
int pthread_cond_broadcast(pthread_cond_t *cond);
/* Wake all waiters on the condition variable. */
int pthread_cond_destroy(pthread_cond_t *cond);
/* Destroy a condition variable (no-op). */

int pthread_rwlock_init(pthread_rwlock_t *rwlock, void *attr);
/* Initialize a read-write lock allowing shared readers. */
int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock);
/* Acquire the lock for reading, blocking writers. */
int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock);
/* Acquire the lock for writing, waiting for readers to drain. */
int pthread_rwlock_unlock(pthread_rwlock_t *rwlock);
/* Release a read or write hold on the lock. */
int pthread_rwlock_destroy(pthread_rwlock_t *rwlock);
/* Destroy a read-write lock object (no-op). */

typedef struct {
    atomic_flag locked;
} pthread_spinlock_t;
int pthread_spin_init(pthread_spinlock_t *lock, int pshared);
/* Initialize a spin lock. "pshared" is ignored and only process-private
 * locks are supported. */
int pthread_spin_lock(pthread_spinlock_t *lock);
/* Acquire the spin lock, busy-waiting until it becomes available. */
int pthread_spin_trylock(pthread_spinlock_t *lock);
/* Try to acquire the spin lock without blocking. Returns EBUSY if held. */
int pthread_spin_unlock(pthread_spinlock_t *lock);
/* Release the spin lock. */
int pthread_spin_destroy(pthread_spinlock_t *lock);
/* Destroy a spin lock object (no-op). */

int pthread_key_create(pthread_key_t *key, void (*destructor)(void *));
/* Allocate a new thread-specific data key. */
int pthread_key_delete(pthread_key_t key);
/* Remove a previously created thread-specific data key. */
int pthread_setspecific(pthread_key_t key, const void *value);
/* Associate a value with the calling thread for the given key. */
void *pthread_getspecific(pthread_key_t key);
/* Retrieve the thread-specific value for the given key. */

int pthread_once(pthread_once_t *once_control, void (*init_routine)(void));
/* Ensure that "init_routine" is executed only once. */

int pthread_barrier_init(pthread_barrier_t *barrier, void *attr,
                         unsigned count);
/* Initialize a barrier that waits for "count" threads. */
int pthread_barrier_wait(pthread_barrier_t *barrier);
/* Wait until all participating threads have reached the barrier. */
int pthread_barrier_destroy(pthread_barrier_t *barrier);
/* Destroy a barrier object (no-op). */

#endif /* PTHREAD_H */
