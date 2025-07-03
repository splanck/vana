/*
 * BSD 2-Clause License
 *
 * Purpose: Minimal asynchronous I/O wrappers for vlibc.
 */

#include "aio.h"
#include "io.h"
#include "memory.h"
#include "errno.h"
#include "pthread.h"
#include "string.h"
#include "unistd.h"

#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)

extern int host_aio_read(struct aiocb *) __asm__("aio_read");
extern int host_aio_write(struct aiocb *) __asm__("aio_write");
extern int host_aio_error(const struct aiocb *) __asm__("aio_error");
extern ssize_t host_aio_return(struct aiocb *) __asm__("aio_return");
extern int host_aio_suspend(const struct aiocb *const [], int,
                            const struct timespec *) __asm__("aio_suspend");
extern int host_aio_cancel(int, struct aiocb *) __asm__("aio_cancel");
extern int host_lio_listio(int, struct aiocb *const [], int,
                           struct sigevent *) __asm__("lio_listio");

int aio_read(struct aiocb *cb) { return host_aio_read(cb); }
int aio_write(struct aiocb *cb) { return host_aio_write(cb); }
int aio_error(const struct aiocb *cb) { return host_aio_error(cb); }
ssize_t aio_return(struct aiocb *cb) { return host_aio_return(cb); }
int aio_suspend(const struct aiocb *const list[], int n,
                const struct timespec *tmo)
{ return host_aio_suspend(list, n, tmo); }
int aio_cancel(int fd, struct aiocb *cb) { return host_aio_cancel(fd, cb); }
int lio_listio(int mode, struct aiocb *const list[], int n,
               struct sigevent *sig)
{ return host_lio_listio(mode, list, n, sig); }

#else /* simple emulation */

struct aio_task {
    struct aiocb *cb;
    pthread_t thr;
    ssize_t ret;
    int err;
    int done;
};

static pthread_mutex_t aio_lock;
static pthread_cond_t aio_cond;

static void complete_task(struct aio_task *t)
{
    pthread_mutex_lock(&aio_lock);
    t->done = 1;
    pthread_cond_broadcast(&aio_cond);
    pthread_mutex_unlock(&aio_lock);
}

static void *reader(void *arg)
{
    struct aio_task *t = arg;
    struct aiocb *cb = t->cb;
    if (cb->aio_offset != -1)
        lseek(cb->aio_fildes, cb->aio_offset, SEEK_SET);
    t->ret = read(cb->aio_fildes, (void *)cb->aio_buf, cb->aio_nbytes);
    t->err = (t->ret < 0) ? errno : 0;
    complete_task(t);
    return NULL;
}

static void *writer(void *arg)
{
    struct aio_task *t = arg;
    struct aiocb *cb = t->cb;
    if (cb->aio_offset != -1)
        lseek(cb->aio_fildes, cb->aio_offset, SEEK_SET);
    t->ret = write(cb->aio_fildes, (const void *)cb->aio_buf, cb->aio_nbytes);
    t->err = (t->ret < 0) ? errno : 0;
    complete_task(t);
    return NULL;
}

static int start_task(struct aiocb *cb, void *(*fn)(void *))
{
    struct aio_task *t = malloc(sizeof(*t));
    if (!t) {
        errno = ENOMEM;
        return -1;
    }
    memset(t, 0, sizeof(*t));
    t->cb = cb;
    cb->__reserved[0] = (long)t;
    int r = pthread_create(&t->thr, NULL, fn, t);
    if (r != 0) {
        free(t);
        cb->__reserved[0] = 0;
        errno = r;
        return -1;
    }
    return 0;
}

int aio_read(struct aiocb *cb)
{
    return start_task(cb, reader);
}

int aio_write(struct aiocb *cb)
{
    return start_task(cb, writer);
}

static struct aio_task *get_task(const struct aiocb *cb)
{
    return (struct aio_task *)(cb ? (void *)cb->__reserved[0] : NULL);
}

int aio_error(const struct aiocb *cb)
{
    struct aio_task *t = get_task(cb);
    if (!t)
        return 0;
    pthread_mutex_lock(&aio_lock);
    int err = t->done ? t->err : EINPROGRESS;
    pthread_mutex_unlock(&aio_lock);
    return err;
}

ssize_t aio_return(struct aiocb *cb)
{
    struct aio_task *t = get_task(cb);
    if (!t)
        return -1;
    pthread_join(t->thr, NULL);
    ssize_t ret = t->ret;
    int err = t->err;
    free(t);
    cb->__reserved[0] = 0;
    if (ret < 0) {
        errno = err;
        return -1;
    }
    return ret;
}

int aio_suspend(const struct aiocb *const list[], int n,
                const struct timespec *timeout)
{
    struct timespec abs;
    if (timeout) {
        clock_gettime(CLOCK_REALTIME, &abs);
        abs.tv_sec += timeout->tv_sec;
        abs.tv_nsec += timeout->tv_nsec;
        if (abs.tv_nsec >= 1000000000) {
            abs.tv_sec++;
            abs.tv_nsec -= 1000000000;
        }
    }
    pthread_mutex_lock(&aio_lock);
    while (1) {
        for (int i = 0; i < n; i++) {
            struct aio_task *t = get_task(list[i]);
            if (t && t->done) {
                pthread_mutex_unlock(&aio_lock);
                return 0;
            }
        }
        if (timeout) {
            int r = pthread_cond_timedwait(&aio_cond, &aio_lock, &abs);
            if (r == ETIMEDOUT) {
                pthread_mutex_unlock(&aio_lock);
                errno = EAGAIN;
                return -1;
            }
        } else {
            pthread_cond_wait(&aio_cond, &aio_lock);
        }
    }
}

int aio_cancel(int fd, struct aiocb *cb)
{
    if (!cb || cb->aio_fildes != fd)
        return AIO_ALLDONE;

    struct aio_task *t = get_task(cb);
    if (!t)
        return AIO_ALLDONE;
    pthread_mutex_lock(&aio_lock);
    if (t->done) {
        pthread_mutex_unlock(&aio_lock);
        return AIO_ALLDONE;
    }
    pthread_cancel(t->thr);
    pthread_mutex_unlock(&aio_lock);
    pthread_join(t->thr, NULL);
    free(t);
    cb->__reserved[0] = 0;
    return AIO_CANCELED;
}

int lio_listio(int mode, struct aiocb *const list[], int n,
               struct sigevent *sig)
{
    (void)sig; /* signals not supported */
    for (int i = 0; i < n; i++) {
        if (!list[i])
            continue;
        if (list[i]->aio_lio_opcode == LIO_WRITE)
            aio_write(list[i]);
        else if (list[i]->aio_lio_opcode == LIO_READ)
            aio_read(list[i]);
    }
    if (mode == LIO_WAIT) {
        while (1) {
            int done = 1;
            for (int i = 0; i < n; i++) {
                struct aio_task *t = get_task(list[i]);
                if (t && !t->done) {
                    done = 0;
                    break;
                }
            }
            if (done)
                break;
            aio_suspend((const struct aiocb *const *)list, n, NULL);
        }
    }
    return 0;
}

#endif
