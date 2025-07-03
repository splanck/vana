/*
 * BSD 2-Clause License
 *
 * Purpose: Declarations for program name helpers.
 */
#ifndef UTIL_H
#define UTIL_H

/* Retrieve the stored program name */
const char *getprogname(void);
/* Store the program name, typically called from main */
void setprogname(const char *name);

#endif /* UTIL_H */
