/*
 * BSD 2-Clause License
 *
 * Purpose: Implements clock_nanosleep for vlibc. When the kernel
 * lacks a direct syscall the function falls back to nanosleep for
 * relative delays and emulates absolute sleeps using clock_gettime.
 */

#include "time.h"
#include "errno.h"
#include <sys/syscall.h>
#include <unistd.h>
#include "syscall.h"

int clock_nanosleep(clockid_t clk_id, int flags,
                    const struct timespec *req, struct timespec *rem)
{
#ifdef SYS_clock_nanosleep
    long ret = vlibc_syscall(SYS_clock_nanosleep, clk_id, flags,
                             (long)req, (long)rem, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
#else
    if (!(flags & TIMER_ABSTIME)) {
        (void)clk_id;
        return nanosleep(req, rem);
    }

    struct timespec now;
    while (1) {
        if (clock_gettime(clk_id, &now) < 0)
            return -1;
        long sec = req->tv_sec - now.tv_sec;
        long nsec = req->tv_nsec - now.tv_nsec;
        if (nsec < 0) {
            nsec += 1000000000L;
            --sec;
        }
        if (sec < 0 || (sec == 0 && nsec <= 0))
            return 0;
        struct timespec rel = {sec, nsec};
        if (nanosleep(&rel, NULL) == 0)
            return 0;
        if (errno != EINTR)
            return -1;
    }
#endif
}
