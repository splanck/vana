/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the syslog functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "syslog.h"
#include "string.h"
#include "stdio.h"
#include "process.h"
#include "io.h"
#include "errno.h"
#include "sys/socket.h"
#include <fcntl.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdarg.h>

static int log_fd = -1;
static int log_facility = LOG_USER;
static int log_option = 0;
static char log_ident[32] = "";

/*
 * openlog() - configure the connection to the system logger. The
 * ident string is copied for later use when formatting messages.
 */
void openlog(const char *ident, int option, int facility)
{
    if (ident)
        strlcpy(log_ident, ident, sizeof(log_ident));
    else
        log_ident[0] = '\0';

    log_option = option;
    log_facility = facility;

    if (log_fd != -1) {
        close(log_fd);
        log_fd = -1;
    }

    int type = SOCK_DGRAM;
#ifdef SOCK_CLOEXEC
    type |= SOCK_CLOEXEC;
#endif
    log_fd = socket(AF_UNIX, type, 0);
    if (log_fd < 0)
        return;
#ifndef SOCK_CLOEXEC
    fcntl(log_fd, F_SETFD, FD_CLOEXEC);
#endif

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strlcpy(addr.sun_path, "/dev/log", sizeof(addr.sun_path));
    connect(log_fd, (struct sockaddr *)&addr, sizeof(addr));
}

/*
 * closelog() - close the connection to the system log daemon if it
 * has been opened via openlog().
 */
void closelog(void)
{
    if (log_fd != -1) {
        close(log_fd);
        log_fd = -1;
    }
}

/*
 * vsyslog() - format a message using a va_list and send it to the
 * system logger with the given priority.
 */
void vsyslog(int priority, const char *format, va_list ap)
{
    if (log_fd == -1)
        openlog(NULL, 0, LOG_USER);

    char msg[256];
    va_list aq;
    va_copy(aq, ap);
    int len = vsnprintf(msg, sizeof(msg), format, aq);
    va_end(aq);
    if (len < 0)
        return;
    if (len >= (int)sizeof(msg))
        len = sizeof(msg) - 1;

    char buf[300];
    int n = snprintf(buf, sizeof(buf), "<%d>", priority | log_facility);
    if (log_ident[0]) {
        n += snprintf(buf + n, sizeof(buf) - n, "%s", log_ident);
        if (log_option & LOG_PID)
            n += snprintf(buf + n, sizeof(buf) - n, "[%d]", getpid());
        n += snprintf(buf + n, sizeof(buf) - n, ": ");
    }
    if ((size_t)(n + len) >= sizeof(buf))
        len = sizeof(buf) - n - 1;
    memcpy(buf + n, msg, len);
    n += len;

    if (log_fd != -1) {
        ssize_t r = send(log_fd, buf, n, 0);
        if (r < 0)
            perror("vsyslog");
    }
}

/*
 * syslog() - convenience wrapper around vsyslog() that accepts a
 * variable argument list.
 */
void syslog(int priority, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vsyslog(priority, format, ap);
    va_end(ap);
}
