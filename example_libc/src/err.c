/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms,
 * with or without modification, are permitted provided that the copyright
 * notice and this permission notice appear in all copies. This software is
 * provided "as is" without warranty.
 *
 * Purpose: Implements the err/warn helpers for vlibc.
 */

#include "err.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "errno.h"
#include "process.h"

/* Print a formatted warning to stderr, optionally appending errno. */
static void vwarn_internal(const char *fmt, va_list ap, int use_errno)
{
    int printed = 0;

    if (fmt && *fmt) {
        if (stderr)
            vfprintf(stderr, fmt, ap);
        else
            vdprintf(2, fmt, ap);
        printed = 1;
    }

    if (use_errno) {
        const char *msg = strerror(errno);
        if (stderr) {
            if (printed)
                fprintf(stderr, ": %s", msg);
            else
                fprintf(stderr, "%s", msg);
        } else {
            if (printed)
                dprintf(2, ": %s", msg);
            else
                dprintf(2, "%s", msg);
        }
        printed = 1;
    }

    if (printed) {
        if (stderr)
            fputc('\n', stderr);
        else
            dprintf(2, "\n");
    }
}

/* Warn with errno using a va_list. */
void vwarn(const char *fmt, va_list ap)
{
    vwarn_internal(fmt, ap, 1);
}

/* Variadic wrapper around vwarn(). */
void warn(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vwarn(fmt, ap);
    va_end(ap);
}

/* Warn without errno using a va_list. */
void vwarnx(const char *fmt, va_list ap)
{
    vwarn_internal(fmt, ap, 0);
}

/* Variadic wrapper around vwarnx(). */
void warnx(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vwarnx(fmt, ap);
    va_end(ap);
}

/*
 * Common helper used by err* functions. Prints the message and exits
 * with the supplied status code.
 */
static void __attribute__((noreturn))
verr_internal(int status, const char *fmt, va_list ap, int use_errno)
{
    /*
     * Use _exit() so a child process that hasn't run vlibc_init() doesn't
     * execute any atexit handlers inherited from the parent. This ensures
     * the warning message written above is the only output generated.
     */
    vwarn_internal(fmt, ap, use_errno);
    _exit(status);
    __builtin_unreachable();
}

/* Print warning with errno and exit. */
void verr(int status, const char *fmt, va_list ap)
{
    verr_internal(status, fmt, ap, 1);
}

/* Variadic wrapper around verr(). */
void err(int status, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    verr(status, fmt, ap);
    va_end(ap);
}

/* Print warning without errno and exit. */
void verrx(int status, const char *fmt, va_list ap)
{
    verr_internal(status, fmt, ap, 0);
}

/* Variadic wrapper around verrx(). */
void errx(int status, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    verrx(status, fmt, ap);
    va_end(ap);
}
