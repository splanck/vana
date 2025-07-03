/*
 * BSD 2-Clause License
 *
 * Purpose: Declarations for basic scheduling helpers.
 */
#ifndef SCHED_H
#define SCHED_H

#include <sys/types.h>
#include "time.h"

int sched_yield(void);

/* scheduling policies */
#ifndef SCHED_OTHER
#define SCHED_OTHER 0
#endif
#ifndef SCHED_FIFO
#define SCHED_FIFO 1
#endif
#ifndef SCHED_RR
#define SCHED_RR   2
#endif

struct sched_param {
    int sched_priority;
};

/* scheduling priority interfaces */
#ifndef PRIO_PROCESS
#define PRIO_PROCESS 0
#endif
#ifndef PRIO_PGRP
#define PRIO_PGRP    1
#endif
#ifndef PRIO_USER
#define PRIO_USER    2
#endif
#ifndef PRIO_MIN
#define PRIO_MIN    -20
#endif
#ifndef PRIO_MAX
#define PRIO_MAX     20
#endif

int nice(int incr);
int getpriority(int which, int who);
int setpriority(int which, int who, int prio);

int sched_getscheduler(pid_t pid);
int sched_setscheduler(pid_t pid, int policy, const struct sched_param *param);
int sched_getparam(pid_t pid, struct sched_param *param);
int sched_setparam(pid_t pid, const struct sched_param *param);
int sched_get_priority_max(int policy);
int sched_get_priority_min(int policy);
int sched_rr_get_interval(pid_t pid, struct timespec *interval);

#endif /* SCHED_H */
