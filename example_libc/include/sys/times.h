/*
 * BSD 2-Clause License
 *
 * Purpose: Process CPU time reporting helpers.
 */
#ifndef SYS_TIMES_H
#define SYS_TIMES_H

#include <sys/types.h>
#include "../time.h"
#include <time.h>

struct tms {
    clock_t tms_utime;   /* user CPU time */
    clock_t tms_stime;   /* system CPU time */
    clock_t tms_cutime;  /* user CPU time of children */
    clock_t tms_cstime;  /* system CPU time of children */
};

clock_t times(struct tms *buf);

#endif /* SYS_TIMES_H */
