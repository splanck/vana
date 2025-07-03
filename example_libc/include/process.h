/*
 * BSD 2-Clause License
 *
 * Purpose: Declarations for process creation and control.
 */
#ifndef PROCESS_H
#define PROCESS_H

#define _POSIX_C_SOURCE 200809L
#include <sys/types.h>
#include <signal.h>

pid_t fork(void);
int execve(const char *pathname, char *const argv[], char *const envp[]);
int fexecve(int fd, char *const argv[], char *const envp[]);
int execvp(const char *file, char *const argv[]);
int execv(const char *path, char *const argv[]);
int execl(const char *path, const char *arg, ...);
int execlp(const char *file, const char *arg, ...);
int execle(const char *path, const char *arg, ...);
pid_t wait(int *status);
pid_t waitpid(pid_t pid, int *status, int options);
int kill(pid_t pid, int sig);
pid_t getpid(void);
pid_t getppid(void);
int setpgid(pid_t pid, pid_t pgid);
pid_t getpgid(pid_t pid);
int setpgrp(void);
pid_t getpgrp(void);
pid_t setsid(void);
pid_t getsid(pid_t pid);

void _exit(int status);
void exit(int status);

typedef void (*sighandler_t)(int);
sighandler_t signal(int signum, sighandler_t handler);

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || \
    defined(__DragonFly__) || defined(__APPLE__)
#include <spawn.h>
#else

/* posix_spawn attributes */
typedef struct {
    short flags;
    sigset_t sigmask;
    sigset_t sigdefault;
    pid_t pgroup;
} posix_spawnattr_t;

int posix_spawnattr_init(posix_spawnattr_t *attr);
int posix_spawnattr_destroy(posix_spawnattr_t *attr);
int posix_spawnattr_setflags(posix_spawnattr_t *attr, short flags);
int posix_spawnattr_getflags(const posix_spawnattr_t *attr, short *flags);
int posix_spawnattr_setsigmask(posix_spawnattr_t *attr, const sigset_t *mask);
int posix_spawnattr_getsigmask(const posix_spawnattr_t *attr, sigset_t *mask);
int posix_spawnattr_setpgroup(posix_spawnattr_t *attr, pid_t pgroup);
int posix_spawnattr_getpgroup(const posix_spawnattr_t *attr, pid_t *pgroup);

/* spawn flags */
#define POSIX_SPAWN_RESETIDS   0x01
#define POSIX_SPAWN_SETPGROUP  0x02
#define POSIX_SPAWN_SETSIGDEF  0x04
#define POSIX_SPAWN_SETSIGMASK 0x08

/* file action types */
typedef struct {
    struct posix_spawn_file_action *actions;
    size_t count;
} posix_spawn_file_actions_t;

int posix_spawn_file_actions_init(posix_spawn_file_actions_t *acts);
int posix_spawn_file_actions_destroy(posix_spawn_file_actions_t *acts);
int posix_spawn_file_actions_addopen(posix_spawn_file_actions_t *acts, int fd,
                                     const char *path, int oflag, mode_t mode);
int posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t *acts, int fd,
                                     int newfd);
int posix_spawn_file_actions_addclose(posix_spawn_file_actions_t *acts, int fd);
int posix_spawn_file_actions_addchdir(posix_spawn_file_actions_t *acts,
                                      const char *path);
int posix_spawn_file_actions_addfchdir(posix_spawn_file_actions_t *acts,
                                       int fd);
int posix_spawn(pid_t *pid, const char *path,
                const posix_spawn_file_actions_t *file_actions,
                const posix_spawnattr_t *attrp,
                char *const argv[], char *const envp[]);
int posix_spawnp(pid_t *pid, const char *file,
                 const posix_spawn_file_actions_t *file_actions,
                 const posix_spawnattr_t *attrp,
                 char *const argv[], char *const envp[]);

#endif /* BSD */

#endif /* PROCESS_H */
