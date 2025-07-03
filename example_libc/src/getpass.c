/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the getpass functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "unistd.h"
#include "termios.h"
#include "string.h"
#include "io.h"
#include <fcntl.h>

char *getpass(const char *prompt)
{
    static __thread char buf[128];
    int fd = open("/dev/tty", O_RDWR);
    int need_close = 0;
    int out_fd;
    if (fd < 0) {
        fd = STDIN_FILENO;
        out_fd = STDERR_FILENO;
    } else {
        need_close = 1;
        out_fd = fd;
    }

    if (prompt) {
        ssize_t w = write(out_fd, prompt, strlen(prompt));
        if (w < 0) {
            if (need_close)
                close(fd);
            return NULL;
        }
    }

    struct termios old;
    int restore = 0;
    if (isatty(fd) && tcgetattr(fd, &old) == 0) {
        struct termios t = old;
        t.c_lflag &= ~(ECHO);
        tcsetattr(fd, TCSAFLUSH, &t);
        restore = 1;
    }

    size_t off = 0;
    while (off < sizeof(buf) - 1) {
        char c;
        ssize_t r = read(fd, &c, 1);
        if (r <= 0)
            break;
        if (c == '\n' || c == '\r')
            break;
        buf[off++] = c;
    }
    buf[off] = '\0';

    if (restore)
        tcsetattr(fd, TCSAFLUSH, &old);
    ssize_t w = write(out_fd, "\n", 1);
    if (w < 0) {
        if (need_close)
            close(fd);
        return NULL;
    }

    if (need_close)
        close(fd);

    return buf;
}
