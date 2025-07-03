/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the system functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "process.h"
#include "stdlib.h"
#include "string.h"
#include "errno.h"
#include "vlibc.h"

/*
 * system() - execute a shell command by forking and running the user's
 * preferred shell via execve(). The parent waits with waitpid() and
 * returns the child's exit status. On fork or waitpid failure -1 is
 * returned.
 */
int system(const char *command)
{
    if (!command)
        return 1;

    pid_t pid = fork();
    if (pid < 0)
        return -1;
    if (pid == 0) {
        const char *shell = vlibc_default_shell();
        char *argv[] = {(char *)shell, "-c", (char *)command, NULL};
        extern char **environ;
        execve(shell, argv, environ);
        _exit(127);
    }

    int status = 0;
    pid_t r;
    do {
        r = waitpid(pid, &status, 0);
    } while (r < 0 && errno == EINTR);
    if (r < 0)
        return -1;
    return status;
}

