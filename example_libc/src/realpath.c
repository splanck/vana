/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the realpath functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "stdlib.h"
#include "string.h"
#include "memory.h"
#include "errno.h"
#include <limits.h>

char *realpath(const char *path, char *resolved_path)
{
    if (!path) {
        errno = EINVAL;
        return NULL;
    }

    char *cwd = NULL;
    char *full = NULL;
    if (path[0] != '/') {
        size_t cap = 256;
#ifdef PATH_MAX
        cap = PATH_MAX;
#endif
        cwd = malloc(cap);
        if (!cwd)
            return NULL;
        for (;;) {
            if (getcwd(cwd, cap))
                break;
            if (errno != ERANGE) {
                free(cwd);
                return NULL;
            }
            size_t new_cap = cap * 2;
            char *tmp = realloc(cwd, new_cap);
            if (!tmp) {
                free(cwd);
                errno = ENOMEM;
                return NULL;
            }
            cwd = tmp;
            cap = new_cap;
        }

        size_t cwd_len = strlen(cwd);
        size_t add_slash = (cwd_len > 1 && cwd[cwd_len-1] != '/') ? 1 : 0;
        size_t len = cwd_len + add_slash + strlen(path) + 1;
        full = malloc(len + 1);
        if (!full) {
            free(cwd);
            return NULL;
        }
        strcpy(full, cwd);
        if (add_slash)
            strcat(full, "/");
        strcat(full, path);
        free(cwd);
    } else {
        full = strdup(path);
        if (!full)
            return NULL;
    }

    size_t out_cap = strlen(full) + 2;
    char *outbuf = resolved_path ? resolved_path : malloc(out_cap);
    if (!outbuf) {
        free(full);
        return NULL;
    }

    size_t out_len = 1;
    outbuf[0] = '/';
    outbuf[1] = '\0';

    char *p = full;
    if (*p == '/')
        ++p;
    while (*p) {
        while (*p == '/')
            ++p;
        if (!*p)
            break;
        char *start = p;
        while (*p && *p != '/')
            ++p;
        size_t seg_len = p - start;
        if (seg_len == 1 && start[0] == '.') {
            /* ignore */
        } else if (seg_len == 2 && start[0] == '.' && start[1] == '.') {
            if (out_len > 1) {
                while (out_len > 1 && outbuf[out_len-1] != '/')
                    --out_len;
                if (out_len > 1)
                    --out_len; /* drop trailing '/' */
                outbuf[out_len] = '\0';
            }
        } else {
            if (out_len > 1)
                outbuf[out_len++] = '/';
            memcpy(outbuf + out_len, start, seg_len);
            out_len += seg_len;
            outbuf[out_len] = '\0';
        }
    }

    if (out_len > 1 && outbuf[out_len-1] == '/') {
        outbuf[out_len-1] = '\0';
    }

    free(full);
    return outbuf;
}

