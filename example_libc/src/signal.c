/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the signal functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "signal.h"
#include "errno.h"
#include <sys/syscall.h>
#include "syscall.h"
#include "memory.h"
#include "string.h"
#include "unistd.h"
#include <stddef.h>

#ifdef __x86_64__
__asm__(".globl vlibc_rt_sigreturn\n"
        "vlibc_rt_sigreturn:\n"
        "mov $15, %rax\n"
        "syscall\n");
extern void vlibc_rt_sigreturn(void);
#endif

struct k_sigaction {
    sighandler_t sa_handler;
    unsigned long sa_flags;
    void (*sa_restorer)(void);
    sigset_t sa_mask;
};

/*
 * sigaction() - examine or change the action associated with a signal.
 */
int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact)
{
#ifdef SYS_rt_sigaction
    struct k_sigaction kact, kold;
    struct k_sigaction *pact = NULL;
    struct k_sigaction *pold = NULL;
    if (act) {
        kact.sa_handler = act->sa_handler;
        kact.sa_flags = act->sa_flags;
#ifdef __x86_64__
        kact.sa_flags |= SA_RESTORER;
        kact.sa_restorer = vlibc_rt_sigreturn;
#else
        kact.sa_restorer = NULL;
#endif
        kact.sa_mask = act->sa_mask;
        pact = &kact;
    }
    if (oldact)
        pold = &kold;
    long ret = vlibc_syscall(SYS_rt_sigaction, signum, (long)pact,
                             (long)pold, sizeof(unsigned long), 0, 0);
    if (ret >= 0 && oldact) {
        oldact->sa_handler = kold.sa_handler;
        oldact->sa_mask = kold.sa_mask;
        oldact->sa_flags = kold.sa_flags;
        oldact->sa_restorer = kold.sa_restorer;
    }
#elif defined(SYS_sigaction)
    long ret = vlibc_syscall(SYS_sigaction, signum, (long)act,
                             (long)oldact, 0, 0, 0);
#else
    long ret = -ENOSYS;
#endif
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
}

/*
 * sigprocmask() - examine or modify the set of blocked signals.
 */
int sigprocmask(int how, const sigset_t *set, sigset_t *oldset)
{
#ifdef SYS_rt_sigprocmask
    /*
     * The kernel expects the size of one word (8 bytes on 64-bit
     * platforms) rather than sizeof(sigset_t).
     */
    long ret = vlibc_syscall(SYS_rt_sigprocmask, how, (long)set,
                             (long)oldset, sizeof(unsigned long), 0, 0);
#elif defined(SYS_sigprocmask)
    long ret = vlibc_syscall(SYS_sigprocmask, how, (long)set,
                             (long)oldset, 0, 0, 0);
#else
    long ret = -ENOSYS;
#endif
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
}

/*
 * sigemptyset() - initialize SET to contain no signals.
 */
int sigemptyset(sigset_t *set)
{
#ifdef VLIBC_HAS___SIGSET_T
    for (size_t i = 0; i < sizeof(set->__val) / sizeof(set->__val[0]); ++i)
        set->__val[i] = 0;
#else
    *set = 0;
#endif
    return 0;
}

/*
 * sigfillset() - initialize SET to contain all signals.
 */
int sigfillset(sigset_t *set)
{
#ifdef VLIBC_HAS___SIGSET_T
    for (size_t i = 0; i < sizeof(set->__val) / sizeof(set->__val[0]); ++i)
        set->__val[i] = ~0UL;
#else
    *set = ~(sigset_t)0;
#endif
    return 0;
}

/*
 * sigaddset() - add SIGNO to SET.
 */
int sigaddset(sigset_t *set, int signo)
{
    if (signo <= 0 || signo > 8 * (int)sizeof(sigset_t))
        return -1;
#ifdef VLIBC_HAS___SIGSET_T
    size_t idx = (signo - 1) / (8 * sizeof(unsigned long));
    unsigned long mask = 1UL << ((signo - 1) % (8 * sizeof(unsigned long)));
    set->__val[idx] |= mask;
#else
    *set |= (sigset_t)1 << (signo - 1);
#endif
    return 0;
}

/*
 * sigdelset() - remove SIGNO from SET.
 */
int sigdelset(sigset_t *set, int signo)
{
    if (signo <= 0 || signo > 8 * (int)sizeof(sigset_t))
        return -1;
#ifdef VLIBC_HAS___SIGSET_T
    size_t idx = (signo - 1) / (8 * sizeof(unsigned long));
    unsigned long mask = 1UL << ((signo - 1) % (8 * sizeof(unsigned long)));
    set->__val[idx] &= ~mask;
#else
    *set &= ~((sigset_t)1 << (signo - 1));
#endif
    return 0;
}

/*
 * sigismember() - check whether SIGNO is in SET.
 */
int sigismember(const sigset_t *set, int signo)
{
    if (signo <= 0 || signo > 8 * (int)sizeof(sigset_t))
        return 0;
#ifdef VLIBC_HAS___SIGSET_T
    size_t idx = (signo - 1) / (8 * sizeof(unsigned long));
    unsigned long mask = 1UL << ((signo - 1) % (8 * sizeof(unsigned long)));
    return (set->__val[idx] & mask) ? 1 : 0;
#else
    return (*set & ((sigset_t)1 << (signo - 1))) ? 1 : 0;
#endif
}

struct sig_name {
    int num;
    const char *name;
};

static const struct sig_name sig_names[] = {
    { SIGHUP, "Hangup" },
    { SIGINT, "Interrupt" },
    { SIGQUIT, "Quit" },
    { SIGILL, "Illegal instruction" },
    { SIGTRAP, "Trace/breakpoint trap" },
    { SIGABRT, "Aborted" },
    { SIGBUS, "Bus error" },
    { SIGFPE, "Floating point exception" },
    { SIGKILL, "Killed" },
    { SIGUSR1, "User signal 1" },
    { SIGSEGV, "Segmentation fault" },
    { SIGUSR2, "User signal 2" },
    { SIGPIPE, "Broken pipe" },
    { SIGALRM, "Alarm clock" },
    { SIGTERM, "Terminated" },
    { SIGCHLD, "Child exited" },
    { SIGCONT, "Continued" },
    { SIGSTOP, "Stopped (signal)" },
    { SIGTSTP, "Stopped" },
    { SIGTTIN, "Stopped (tty input)" },
    { SIGTTOU, "Stopped (tty output)" },
    { 0, NULL }
};
/*
 * strsignal() - return a human readable string for SIGNUM.
 */

char *strsignal(int signum)
{
    for (size_t i = 0; sig_names[i].name; ++i) {
        if (sig_names[i].num == signum)
            return (char *)sig_names[i].name;
    }
    return "Unknown signal";
}

/*
 * sigpending() - retrieve the set of signals pending for the process.
 */
int sigpending(sigset_t *set)
{
#ifdef SYS_rt_sigpending
    long ret = vlibc_syscall(SYS_rt_sigpending, (long)set,
                             sizeof(unsigned long), 0, 0, 0, 0);
#elif defined(SYS_sigpending)
    long ret = vlibc_syscall(SYS_sigpending, (long)set, 0, 0, 0, 0, 0);
#else
    long ret = -ENOSYS;
#endif
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
}

/*
 * sigsuspend() - replace the process signal mask and wait for a signal.
 */
int sigsuspend(const sigset_t *mask)
{
#ifdef SYS_rt_sigsuspend
    long ret = vlibc_syscall(SYS_rt_sigsuspend, (long)mask,
                             sizeof(unsigned long), 0, 0, 0, 0);
#elif defined(SYS_sigsuspend)
    long ret = vlibc_syscall(SYS_sigsuspend, (long)mask, 0, 0, 0, 0, 0);
#else
    long ret = -ENOSYS;
#endif
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
}
/*
 * pause() - sleep until a signal is delivered.
 */
int pause(void)
{
#ifdef SYS_pause
    long ret = vlibc_syscall(SYS_pause, 0, 0, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#else
    sigset_t mask;
    sigemptyset(&mask);
    return sigsuspend(&mask);
#endif
}

/*
 * sigtimedwait() - wait for a signal from SET with an optional timeout.
 */
int sigtimedwait(const sigset_t *set, siginfo_t *info,
                 const struct timespec *timeout)
{
#ifdef SYS_rt_sigtimedwait_time64
    long ret = vlibc_syscall(SYS_rt_sigtimedwait_time64, (long)set,
                             (long)info, (long)timeout,
                             sizeof(unsigned long), 0, 0);
#elif defined(SYS_rt_sigtimedwait)
    long ret = vlibc_syscall(SYS_rt_sigtimedwait, (long)set,
                             (long)info, (long)timeout,
                             sizeof(unsigned long), 0, 0);
#else
    (void)set; (void)info; (void)timeout;
    long ret = -ENOSYS;
#endif
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
}

/*
 * sigwaitinfo() - wait for any signal in SET and return info about it.
 */
int sigwaitinfo(const sigset_t *set, siginfo_t *info)
{
    return sigtimedwait(set, info, NULL);
}

/*
 * sigwait() - block until a signal in SET arrives and store its number.
 */
int sigwait(const sigset_t *set, int *sig)
{
    siginfo_t info;
    int r = sigtimedwait(set, &info, NULL);
    if (r < 0)
        return errno;
    if (sig)
        *sig = info.si_signo;
    return 0;
}
/*
 * sigqueue() - send a queued signal with accompanying value to PID.
 */

int sigqueue(pid_t pid, int signo, const union sigval value)
{
#ifdef SYS_rt_sigqueueinfo
    siginfo_t info;
    vmemset(&info, 0, sizeof(info));
    info.si_signo = signo;
    info.si_code = -1; /* SI_QUEUE */
    info._pad[0] = getpid();
    info._pad[1] = getuid();
    *(union sigval *)&info._pad[3] = value;
    long ret = vlibc_syscall(SYS_rt_sigqueueinfo, pid, signo,
                             (long)&info, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__) || defined(__APPLE__)
    extern int host_sigqueue(pid_t, int, const union sigval) __asm("sigqueue");
    return host_sigqueue(pid, signo, value);
#else
    (void)pid; (void)signo; (void)value;
    errno = ENOSYS;
    return -1;
#endif
}

/*
 * sigaltstack() - install or retrieve an alternate signal stack. When
 * SYS_sigaltstack is present the syscall is invoked directly. Otherwise the
 * host libc implementation is used when available via a weak reference.
 */
int sigaltstack(const stack_t *ss, stack_t *old)
{
#ifdef SYS_sigaltstack
    long ret = vlibc_syscall(SYS_sigaltstack, (long)ss, (long)old,
                             0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
#else
    extern int host_sigaltstack(const stack_t *, stack_t *)
        __asm("sigaltstack") __attribute__((weak));
    if (host_sigaltstack)
        return host_sigaltstack(ss, old);
    (void)ss; (void)old;
    errno = ENOSYS;
    return -1;
#endif
}
