/*
 * BSD 2-Clause License
 *
 * Purpose: Declarations for pathname globbing helpers.
 */
#ifndef GLOB_H
#define GLOB_H

#include <stddef.h>

/* result structure for glob matches */
typedef struct {
    size_t gl_pathc;  /* number of matched paths */
    char **gl_pathv;  /* NULL terminated list of matches */
    size_t gl_offs;   /* reserved */
} glob_t;

/* glob flags */
#define GLOB_NOSORT   0x01
#define GLOB_NOESCAPE 0x02

/* error codes */
#define GLOB_NOMATCH  1
#define GLOB_NOSPACE  2
#define GLOB_ABORTED  3

int glob(const char *pattern, int flags,
         int (*errfunc)(const char *epath, int eerrno), glob_t *pglob);
void globfree(glob_t *pglob);

#endif /* GLOB_H */
