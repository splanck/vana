/*
 * BSD 2-Clause License
 *
 * Purpose: Declarations for simple regular expression matching.
 */
#ifndef REGEX_H
#define REGEX_H

#include <stddef.h>

/* simple regular expression interface */

typedef struct {
    void *impl;        /* internal compiled form */
    size_t re_nsub;    /* number of capture groups */
} regex_t;

typedef struct {
    int rm_so; /* start offset */
    int rm_eo; /* end offset */
} regmatch_t;

/* compile pattern into preg. Supported syntax includes:
 *  - literals and '.' wildcard
 *  - character classes with [] and POSIX classes like [:digit:]
 *  - repetition *, +, ?, and {m,n}
 *  - alternation with '|'
 *  - grouping via parentheses which also create numbered
 *    capture groups referenced as \1, \2, ...
 *  - anchors ^ and $
 */
int regcomp(regex_t *preg, const char *pattern, int cflags);

/* execute regex on string. If pmatch is provided it will be
 * filled with offsets for each capture group.
 */
int regexec(const regex_t *preg, const char *string,
            size_t nmatch, regmatch_t pmatch[], int eflags);

/* free compiled pattern */
void regfree(regex_t *preg);

/* error code when no match was found */
#define REG_NOMATCH 1

#endif /* REGEX_H */
