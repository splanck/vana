/*
 * BSD 2-Clause License
 *
 * Purpose: Status macros for process waiting.
 */
#ifndef SYS_WAIT_H
#define SYS_WAIT_H

#include <sys/types.h>

/* Waitpid option flags */
#ifndef WNOHANG
#define WNOHANG    1
#endif
#ifndef WUNTRACED
#define WUNTRACED  2
#endif
#ifndef WCONTINUED
#define WCONTINUED 8
#endif

/* Status decoding macros */
#define WEXITSTATUS(status)   (((status) >> 8) & 0xff)
#define WTERMSIG(status)      ((status) & 0x7f)
#define WSTOPSIG(status)      WEXITSTATUS(status)
#define WIFEXITED(status)     (WTERMSIG(status) == 0)
#define WIFSIGNALED(status)   (WTERMSIG(status) != 0 && WTERMSIG(status) != 0x7f)
#define WIFSTOPPED(status)    (((status) & 0xff) == 0x7f)
#define WIFCONTINUED(status)  ((status) == 0xffff)
#define WCOREDUMP(status)     ((status) & 0x80)

#endif /* SYS_WAIT_H */
