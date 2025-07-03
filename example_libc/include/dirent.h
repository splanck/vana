/*
 * BSD 2-Clause License
 *
 * Purpose: Declarations for directory iteration wrappers.
 */
#ifndef DIRENT_H
#define DIRENT_H

#include <sys/types.h>
#include <stddef.h>

/*
 * Pull in the system definitions of DIR and struct dirent. This header sits
 * ahead of the system directories on the include path, so use
 * include_next to reach the real header.
 */
#include_next <dirent.h>

/* Wrapper prototypes for the system directory functions. */
DIR *vlibc_opendir(const char *name);
struct dirent *vlibc_readdir(DIR *dirp);
int vlibc_closedir(DIR *dirp);

/* Directory scanning helpers */
int scandir(const char *dirp, struct dirent ***namelist,
            int (*filter)(const struct dirent *),
            int (*compar)(const struct dirent **, const struct dirent **));
int alphasort(const struct dirent **a, const struct dirent **b);

#define opendir  vlibc_opendir
#define readdir  vlibc_readdir
#define closedir vlibc_closedir

#endif /* DIRENT_H */
