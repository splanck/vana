/*
 * BSD 2-Clause License
 *
 * Purpose: Declarations for filename matching helpers.
 */
#ifndef FNMATCH_H
#define FNMATCH_H

/* simple filename matching helper */

#define FNM_NOESCAPE 0x01
#define FNM_NOMATCH  1

int fnmatch(const char *pattern, const char *string, int flags);

#endif /* FNMATCH_H */
