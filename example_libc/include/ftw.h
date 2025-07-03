/*
 * BSD 2-Clause License
 *
 * Purpose: Declarations for directory tree traversal helpers.
 */
#ifndef FTW_H
#define FTW_H

#include <sys/types.h>
#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif

/* flags for third argument of callback */
#define FTW_F   0
#define FTW_D   1
#define FTW_DNR 2
#define FTW_NS  3
#define FTW_SL  4
#define FTW_DP  5
#define FTW_SLN 6

/* flags for nftw */
#define FTW_PHYS  1
#define FTW_MOUNT 2
#define FTW_CHDIR 4
#define FTW_DEPTH 8

struct FTW {
    int base;
    int level;
};

typedef int (*ftw_func_t)(const char *, const struct stat *, int);

typedef int (*nftw_func_t)(const char *, const struct stat *, int, struct FTW *);

int ftw(const char *dir, ftw_func_t fn, int fdlimit);
int nftw(const char *dir, nftw_func_t fn, int fdlimit, int flags);

#ifdef __cplusplus
}
#endif

#endif /* FTW_H */
