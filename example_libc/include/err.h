/*
 * BSD 2-Clause License
 *
 * Purpose: Declarations for simple error reporting helpers.
 */
#ifndef ERR_H
#define ERR_H

#include <stdarg.h>

/* Warn with the current errno message appended */
void warn(const char *fmt, ...);
void vwarn(const char *fmt, va_list ap);

/* Warn without errno */
void warnx(const char *fmt, ...);
void vwarnx(const char *fmt, va_list ap);

/* Print warning and exit */
void err(int status, const char *fmt, ...) __attribute__((noreturn));
void verr(int status, const char *fmt, va_list ap) __attribute__((noreturn));

/* Print message without errno and exit */
void errx(int status, const char *fmt, ...) __attribute__((noreturn));
void verrx(int status, const char *fmt, va_list ap) __attribute__((noreturn));

#endif /* ERR_H */
