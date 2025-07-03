/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the fnmatch functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "fnmatch.h"
#include "string.h"

/*
 * match_range() - parse and evaluate a character range starting at `p`.
 * Sets `ep` to the position after the closing bracket and returns
 * whether character `c` matches the range considering `flags`.
 */
static int match_range(const char *p, char c, int flags, const char **ep)
{
    int neg = 0;
    int ok = 0;
    if (*p == '!' || *p == '^') {
        neg = 1;
        p++;
    }
    while (*p && *p != ']') {
        char start;
        if (*p == '\\' && !(flags & FNM_NOESCAPE) && p[1]) {
            start = p[1];
            p += 2;
        } else {
            start = *p++;
        }
        if (*p == '-' && p[1] && p[1] != ']') {
            p++;
            char end;
            if (*p == '\\' && !(flags & FNM_NOESCAPE) && p[1]) {
                end = p[1];
                p += 2;
            } else {
                end = *p++;
            }
            if (start <= c && c <= end)
                ok = 1;
        } else {
            if (start == c)
                ok = 1;
        }
    }
    if (*p == ']')
        p++;
    *ep = p;
    return neg ? !ok : ok;
}

/*
 * do_match() - internal recursive matcher implementing the core
 * fnmatch algorithm. Returns non-zero if string `s` matches pattern
 * `p` according to `flags`.
 */
static int do_match(const char *p, const char *s, int flags)
{
    while (*p) {
        char pc = *p++;
        if (pc == '?') {
            if (!*s)
                return 0;
            s++;
            continue;
        } else if (pc == '*') {
            while (*p == '*')
                p++;
            if (!*p)
                return 1;
            while (*s) {
                if (do_match(p, s, flags))
                    return 1;
                s++;
            }
            return 0;
        } else if (pc == '[') {
            if (!*s)
                return 0;
            const char *end = p;
            if (!match_range(end, *s, flags, &end))
                return 0;
            p = end;
            s++;
            continue;
        } else if (pc == '\\' && !(flags & FNM_NOESCAPE)) {
            pc = *p++;
            if (!pc)
                pc = '\\';
        }
        if (*s != pc)
            return 0;
        s++;
    }
    return *s == '\0';
}

/*
 * fnmatch() - compare `string` to the glob pattern `pattern`.
 * Returns 0 on match or FNM_NOMATCH when the pattern does not
 * match, honoring the provided `flags`.
 */
int fnmatch(const char *pattern, const char *string, int flags)
{
    return do_match(pattern, string, flags) ? 0 : FNM_NOMATCH;
}

