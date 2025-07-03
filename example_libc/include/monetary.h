/*
 * BSD 2-Clause License
 *
 * Purpose: Declarations for monetary formatting.
 */
#ifndef MONETARY_H
#define MONETARY_H

#include <sys/types.h>
#include <stddef.h>

ssize_t strfmon(char *s, size_t max, const char *format, ...);

#endif /* MONETARY_H */
