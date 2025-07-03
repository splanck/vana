/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the ftw functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "ftw.h"
#include "dirent.h"
#include "string.h"
#include "stdlib.h"
#include "memory.h"
#include "errno.h"
#include "unistd.h"
#include "sys/stat.h"

static int do_nftw(const char *path,
                   nftw_func_t fn,
                   int fdlimit, int flags, int level)
{
    struct stat st;
    int r;

    if (flags & FTW_PHYS)
        r = lstat(path, &st);
    else
        r = stat(path, &st);

    int type;
    if (r < 0) {
        type = FTW_NS;
    } else if (S_ISDIR(st.st_mode)) {
        type = FTW_D;
    } else if (S_ISLNK(st.st_mode)) {
        if (flags & FTW_PHYS) {
            struct stat sb;
            if (stat(path, &sb) < 0)
                type = FTW_SLN;
            else
                type = FTW_SL;
        } else {
            type = FTW_SL;
        }
    } else {
        type = FTW_F;
    }

    struct FTW info;
    const char *base = strrchr(path, '/');
    info.base = base ? (int)(base + 1 - path) : 0;
    info.level = level;

    if (!(flags & FTW_DEPTH) || type != FTW_D || level == 0) {
        r = fn(path, &st, type, &info);
        if (r != 0)
            return r;
    }

    if (type == FTW_D) {
        DIR *d = opendir(path);
        if (!d)
            return fn(path, &st, FTW_DNR, &info);
        struct dirent *e;
        while ((e = readdir(d))) {
            if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0)
                continue;
            size_t len = strlen(path);
            size_t add = len && path[len-1] != '/' ? 1 : 0;
            size_t name_len = strlen(e->d_name);
            if (len > SIZE_MAX - add - name_len - 1) {
                closedir(d);
                errno = ENAMETOOLONG;
                return -1;
            }
            char *child = malloc(len + add + name_len + 1);
            if (!child) {
                closedir(d);
                errno = ENOMEM;
                return -1;
            }
            memcpy(child, path, len);
            if (add)
                child[len] = '/';
            memcpy(child + len + add, e->d_name, name_len + 1);
            r = do_nftw(child, fn, fdlimit, flags, level + 1);
            free(child);
            if (r != 0) {
                closedir(d);
                return r;
            }
        }
        closedir(d);
    }

    if ((flags & FTW_DEPTH) && type == FTW_D) {
        r = fn(path, &st, FTW_DP, &info);
        if (r != 0)
            return r;
    }
    return 0;
}

static ftw_func_t ftw_cb;

/*
 * ftw_wrapper() - adapter used by ftw() so that nftw() can call an
 * ftw_func_t callback.  The FTW structure passed by nftw() is ignored
 * and the saved callback is invoked with the original arguments.
 */
static int ftw_wrapper(const char *fpath, const struct stat *sb, int typeflag,
                       struct FTW *info)
{
    (void)info;
    return ftw_cb(fpath, sb, typeflag);
}

/*
 * nftw() - walk a directory tree calling fn for each encountered file.
 * The fdlimit argument is ignored in this implementation.  Traversal is
 * performed by the internal do_nftw() helper which handles FTW_* flags.
 */
int nftw(const char *path, nftw_func_t fn, int fdlimit, int flags)
{
    if (!path || !fn) {
        errno = EINVAL;
        return -1;
    }
    (void)fdlimit;
    return do_nftw(path, fn, fdlimit, flags, 0);
}

/*
 * ftw() - legacy wrapper around nftw().  The provided callback is stored
 * in a global variable and invoked via ftw_wrapper() so the older
 * ftw_func_t signature can be used with nftw's traversal logic.
 */
int ftw(const char *path, ftw_func_t fn, int fdlimit)
{
    ftw_cb = fn;
    return nftw(path, ftw_wrapper, fdlimit, FTW_PHYS | FTW_DEPTH);
}

