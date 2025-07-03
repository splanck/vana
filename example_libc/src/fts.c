/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the fts functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "fts.h"
#include "dirent.h"
#include "memory.h"
#include "string.h"
#include "errno.h"
#include "unistd.h"
#include <stdint.h>

struct node {
    struct node *next;
    char *path;
    int level;
};

struct _fts {
    struct node *head;
    struct node *tail;
    FTSENT *cur;
    int options;
    int (*compar)(const FTSENT **, const FTSENT **);
};

/*
 * queue_push() - append a new path node to the internal traversal queue.
 *
 * PATH is duplicated and stored along with LEVEL indicating depth in the
 * hierarchy.  Returns 0 on success or -1 if memory allocation fails.
 */
static int queue_push(FTS *fts, const char *path, int level)
{
    struct node *n = malloc(sizeof(*n));
    if (!n)
        return -1;
    n->path = strdup(path);
    if (!n->path) {
        free(n);
        return -1;
    }
    n->level = level;
    n->next = NULL;
    if (fts->tail)
        fts->tail->next = n;
    else
        fts->head = n;
    fts->tail = n;
    return 0;
}

/*
 * queue_pop() - remove and return the next queued path node.
 *
 * Returns a pointer to the node or NULL if the queue is empty.
 */
static struct node *queue_pop(FTS *fts)
{
    struct node *n = fts->head;
    if (n) {
        fts->head = n->next;
        if (!fts->head)
            fts->tail = NULL;
    }
    return n;
}

/*
 * fts_open() - start a file hierarchy traversal.
 *
 * PATHS is a NULL terminated list of root paths to traverse. OPTIONS
 * controls traversal behaviour and COMPAR is an optional sorting
 * callback.  Returns a new FTS handle or NULL on allocation failure.
 * Paths are queued and returned incrementally by fts_read().
 */
FTS *fts_open(char * const *paths, int options,
              int (*compar)(const FTSENT **, const FTSENT **))
{
    if (!paths)
        return NULL;
    FTS *fts = calloc(1, sizeof(*fts));
    if (!fts)
        return NULL;
    fts->options = options;
    fts->compar = compar;
    for (size_t i = 0; paths[i]; i++) {
        if (queue_push(fts, paths[i], 0) < 0) {
            fts_close(fts);
            errno = ENOMEM;
            return NULL;
        }
    }
    return fts;
}

/*
 * free_entry - release memory for one traversal entry.
 */
static void free_entry(FTSENT *e)
{
    if (!e)
        return;
    free(e->fts_path);
    free(e);
}

/*
 * fts_read() - return the next entry from the traversal queue.
 *
 * The returned FTSENT describes the next path in the hierarchy.  If a
 * directory is encountered its children are queued for later processing.
 * NULL is returned when traversal is complete or on allocation failure.
 */
FTSENT *fts_read(FTS *fts)
{
    if (!fts)
        return NULL;
    free_entry(fts->cur);
    fts->cur = NULL;

    struct node *n = queue_pop(fts);
    if (!n)
        return NULL;

    struct stat st;
    memset(&st, 0, sizeof(st));
    int r;
    if (fts->options & FTS_PHYSICAL)
        r = lstat(n->path, &st);
    else
        r = stat(n->path, &st);

    FTSENT *ent = calloc(1, sizeof(*ent));
    if (!ent) {
        free(n->path);
        free(n);
        return NULL;
    }
    ent->fts_path = n->path;
    ent->fts_accpath = ent->fts_path;
    char *name = strrchr(ent->fts_path, '/');
    ent->fts_name = name ? name + 1 : ent->fts_path;
    ent->fts_namelen = strlen(ent->fts_name);
    ent->fts_level = n->level;
    if (r < 0) {
        ent->fts_info = FTS_NS;
    } else if (S_ISDIR(st.st_mode)) {
        ent->fts_info = FTS_D;
    } else if (S_ISLNK(st.st_mode)) {
        ent->fts_info = FTS_SL;
    } else {
        ent->fts_info = FTS_F;
    }
    if (r == 0)
        ent->fts_stat = st;
    else
        memset(&ent->fts_stat, 0, sizeof(ent->fts_stat));

    if (ent->fts_info == FTS_D) {
        DIR *d = opendir(ent->fts_path);
        if (!d) {
            ent->fts_info = FTS_DNR;
        } else {
            struct dirent *e;
            while ((e = readdir(d))) {
                if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0)
                    continue;
                size_t len = strlen(ent->fts_path);
                size_t add = len && ent->fts_path[len-1] != '/' ? 1 : 0;
                size_t name_len = strlen(e->d_name);
                if (len > SIZE_MAX - add - name_len - 1) {
                    closedir(d);
                    free_entry(ent);
                    free(n);
                    errno = ENAMETOOLONG;
                    return NULL;
                }
                char *child = malloc(len + add + name_len + 1);
                if (!child) {
                    closedir(d);
                    free_entry(ent);
                    free(n);
                    return NULL;
                }
                memcpy(child, ent->fts_path, len);
                if (add)
                    child[len] = '/';
                memcpy(child + len + add, e->d_name, name_len + 1);
                if (queue_push(fts, child, n->level + 1) < 0) {
                    closedir(d);
                    free(child);
                    free_entry(ent);
                    free(n);
                    errno = ENOMEM;
                    return NULL;
                }
                free(child);
            }
            closedir(d);
        }
    }

    free(n);
    fts->cur = ent;
    return ent;
}

/*
 * fts_close() - free all resources used by an FTS handle.
 *
 * Releases any queued paths and the current entry.  Returns 0 on
 * success or -1 if the provided handle is NULL.
 */
int fts_close(FTS *fts)
{
    if (!fts)
        return -1;
    free_entry(fts->cur);
    struct node *n;
    while ((n = queue_pop(fts))) {
        free(n->path);
        free(n);
    }
    free(fts);
    return 0;
}

