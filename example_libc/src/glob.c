/*
 * BSD 2-Clause "Simplified" License
 *
 * Copyright (c) 2025, vlibc authors
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Purpose: Implements the glob functions for vlibc.
 */

#include "glob.h"
#include "string.h"
#include "memory.h"
#include "stdlib.h"
#include "dirent.h"
#include "fnmatch.h"
#include <errno.h>

/*
 * join_path() - allocate a new string combining BASE and NAME.
 *
 * A '/' separator is inserted when needed.  The caller must free the
 * returned string or NULL will be returned on allocation failure.
 */
static char *join_path(const char *base, const char *name)
{
    size_t blen = base ? strlen(base) : 0;
    size_t add = (blen && base[blen-1] != '/') ? 1 : 0;
    size_t nlen = strlen(name);
    char *res = malloc(blen + add + nlen + 1);
    if (!res)
        return NULL;
    if (blen) {
        memcpy(res, base, blen);
        if (add)
            res[blen] = '/';
    }
    size_t pos = blen + add;
    memcpy(res + pos, name, nlen);
    res[pos + nlen] = '\0';
    return res;
}

/*
 * add_match() - append PATH to the result list in G.
 *
 * The glob_t array is expanded and the string duplicated.  Returns 0
 * on success or -1 when memory allocation fails.
 */
static int add_match(glob_t *g, const char *path)
{
    char **tmp = realloc(g->gl_pathv, sizeof(char*) * (g->gl_pathc + 2));
    if (!tmp)
        return -1;
    g->gl_pathv = tmp;
    g->gl_pathv[g->gl_pathc] = strdup(path);
    if (!g->gl_pathv[g->gl_pathc])
        return -1;
    g->gl_pathc++;
    g->gl_pathv[g->gl_pathc] = NULL;
    return 0;
}

/* cmp_str() - helper for sorting path strings */
static int cmp_str(const void *a, const void *b)
{
    const char *sa = *(const char *const*)a;
    const char *sb = *(const char *const*)b;
    return strcmp(sa, sb);
}

static int expand(const char *base, const char *pattern, glob_t *g, int flags,
                  int (*errfunc)(const char *, int));

/* handle_dir() - process directory entries matching pattern */
static int handle_dir(const char *dir, const char *pattern, const char *rest,
                      glob_t *g, int flags,
                      int (*errfunc)(const char *, int))
{
    DIR *d = opendir(*dir ? dir : ".");
    if (!d) {
        if (errfunc)
            errfunc(dir, errno);
        return 0;
    }
    struct dirent *ent;
    int ret = 0;
    int fnm_flags = (flags & GLOB_NOESCAPE) ? FNM_NOESCAPE : 0;
    while ((ent = readdir(d))) {
        const char *name = ent->d_name;
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
            continue;
        if (fnmatch(pattern, name, fnm_flags) == 0) {
            char *next = join_path(dir, name);
            if (!next) { ret = GLOB_NOSPACE; break; }
            if (rest) {
                ret = expand(next, rest, g, flags, errfunc);
            } else {
                if (add_match(g, next) < 0)
                    ret = GLOB_NOSPACE;
            }
            free(next);
            if (ret)
                break;
        }
    }
    closedir(d);
    return ret;
}

/*
 * expand() - recursively expand PATTERN relative to BASE.
 *
 * Each path segment is processed in turn and directories are scanned
 * to find matching entries.  Matches are accumulated into the glob_t
 * structure via add_match().  Returns 0 on success or a GLOB_* error
 * code.
 */
static int expand(const char *base, const char *pattern, glob_t *g, int flags,
                  int (*errfunc)(const char *, int))
{
    while (*pattern == '/')
        pattern++;
    const char *slash = strchr(pattern, '/');
    if (!slash)
        return handle_dir(base, pattern, NULL, g, flags, errfunc);

    size_t len = (size_t)(slash - pattern);
    char part[len + 1];
    memcpy(part, pattern, len);
    part[len] = '\0';
    const char *rest = slash[1] ? slash + 1 : NULL;

    return handle_dir(base, part, rest, g, flags, errfunc);
}

/*
 * glob() - perform pathname expansion on PATTERN.
 *
 * The interface matches the POSIX glob() function.  Results are stored
 * in PGLOB with behaviour controlled by FLAGS and ERRFUNC.  The helper
 * expand() does the recursive directory traversal.  On success the
 * function returns 0.  Otherwise a GLOB_* error code is returned.
 */
int glob(const char *pattern, int flags,
         int (*errfunc)(const char *epath, int eerrno), glob_t *pglob)
{
    if (!pglob)
        return GLOB_ABORTED;
    pglob->gl_pathc = 0;
    pglob->gl_pathv = NULL;
    pglob->gl_offs = 0;

    if (!pattern)
        return GLOB_ABORTED;

    int ret;
    if (pattern[0] == '/')
        ret = expand("/", pattern + 1, pglob, flags, errfunc);
    else
        ret = expand("", pattern, pglob, flags, errfunc);

    if (ret == 0 && pglob->gl_pathc == 0)
        ret = GLOB_NOMATCH;
    if (ret == 0 && !(flags & GLOB_NOSORT))
        qsort(pglob->gl_pathv, pglob->gl_pathc, sizeof(char*), cmp_str);
    return ret;
}

/* globfree() - release memory used by glob results */
void globfree(glob_t *pglob)
{
    if (!pglob)
        return;
    for (size_t i = 0; i < pglob->gl_pathc; i++)
        free(pglob->gl_pathv[i]);
    free(pglob->gl_pathv);
    pglob->gl_pathv = NULL;
    pglob->gl_pathc = 0;
    pglob->gl_offs = 0;
}

