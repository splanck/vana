/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the popen functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "stdio.h"
#include "process.h"
#include "io.h"
#include "memory.h"
#include "string.h"
#include "errno.h"
#include "vlibc.h"

struct popen_file {
    FILE file;
    pid_t pid;
};

FILE *popen(const char *command, const char *mode)
{
    if (!command || !mode)
        return NULL;

    int read_mode = (mode[0] == 'r');
    int write_mode = (mode[0] == 'w');
    if (!read_mode && !write_mode)
        return NULL;

    int pipefd[2];
    if (pipe(pipefd) < 0)
        return NULL;

    pid_t pid = fork();
    if (pid < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        return NULL;
    }

    if (pid == 0) {
        if (read_mode) {
            dup2(pipefd[1], 1);
            close(pipefd[0]);
            close(pipefd[1]);
        } else {
            dup2(pipefd[0], 0);
            close(pipefd[1]);
            close(pipefd[0]);
        }
        const char *shell = vlibc_default_shell();
        char *argv[] = {(char *)shell, "-c", (char *)command, NULL};
        extern char **environ;
        execve(shell, argv, environ);
        _exit(127);
    }

    struct popen_file *pf = malloc(sizeof(struct popen_file));
    if (!pf) {
        close(pipefd[0]);
        close(pipefd[1]);
        return NULL;
    }
    memset(&pf->file, 0, sizeof(FILE));
    atomic_flag_clear(&pf->file.lock);
    pf->pid = pid;
    if (read_mode) {
        pf->file.fd = pipefd[0];
        close(pipefd[1]);
    } else {
        pf->file.fd = pipefd[1];
        close(pipefd[0]);
    }
    return &pf->file;
}

int pclose(FILE *stream)
{
    if (!stream)
        return -1;
    struct popen_file *pf = (struct popen_file *)stream;
    pid_t pid = pf->pid;
    fflush(stream);
    close(stream->fd);
    if (stream->buf && stream->buf_owned)
        free(stream->buf);
    free(pf);
    int status = 0;
    pid_t r;
    do {
        r = waitpid(pid, &status, 0);
    } while (r < 0 && errno == EINTR);
    if (r < 0)
        return -1;
    return status;
}

