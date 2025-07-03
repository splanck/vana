/*
 * BSD 2-Clause License
 *
 * Purpose: Declarations for signal handling helpers.
 */
#ifndef SIGNAL_H
#define SIGNAL_H

#include <sys/types.h>
#include "time.h"

#if defined(__has_include)
#  if __has_include("/usr/include/x86_64-linux-gnu/bits/types/__sigset_t.h")
#    include "/usr/include/x86_64-linux-gnu/bits/types/__sigset_t.h"
#    define VLIBC_HAS___SIGSET_T 1
#  elif __has_include("/usr/include/bits/types/__sigset_t.h")
#    include "/usr/include/bits/types/__sigset_t.h"
#    define VLIBC_HAS___SIGSET_T 1
#  endif
#endif

/* basic signal numbers */
#define SIGHUP    1
#define SIGINT    2
#define SIGQUIT   3
#define SIGILL    4
#define SIGTRAP   5
#define SIGABRT   6
#define SIGIOT    6
#define SIGBUS    7
#define SIGFPE    8
#define SIGKILL   9
#define SIGUSR1   10
#define SIGSEGV   11
#define SIGUSR2   12
#define SIGPIPE   13
#define SIGALRM   14
#define SIGTERM   15
#define SIGSTKFLT 16
#define SIGCHLD   17
#define SIGCONT   18
#define SIGSTOP   19
#define SIGTSTP   20
#define SIGTTIN   21
#define SIGTTOU   22
#define SIGURG    23
#define SIGXCPU   24
#define SIGXFSZ   25
#define SIGVTALRM 26
#define SIGPROF   27
#define SIGWINCH  28
#define SIGIO     29
#define SIGPOLL   SIGIO
#define SIGPWR    30
#define SIGSYS    31
#define SIGUNUSED 31

#define SIGRTMIN  32
#define SIGRTMAX  64

typedef void (*sighandler_t)(int);
typedef int sig_atomic_t;

#define SIG_DFL ((sighandler_t)0)
#define SIG_IGN ((sighandler_t)1)
#define SIG_ERR ((sighandler_t)-1)

/* flags for struct sigaction */
#ifndef SA_RESTORER
#define SA_RESTORER 0x04000000
#endif

/* how argument for sigprocmask */
#define SIG_BLOCK   0
#define SIG_UNBLOCK 1
#define SIG_SETMASK 2

#ifdef VLIBC_HAS___SIGSET_T
typedef __sigset_t sigset_t;
#else
typedef unsigned long sigset_t;
#endif

struct sigaction {
    sighandler_t sa_handler;
    sigset_t sa_mask;
    int sa_flags;
    void (*sa_restorer)(void);
};

int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);
int sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
int sigemptyset(sigset_t *set);
int sigfillset(sigset_t *set);
int sigaddset(sigset_t *set, int signo);
int sigdelset(sigset_t *set, int signo);
int sigismember(const sigset_t *set, int signo);
char *strsignal(int signum);

#ifdef VLIBC_HAS_SYS_UCONTEXT
#include <sys/ucontext.h>
#else
typedef struct {
    void  *ss_sp;
    size_t ss_size;
    int    ss_flags;
} stack_t;
#endif

#define SS_ONSTACK  1
#define SS_DISABLE  2
#define MINSIGSTKSZ 2048
#define SIGSTKSZ    8192

int sigaltstack(const stack_t *ss, stack_t *old);

typedef struct siginfo {
    int si_signo;
    int si_errno;
    int si_code;
    int _pad[29];
} siginfo_t;

int pause(void);
int sigsuspend(const sigset_t *mask);
int sigpending(sigset_t *set);
int sigwait(const sigset_t *set, int *sig);
int sigwaitinfo(const sigset_t *set, siginfo_t *info);
int sigtimedwait(const sigset_t *set, siginfo_t *info,
                 const struct timespec *timeout);
int sigqueue(pid_t pid, int signo, const union sigval value);

#endif /* SIGNAL_H */
