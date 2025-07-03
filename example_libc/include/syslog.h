/*
 * BSD 2-Clause License
 *
 * Purpose: Declarations for syslog-style logging functions.
 */
#ifndef SYSLOG_H
#define SYSLOG_H

#include <stdarg.h>

/* syslog priorities */
#define LOG_EMERG   0
#define LOG_ALERT   1
#define LOG_CRIT    2
#define LOG_ERR     3
#define LOG_WARNING 4
#define LOG_NOTICE  5
#define LOG_INFO    6
#define LOG_DEBUG   7

/* facilities */
#define LOG_KERN    (0<<3)
#define LOG_USER    (1<<3)
#define LOG_DAEMON  (3<<3)
#define LOG_AUTH    (4<<3)
#define LOG_SYSLOG  (5<<3)
#define LOG_LOCAL0  (16<<3)
#define LOG_LOCAL1  (17<<3)
#define LOG_LOCAL2  (18<<3)
#define LOG_LOCAL3  (19<<3)
#define LOG_LOCAL4  (20<<3)
#define LOG_LOCAL5  (21<<3)
#define LOG_LOCAL6  (22<<3)
#define LOG_LOCAL7  (23<<3)

/* options */
#define LOG_PID     0x01

void openlog(const char *ident, int option, int facility);
void vsyslog(int priority, const char *format, va_list ap);
void syslog(int priority, const char *format, ...);
void closelog(void);

#endif /* SYSLOG_H */
