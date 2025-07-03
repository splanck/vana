/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the sleep functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "time.h"
#include "errno.h"
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

/*
 * nanosleep() - suspend execution for the time specified in 'req'.  If
 * interrupted, the remaining time is written to 'rem' when non-NULL.
 * Returns 0 on success or -1 on failure with errno set.
 */
extern int host_nanosleep(const struct timespec *, struct timespec *);
__asm__(".symver host_nanosleep,nanosleep@GLIBC_2.2.5");

int vlibc_nanosleep(const struct timespec *req, struct timespec *rem)
{
    int ret = host_nanosleep(req, rem);
    if (ret == -1)
        return -1;
    /* Ensure any pending cancellation is acted upon after sleeping */
    pthread_testcancel();
    return 0;
}

/*
 * usleep() - sleep for the specified number of microseconds.  Internally
 * converts the value to a timespec and calls nanosleep().
 */
int usleep(useconds_t usec)
{
    struct timespec ts;
    ts.tv_sec = usec / 1000000;
    ts.tv_nsec = (usec % 1000000) * 1000;
    return nanosleep(&ts, NULL);
}

/*
 * sleep() - suspend execution for the given number of seconds.  If the
 * call is interrupted by a signal, the number of seconds remaining is
 * returned; otherwise zero is returned.
 */
unsigned sleep(unsigned seconds)
{
    struct timespec ts = {seconds, 0};
    struct timespec rem = {0, 0};
    int r = nanosleep(&ts, &rem);
    if (r < 0 && errno == EINTR)
        return rem.tv_sec;
    return 0;
}

