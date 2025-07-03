/*
 * BSD 2-Clause License
 *
 * Purpose: Declarations for pseudo-terminal helpers.
 */
#ifndef PTY_H
#define PTY_H

#include <sys/types.h>
#include <stddef.h>
#include <termios.h>
#include "sys/ioctl.h"

int openpty(int *amaster, int *aslave, char *name, size_t namesz,
            struct termios *termp, struct winsize *winp);
int forkpty(int *amaster, char *name, size_t namesz,
            struct termios *termp, struct winsize *winp);

#endif /* PTY_H */
