/*
 * BSD 2-Clause License
 *
 * Purpose: Declarations for file tree walk helpers.
 */
#ifndef FTS_H
#define FTS_H

#include <sys/types.h>
#include <sys/stat.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ftsent FTSENT;
typedef struct _fts FTS;

struct _ftsent {
    FTSENT *fts_parent;
    FTSENT *fts_link;
    char *fts_accpath;
    char *fts_path;
    char *fts_name;
    size_t fts_namelen;
    int fts_level;
    int fts_info;
    struct stat fts_stat;
};

/* fts_info values */
#define FTS_F 1
#define FTS_D 2
#define FTS_DNR 3
#define FTS_NS 4
#define FTS_SL 5
#define FTS_DP 6

/* options */
#define FTS_PHYSICAL 0x01

FTS *fts_open(char * const *paths, int options,
              int (*compar)(const FTSENT **, const FTSENT **));
FTSENT *fts_read(FTS *ftsp);
int fts_close(FTS *ftsp);

#ifdef __cplusplus
}
#endif

#endif /* FTS_H */
