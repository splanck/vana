/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the process functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "process.h"
#include "errno.h"
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <signal.h>
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || \
    defined(__DragonFly__) || defined(__APPLE__)
#include <spawn.h>
extern int __posix_spawn(pid_t *, const char *,
                         const posix_spawn_file_actions_t *,
                         const posix_spawnattr_t *,
                         char *const [], char *const []) __asm("posix_spawn");
extern int __posix_spawnp(pid_t *, const char *,
                          const posix_spawn_file_actions_t *,
                          const posix_spawnattr_t *,
                          char *const [], char *const []) __asm("posix_spawnp");
#endif
#include "string.h"
#include "syscall.h"
#include "env.h"
#include "memory.h"
#include <stdarg.h>
#include <fcntl.h>
#include "stdio.h"
extern long syscall(long number, ...);

/* from atexit.c */
extern void __run_atexit(void);

/*
 * fork() - create a new process using the underlying fork or clone
 * system call. Returns the child's PID to the parent and 0 to the
 * child. On failure the wrapper stores the negative return value in
 * errno and returns -1.
 */


pid_t fork(void)
{
#ifdef SYS_fork
    long ret = vlibc_syscall(SYS_fork, 0, 0, 0, 0, 0, 0);
#else
    long ret = vlibc_syscall(SYS_clone, SIGCHLD, 0, 0, 0, 0, 0);
#endif
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (pid_t)ret;
}

/*
 * vfork() - create a new process and temporarily suspend the parent until the
 * child either execs or exits.  The original implementation invoked the
 * `SYS_vfork` syscall directly when available, but that can leave the parent
 * using a corrupted stack if the child calls into the C library.  To avoid
 * those issues we emulate `vfork` using `fork`.  This still returns 0 in the
 * child and the child's PID in the parent while allowing the child to safely
 * call into the library before exiting or executing a new program.
 */
pid_t vfork(void)
{
    return fork();
}

/*
 * execve() - execute a program via the SYS_execve system call wrapper. On
 * failure the return value from vlibc_syscall is negated and stored in errno
 * and -1 is returned.
 */
int execve(const char *pathname, char *const argv[], char *const envp[])
{
    long ret = vlibc_syscall(SYS_execve, (long)pathname, (long)argv, (long)envp, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
}

/*
 * fexecve() - execute a program referenced by an open file descriptor. If the
 * platform provides SYS_fexecve the call is made directly. When SYS_execveat
 * is available we call it with AT_EMPTY_PATH. Otherwise BSD systems fall back
 * to executing /dev/fd/<fd> and other platforms use /proc/self/fd/<fd> via
 * execve().
 */
int fexecve(int fd, char *const argv[], char *const envp[])
{
#ifdef SYS_fexecve
    long ret = vlibc_syscall(SYS_fexecve, fd, (long)argv, (long)envp, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#elif defined(SYS_execveat)
#ifndef AT_EMPTY_PATH
#define AT_EMPTY_PATH 0x1000
#endif
    long ret = vlibc_syscall(SYS_execveat, fd, (long)"", (long)argv,
                             (long)envp, AT_EMPTY_PATH, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
#else
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || \
    defined(__DragonFly__) || defined(__APPLE__)
    char path[32];
    snprintf(path, sizeof(path), "/dev/fd/%d", fd);
    return execve(path, argv, envp);
#else
    char path[64];
    snprintf(path, sizeof(path), "/proc/self/fd/%d", fd);
    return execve(path, argv, envp);
#endif
#endif
}

/*
 * execvp() - search PATH and execute a program using execve(). If the file
 * contains a slash it is executed directly. Returns -1 on failure with errno
 * set from execve attempts.
 */
int execvp(const char *file, char *const argv[])
{
    if (strchr(file, '/'))
        return execve(file, argv, environ);

    const char *path = getenv("PATH");
    if (!path)
        return execve(file, argv, environ);

    size_t flen = strlen(file);
    const char *p = path;
    int last_errno = ENOENT;

    while (1) {
        const char *end = strchr(p, ':');
        size_t len = end ? (size_t)(end - p) : strlen(p);

        size_t alloc_len = len ? (len + 1 + flen + 1) : (2 + flen + 1);
        char *buf = malloc(alloc_len);
        if (!buf)
            return -1;

        if (len) {
            memcpy(buf, p, len);
            buf[len] = '/';
            memcpy(buf + len + 1, file, flen);
            buf[len + 1 + flen] = '\0';
        } else {
            buf[0] = '.';
            buf[1] = '/';
            memcpy(buf + 2, file, flen);
            buf[2 + flen] = '\0';
        }

        execve(buf, argv, environ);
        last_errno = errno;
        free(buf);

        if (!end)
            break;
        p = end + 1;
    }

    errno = last_errno;
    return -1;
}

/*
 * execv() - execute a program with the current environment using execve().
 * Simply forwards the call and therefore follows the same error behaviour as
 * execve().
 */
int execv(const char *path, char *const argv[])
{
    return execve(path, argv, environ);
}

/*
 * vlibc_build_argv() - helper to construct a NULL terminated argv array from
 * a variable argument list.  Memory is allocated for the array and returned
 * via the out parameter.
 */
static int vlibc_build_argv(const char *arg, va_list ap, char ***out)
{
    va_list ap_copy;
    va_copy(ap_copy, ap);
    size_t count = 1;
    const char *p = arg;
    while (p) {
        p = va_arg(ap_copy, char *);
        if (p)
            count++;
    }
    va_end(ap_copy);

    char **argv = malloc(sizeof(char *) * (count + 1));
    if (!argv) {
        errno = ENOMEM;
        return -1;
    }

    argv[0] = (char *)arg;
    for (size_t i = 1; i <= count; i++) {
        argv[i] = (char *)va_arg(ap, char *);
        if (!argv[i]) {
            argv[count] = NULL;
            break;
        }
    }

    *out = argv;
    return 0;
}

/*
 * execl() - build an argument vector and invoke execve() on the given path.
 * Memory is allocated for the argv array and freed after the execve() call
 * returns. The error from execve() is propagated to the caller.
 */
int execl(const char *path, const char *arg, ...)
{
    va_list ap;
    va_start(ap, arg);
    char **argv;
    if (vlibc_build_argv(arg, ap, &argv) < 0) {
        va_end(ap);
        return -1;
    }
    va_end(ap);
    int r = execve(path, argv, environ);
    free(argv);
    return r;
}

/*
 * execlp() - build an argument vector and search the PATH for the program,
 * invoking execvp(). Errors from execvp() are returned to the caller.
 */
int execlp(const char *file, const char *arg, ...)
{
    va_list ap;
    va_start(ap, arg);
    char **argv;
    if (vlibc_build_argv(arg, ap, &argv) < 0) {
        va_end(ap);
        return -1;
    }
    va_end(ap);
    int r = execvp(file, argv);
    free(argv);
    return r;
}

/*
 * execle() - like execl() but allows an explicit environment pointer at the
 * end of the argument list. Errors from execve() are returned and errno is set
 * accordingly.
 */
int execle(const char *path, const char *arg, ...)
{
    va_list ap;
    va_start(ap, arg);
    va_list ap_copy;
    va_copy(ap_copy, ap);
    size_t count = 1;
    const char *p = arg;
    while ((p = va_arg(ap_copy, char *)))
        count++;
    char **envp = va_arg(ap_copy, char **);
    va_end(ap_copy);

    char **argv = malloc(sizeof(char *) * (count + 1));
    if (!argv) {
        errno = ENOMEM;
        va_end(ap);
        return -1;
    }

    argv[0] = (char *)arg;
    for (size_t i = 1; i <= count; i++) {
        argv[i] = (char *)va_arg(ap, char *);
        if (!argv[i]) {
            argv[count] = NULL;
            break;
        }
    }
    va_end(ap);

    int r = execve(path, argv, envp);
    free(argv);
    return r;
}

/*
 * waitpid() - wait for a child process using SYS_waitpid or SYS_wait4
 * depending on platform. Returns the pid of the terminated child or -1
 * with errno set on failure.
 */
pid_t waitpid(pid_t pid, int *status, int options)
{
#ifdef SYS_waitpid
    long ret = vlibc_syscall(SYS_waitpid, pid, (long)status, options, 0, 0, 0);
#else
    long ret = vlibc_syscall(SYS_wait4, pid, (long)status, options, 0, 0, 0);
#endif
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (pid_t)ret;
}

/*
 * wait() - convenience wrapper around waitpid() that waits for any child.
 */
pid_t wait(int *status)
{
    return waitpid(-1, status, 0);
}

/*
 * kill() - send a signal to a process via the SYS_kill system call wrapper.
 * On error errno is set from the negative syscall return value and -1 is
 * returned.
 */
int kill(pid_t pid, int sig)
{
    long ret = vlibc_syscall(SYS_kill, pid, sig, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (int)ret;
}

/*
 * getpid() - wrapper for SYS_getpid. Returns the process id or -1 on
 * failure with errno set.
 */
pid_t getpid(void)
{
    long ret = vlibc_syscall(SYS_getpid, 0, 0, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (pid_t)ret;
}

/*
 * getppid() - wrapper for SYS_getppid. Returns the parent's process id or -1
 * with errno set when the syscall fails.
 */
pid_t getppid(void)
{
    long ret = vlibc_syscall(SYS_getppid, 0, 0, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (pid_t)ret;
}

/*
 * setpgid() - set the process group using SYS_setpgid when available. On
 * platforms without the syscall a host implementation may be used or ENOSYS is
 * returned. Returns 0 on success or -1 on failure with errno set.
 */
int setpgid(pid_t pid, pid_t pgid)
{
#ifdef SYS_setpgid
    long ret = vlibc_syscall(SYS_setpgid, pid, pgid, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
#else
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_setpgid(pid_t, pid_t) __asm__("setpgid");
    return host_setpgid(pid, pgid);
#else
    errno = ENOSYS;
    return -1;
#endif
#endif
}

/*
 * getpgid() - obtain the process group ID using SYS_getpgid or a host
 * implementation. Returns -1 on error with errno set.
 */
pid_t getpgid(pid_t pid)
{
#ifdef SYS_getpgid
    long ret = vlibc_syscall(SYS_getpgid, pid, 0, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (pid_t)ret;
#else
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    extern pid_t host_getpgid(pid_t) __asm__("getpgid");
    return host_getpgid(pid);
#else
    errno = ENOSYS;
    return -1;
#endif
#endif
}

/*
 * getpgrp() - wrapper mapping to getpgid(0). Returns the calling process
 * group ID or -1 with errno set if retrieval fails.
 */
pid_t getpgrp(void)
{
    return getpgid(0);
}

/*
 * setpgrp() - wrapper mapping to setpgid(0, 0). Creates a new process group
 * for the calling process. Returns 0 on success or -1 on failure with errno
 * set.
 */
int setpgrp(void)
{
    return setpgid(0, 0);
}

/*
 * setsid() - create a new session using SYS_setsid when available. Returns
 * the new session ID or -1 on failure with errno set.
 */
pid_t setsid(void)
{
#ifdef SYS_setsid
    long ret = vlibc_syscall(SYS_setsid, 0, 0, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (pid_t)ret;
#else
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    extern pid_t host_setsid(void) __asm__("setsid");
    return host_setsid();
#else
    errno = ENOSYS;
    return -1;
#endif
#endif
}

/*
 * getsid() - fetch the session ID for a process using SYS_getsid or a host
 * helper. Returns the session ID or -1 if an error occurs.
 */
pid_t getsid(pid_t pid)
{
#ifdef SYS_getsid
    long ret = vlibc_syscall(SYS_getsid, pid, 0, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return (pid_t)ret;
#else
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    extern pid_t host_getsid(pid_t) __asm__("getsid");
    return host_getsid(pid);
#else
    errno = ENOSYS;
    return -1;
#endif
#endif
}


/*
 * _exit() - terminate the process immediately via SYS_exit without running
 * atexit handlers. This function does not return.
 */
void _exit(int status)
{
    vlibc_syscall(SYS_exit, status, 0, 0, 0, 0, 0);
    for (;;) {}
}

/*
 * exit() - run registered atexit handlers and then invoke _exit().
 */
void exit(int status)
{
    /* No stdio buffering yet, so nothing to flush. */
    __run_atexit();
    _exit(status);
}

/*
 * signal() - install a signal handler using either SYS_rt_sigaction or
 * SYS_signal depending on platform support. Returns the previous handler or
 * SIG_ERR on failure with errno set.
 */
sighandler_t signal(int signum, sighandler_t handler)
{
#ifdef SYS_rt_sigaction
    struct sigaction act, old;
    vmemset(&act, 0, sizeof(act));
    act.sa_handler = handler;
    long ret = vlibc_syscall(SYS_rt_sigaction, signum, (long)&act, (long)&old, sizeof(sigset_t), 0, 0);
    if (ret < 0) {
        errno = -ret;
        return SIG_ERR;
    }
    return old.sa_handler;
#else
    long ret = vlibc_syscall(SYS_signal, signum, (long)handler, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return SIG_ERR;
    }
    return (sighandler_t)ret;
#endif
}

struct posix_spawn_file_action {
    int type;
    int fd;
    int newfd;
    char *path;
    int oflag;
    mode_t mode;
};

#define SPAWN_ACTION_OPEN   1
#define SPAWN_ACTION_CLOSE  2
#define SPAWN_ACTION_DUP2   3
#define SPAWN_ACTION_CHDIR  4
#define SPAWN_ACTION_FCHDIR 5

/*
 * posix_spawn_file_actions_init() - initialise a file actions structure used
 * by posix_spawn(). Returns 0 on success or EINVAL if the pointer is NULL.
 */
int posix_spawn_file_actions_init(posix_spawn_file_actions_t *acts)
{
    if (!acts)
        return EINVAL;
    acts->actions = NULL;
    acts->count = 0;
    return 0;
}

/*
 * posix_spawn_file_actions_destroy() - free memory referenced by a file action
 * list. Returns 0 or EINVAL when passed a NULL pointer.
 */
int posix_spawn_file_actions_destroy(posix_spawn_file_actions_t *acts)
{
    if (!acts)
        return EINVAL;
    for (size_t i = 0; i < acts->count; i++)
        if (acts->actions[i].type == SPAWN_ACTION_OPEN ||
            acts->actions[i].type == SPAWN_ACTION_CHDIR)
            free(acts->actions[i].path);
    free(acts->actions);
    acts->actions = NULL;
    acts->count = 0;
    return 0;
}

/*
 * file_actions_add() - internal utility to append a new action to a
 * posix_spawn_file_actions_t list.  Reallocates the array as needed and
 * returns a pointer to the new slot via out.
 */
static int file_actions_add(posix_spawn_file_actions_t *acts,
                            struct posix_spawn_file_action **out)
{
    size_t n = acts->count + 1;
    struct posix_spawn_file_action *tmp =
        realloc(acts->actions, n * sizeof(*tmp));
    if (!tmp)
        return ENOMEM;
    acts->actions = tmp;
    *out = &acts->actions[acts->count];
    acts->count = n;
    return 0;
}

/*
 * posix_spawn_file_actions_addopen() - schedule an open() call in the child.
 * Allocates storage for the path string. Returns 0 or an error code.
 */
int posix_spawn_file_actions_addopen(posix_spawn_file_actions_t *acts, int fd,
                                     const char *path, int oflag, mode_t mode)
{
    if (!acts || !path)
        return EINVAL;
    struct posix_spawn_file_action *a;
    int r = file_actions_add(acts, &a);
    if (r)
        return r;
    a->type = SPAWN_ACTION_OPEN;
    a->fd = fd;
    a->oflag = oflag;
    a->mode = mode;
    a->path = strdup(path);
    if (!a->path) {
        acts->count--;
        if (acts->count == 0) {
            free(acts->actions);
            acts->actions = NULL;
        } else {
            struct posix_spawn_file_action *tmp =
                realloc(acts->actions, acts->count * sizeof(*tmp));
            if (tmp)
                acts->actions = tmp;
        }
        return ENOMEM;
    }
    return 0;
}

/*
 * posix_spawn_file_actions_adddup2() - schedule a dup2() in the spawned
 * process. Returns 0 on success or an error code.
 */
int posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t *acts, int fd,
                                     int newfd)
{
    if (!acts)
        return EINVAL;
    struct posix_spawn_file_action *a;
    int r = file_actions_add(acts, &a);
    if (r)
        return r;
    a->type = SPAWN_ACTION_DUP2;
    a->fd = fd;
    a->newfd = newfd;
    a->path = NULL;
    return 0;
}

/*
 * posix_spawn_file_actions_addclose() - close a file descriptor in the child.
 */
int posix_spawn_file_actions_addclose(posix_spawn_file_actions_t *acts, int fd)
{
    if (!acts)
        return EINVAL;
    struct posix_spawn_file_action *a;
    int r = file_actions_add(acts, &a);
    if (r)
        return r;
    a->type = SPAWN_ACTION_CLOSE;
    a->fd = fd;
    a->path = NULL;
    return 0;
}

/* Change directory before executing */
int posix_spawn_file_actions_addchdir(posix_spawn_file_actions_t *acts,
                                      const char *path)
{
    if (!acts || !path)
        return EINVAL;
    struct posix_spawn_file_action *a;
    int r = file_actions_add(acts, &a);
    if (r)
        return r;
    a->type = SPAWN_ACTION_CHDIR;
    a->fd = -1;
    a->path = strdup(path);
    if (!a->path) {
        acts->count--;
        if (acts->count == 0) {
            free(acts->actions);
            acts->actions = NULL;
        } else {
            struct posix_spawn_file_action *tmp =
                realloc(acts->actions, acts->count * sizeof(*tmp));
            if (tmp)
                acts->actions = tmp;
        }
        return ENOMEM;
    }
    return 0;
}

/* Change directory using an open descriptor */
int posix_spawn_file_actions_addfchdir(posix_spawn_file_actions_t *acts, int fd)
{
    if (!acts)
        return EINVAL;
    struct posix_spawn_file_action *a;
    int r = file_actions_add(acts, &a);
    if (r)
        return r;
    a->type = SPAWN_ACTION_FCHDIR;
    a->fd = fd;
    a->path = NULL;
    return 0;
}

/* Initialise spawn attributes structure */
int posix_spawnattr_init(posix_spawnattr_t *attr)
{
    if (!attr)
        return EINVAL;
    attr->flags = 0;
    sigemptyset(&attr->sigmask);
    sigemptyset(&attr->sigdefault);
    attr->pgroup = 0;
    return 0;
}

/* No dynamic state so just return 0 */
int posix_spawnattr_destroy(posix_spawnattr_t *attr)
{
    (void)attr;
    return 0;
}

/* Set flags field */
int posix_spawnattr_setflags(posix_spawnattr_t *attr, short flags)
{
    if (!attr)
        return EINVAL;
    attr->flags = flags;
    return 0;
}

/* Retrieve flags field */
int posix_spawnattr_getflags(const posix_spawnattr_t *attr, short *flags)
{
    if (!attr || !flags)
        return EINVAL;
    *flags = attr->flags;
    return 0;
}

/* Store a signal mask */
int posix_spawnattr_setsigmask(posix_spawnattr_t *attr, const sigset_t *mask)
{
    if (!attr || !mask)
        return EINVAL;
    attr->sigmask = *mask;
    return 0;
}

/* Copy out the stored signal mask */
int posix_spawnattr_getsigmask(const posix_spawnattr_t *attr, sigset_t *mask)
{
    if (!attr || !mask)
        return EINVAL;
    *mask = attr->sigmask;
    return 0;
}

/* Store the process group */
int posix_spawnattr_setpgroup(posix_spawnattr_t *attr, pid_t pgroup)
{
    if (!attr)
        return EINVAL;
    attr->pgroup = pgroup;
    return 0;
}

/* Copy out the stored process group */
int posix_spawnattr_getpgroup(const posix_spawnattr_t *attr, pid_t *pgroup)
{
    if (!attr || !pgroup)
        return EINVAL;
    *pgroup = attr->pgroup;
    return 0;
}

/*
 * vlibc_vfork() - small wrapper selecting vfork() when available and falling
 * back to fork() otherwise.  Used internally by posix_spawn().
 */
static __attribute__((unused)) pid_t vlibc_vfork(void)
{
    return vfork();
}

/*
 * posix_spawn() - create a new process using fork() and execve(). File actions
 * and spawn attributes are applied in the child. Errors detected before execve
 * are written through a pipe so the parent can return an errno value.
 */
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || \
    defined(__DragonFly__) || defined(__APPLE__)

int posix_spawn(pid_t *pid, const char *path,
                const posix_spawn_file_actions_t *file_actions,
                const posix_spawnattr_t *attrp,
                char *const argv[], char *const envp[])
{
    return __posix_spawn(pid, path, file_actions, attrp, argv, envp);
}

#else

int posix_spawn(pid_t *pid, const char *path,
                const posix_spawn_file_actions_t *file_actions,
                const posix_spawnattr_t *attrp,
                char *const argv[], char *const envp[])
{
    int errpipe[2];
    if (pipe(errpipe) < 0)
        return errno;
    fcntl(errpipe[0], F_SETFD, FD_CLOEXEC);
    fcntl(errpipe[1], F_SETFD, FD_CLOEXEC);

    pid_t cpid = fork();
    if (cpid < 0)
        {
            int err = errno;
            close(errpipe[0]);
            close(errpipe[1]);
            return err;
        }
    if (cpid == 0) {
        close(errpipe[0]);
        int err;

        if (attrp) {
            if (attrp->flags & POSIX_SPAWN_SETPGROUP)
                setpgid(0, attrp->pgroup);
            if (attrp->flags & POSIX_SPAWN_SETSIGMASK)
                sigprocmask(SIG_SETMASK, &attrp->sigmask, NULL);
            if (attrp->flags & POSIX_SPAWN_SETSIGDEF)
                for (int s = 1; s < 64; s++)
                    if (sigismember(&attrp->sigdefault, s))
                        signal(s, SIG_DFL);
        }

        if (file_actions) {
            for (size_t i = 0; i < file_actions->count; i++) {
                struct posix_spawn_file_action *a = &file_actions->actions[i];
                switch (a->type) {
                case SPAWN_ACTION_OPEN: {
                    int fd = open(a->path, a->oflag, a->mode);
                    if (fd < 0) {
                        err = errno;
                        ssize_t w = write(errpipe[1], &err, sizeof(err));
                        if (w < (ssize_t)sizeof(err))
                            _exit(127);
                        _exit(127);
                    }
                    if (fd != a->fd) {
                        if (dup2(fd, a->fd) < 0) {
                            err = errno;
                            ssize_t w = write(errpipe[1], &err, sizeof(err));
                            if (w < (ssize_t)sizeof(err))
                                _exit(127);
                            _exit(127);
                        }
                        close(fd);
                    }
                    break; }
                case SPAWN_ACTION_CLOSE:
                    if (close(a->fd) < 0) {
                        err = errno;
                        ssize_t w = write(errpipe[1], &err, sizeof(err));
                        if (w < (ssize_t)sizeof(err))
                            _exit(127);
                        _exit(127);
                    }
                    break;
                case SPAWN_ACTION_DUP2:
                    if (dup2(a->fd, a->newfd) < 0) {
                        err = errno;
                        ssize_t w = write(errpipe[1], &err, sizeof(err));
                        if (w < (ssize_t)sizeof(err))
                            _exit(127);
                        _exit(127);
                    }
                    break;
                case SPAWN_ACTION_CHDIR:
                    if (chdir(a->path) < 0) {
                        err = errno;
                        ssize_t w = write(errpipe[1], &err, sizeof(err));
                        if (w < (ssize_t)sizeof(err))
                            _exit(127);
                        _exit(127);
                    }
                    break;
                case SPAWN_ACTION_FCHDIR:
                    if (fchdir(a->fd) < 0) {
                        err = errno;
                        ssize_t w = write(errpipe[1], &err, sizeof(err));
                        if (w < (ssize_t)sizeof(err))
                            _exit(127);
                        _exit(127);
                    }
                    break;
                }
            }
        }

        execve(path, argv, envp ? envp : environ);
        err = errno;
        ssize_t w = write(errpipe[1], &err, sizeof(err));
        if (w < (ssize_t)sizeof(err))
            _exit(127);
        _exit(127);
    }
    close(errpipe[1]);
    int child_err = 0;
    ssize_t n = read(errpipe[0], &child_err, sizeof(child_err));
    close(errpipe[0]);
    if (n > 0) {
        waitpid(cpid, NULL, 0);
        return child_err;
    }
    if (pid)
        *pid = cpid;
    return 0;
}
#endif /* BSD posix_spawn */

/*
 * posix_spawnp() - search PATH for the executable and then call posix_spawn().
 * Returns an errno style value on failure.
 */
int posix_spawnp(pid_t *pid, const char *file,
                 const posix_spawn_file_actions_t *file_actions,
                 const posix_spawnattr_t *attrp,
                 char *const argv[], char *const envp[])
{
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || \
    defined(__DragonFly__) || defined(__APPLE__)
    return __posix_spawnp(pid, file, file_actions, attrp, argv, envp);
#else
    if (!file)
        return EINVAL;

    if (strchr(file, '/'))
        return posix_spawn(pid, file, file_actions, attrp, argv, envp);

    const char *path = getenv("PATH");
    if (!path)
        return posix_spawn(pid, file, file_actions, attrp, argv, envp);

    size_t flen = strlen(file);
    const char *p = path;
    int last_error = ENOENT;

    while (1) {
        const char *end = strchr(p, ':');
        size_t len = end ? (size_t)(end - p) : strlen(p);

        size_t alloc_len = len ? (len + 1 + flen + 1) : (2 + flen + 1);
        char *buf = malloc(alloc_len);
        if (!buf)
            return ENOMEM;

        if (len) {
            memcpy(buf, p, len);
            buf[len] = '/';
            memcpy(buf + len + 1, file, flen);
            buf[len + 1 + flen] = '\0';
        } else {
            buf[0] = '.';
            buf[1] = '/';
            memcpy(buf + 2, file, flen);
            buf[2 + flen] = '\0';
        }

        int r = posix_spawn(pid, buf, file_actions, attrp, argv, envp);
        if (r == 0) {
            free(buf);
            return 0;
        }
        last_error = r;
        free(buf);

        if (!end)
            break;
        p = end + 1;
    }

    return last_error;
#endif
}
