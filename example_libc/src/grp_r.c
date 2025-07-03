/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the grp_r functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "grp.h"
#include "io.h"
#include "string.h"
#include "stdlib.h"
#include "env.h"
#include <fcntl.h>
#include <unistd.h>

#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)

/*
 * group_path() - return the pathname of the group database.
 * Uses the VLIBC_GROUP environment variable when set,
 * otherwise falls back to the system default.
 */
static const char *group_path(void)
{
    const char *p = getenv("VLIBC_GROUP");
    if (p && *p)
        return p;
    return "/etc/group";
}

/*
 * parse_line_r() - populate *gr from a single line of the group file.
 * Splits the line and copies fields into the caller-provided buffer.
 * Returns 0 on success or -1 on failure (e.g. buffer too small).
 */
static int parse_line_r(const char *line, struct group *gr,
                        char *buf, size_t buflen)
{
    char lbuf[256];
    strncpy(lbuf, line, sizeof(lbuf) - 1);
    lbuf[sizeof(lbuf) - 1] = '\0';

    char *save;
    char *name = strtok_r(lbuf, ":", &save);
    char *passwd = strtok_r(NULL, ":", &save);
    char *gid_s = strtok_r(NULL, ":", &save);
    char *mem_list = strtok_r(NULL, ":\n", &save);
    if (!name || !passwd || !gid_s || !mem_list)
        return -1;

    /* count members */
    char list_copy[256];
    strncpy(list_copy, mem_list, sizeof(list_copy) - 1);
    list_copy[sizeof(list_copy) - 1] = '\0';
    int count = 0;
    char *save_m;
    for (char *m = strtok_r(list_copy, ",", &save_m); m; m = strtok_r(NULL, ",", &save_m))
        count++;

    size_t need = (count + 1) * sizeof(char *);
    if (need > buflen)
        return -1;
    char **memarr = (char **)buf;
    buf += need;
    buflen -= need;

    size_t len;
    len = strlcpy(buf, name, buflen);
    if (len >= buflen)
        return -1;
    gr->gr_name = buf; buf += len + 1; buflen -= len + 1;

    len = strlcpy(buf, passwd, buflen);
    if (len >= buflen)
        return -1;
    gr->gr_passwd = buf; buf += len + 1; buflen -= len + 1;

    gr->gr_gid = (gid_t)atoi(gid_s);

    strncpy(list_copy, mem_list, sizeof(list_copy) - 1);
    list_copy[sizeof(list_copy) - 1] = '\0';
    int i = 0;
    for (char *m = strtok_r(list_copy, ",", &save_m); m && i < count;
         m = strtok_r(NULL, ",", &save_m)) {
        len = strlcpy(buf, m, buflen);
        if (len >= buflen)
            return -1;
        memarr[i++] = buf;
        buf += len + 1;
        buflen -= len + 1;
    }
    memarr[i] = NULL;
    gr->gr_mem = memarr;
    return 0;
}

/*
 * lookup_r() - helper to search the group database.
 * When by_name is non-zero the search is by group name, otherwise by gid.
 * The parsed entry is placed into *grp using the caller provided buffer.
 */
static int lookup_r(const char *name, gid_t gid, int by_name,
                    struct group *grp, char *buf, size_t buflen,
                    struct group **result)
{
    if (!grp || !buf || !result)
        return -1;
    *result = NULL;

#ifdef O_CLOEXEC
    int flags = O_RDONLY | O_CLOEXEC;
#else
    int flags = O_RDONLY;
#endif
    int fd = open(group_path(), flags, 0);
    if (fd < 0)
        return -1;
    char filebuf[4096];
    ssize_t n = read(fd, filebuf, sizeof(filebuf) - 1);
    close(fd);
    if (n <= 0)
        return -1;
    filebuf[n] = '\0';

    char *save_line;
    for (char *line = strtok_r(filebuf, "\n", &save_line); line;
         line = strtok_r(NULL, "\n", &save_line)) {
        if (parse_line_r(line, grp, buf, buflen) != 0)
            continue;
        if (by_name) {
            if (strcmp(grp->gr_name, name) == 0) {
                *result = grp;
                return 0;
            }
        } else {
            if (grp->gr_gid == gid) {
                *result = grp;
                return 0;
            }
        }
    }
    return -1;
}

/*
 * getgrgid_r() - thread-safe lookup of a group by gid.
 * Uses lookup_r() to parse the entry into the caller supplied structures.
 */
int getgrgid_r(gid_t gid, struct group *grp, char *buf, size_t buflen,
               struct group **result)
{
    return lookup_r(NULL, gid, 0, grp, buf, buflen, result);
}

/*
 * getgrnam_r() - thread-safe lookup of a group by name.
 * Wraps lookup_r() with by_name enabled.
 */
int getgrnam_r(const char *name, struct group *grp, char *buf, size_t buflen,
               struct group **result)
{
    return lookup_r(name, 0, 1, grp, buf, buflen, result);
}

#else

extern int host_getgrgid_r(gid_t, struct group *, char *, size_t,
                           struct group **) __asm("getgrgid_r");
extern int host_getgrnam_r(const char *, struct group *, char *, size_t,
                           struct group **) __asm("getgrnam_r");

/*
 * getgrgid_r() - forward to the host implementation when available.
 */
int getgrgid_r(gid_t gid, struct group *grp, char *buf, size_t buflen,
               struct group **result)
{
    return host_getgrgid_r(gid, grp, buf, buflen, result);
}

/*
 * getgrnam_r() - forward to the host implementation when available.
 */
int getgrnam_r(const char *name, struct group *grp, char *buf, size_t buflen,
               struct group **result)
{
    return host_getgrnam_r(name, grp, buf, buflen, result);
}

#endif
