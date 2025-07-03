/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the pwd_r functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "pwd.h"
#include "io.h"
#include "string.h"
#include "stdlib.h"
#include "env.h"
#include <fcntl.h>
#include <unistd.h>

#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)

static const char *passwd_path(void)
{
    const char *p = getenv("VLIBC_PASSWD");
    if (p && *p)
        return p;
    return "/etc/passwd";
}

static int parse_line_r(const char *line, struct passwd *pw,
                        char *buf, size_t buflen)
{
    char lbuf[256];
    strncpy(lbuf, line, sizeof(lbuf) - 1);
    lbuf[sizeof(lbuf) - 1] = '\0';

    char *save;
    char *name = strtok_r(lbuf, ":", &save);
    char *passwd = strtok_r(NULL, ":", &save);
    char *uid_s = strtok_r(NULL, ":", &save);
    char *gid_s = strtok_r(NULL, ":", &save);
    char *gecos = strtok_r(NULL, ":", &save);
    char *dir = strtok_r(NULL, ":", &save);
    char *shell = strtok_r(NULL, ":\n", &save);
    if (!name || !passwd || !uid_s || !gid_s || !gecos || !dir || !shell)
        return -1;

    size_t len;

    len = strlcpy(buf, name, buflen);
    if (len >= buflen)
        return -1;
    pw->pw_name = buf; buf += len + 1; buflen -= len + 1;

    len = strlcpy(buf, passwd, buflen);
    if (len >= buflen)
        return -1;
    pw->pw_passwd = buf; buf += len + 1; buflen -= len + 1;

    len = strlcpy(buf, gecos, buflen);
    if (len >= buflen)
        return -1;
    pw->pw_gecos = buf; buf += len + 1; buflen -= len + 1;

    len = strlcpy(buf, dir, buflen);
    if (len >= buflen)
        return -1;
    pw->pw_dir = buf; buf += len + 1; buflen -= len + 1;

    len = strlcpy(buf, shell, buflen);
    if (len >= buflen)
        return -1;
    pw->pw_shell = buf; buf += len + 1; buflen -= len + 1;

    pw->pw_uid = (uid_t)atoi(uid_s);
    pw->pw_gid = (gid_t)atoi(gid_s);
    return 0;
}

static int lookup_r(const char *name, uid_t uid, int by_name,
                    struct passwd *pwd, char *buf, size_t buflen,
                    struct passwd **result)
{
    if (!pwd || !buf || !result)
        return -1;
    *result = NULL;

#ifdef O_CLOEXEC
    int flags = O_RDONLY | O_CLOEXEC;
#else
    int flags = O_RDONLY;
#endif
    int fd = open(passwd_path(), flags, 0);
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
        if (parse_line_r(line, pwd, buf, buflen) != 0)
            continue;
        if (by_name) {
            if (strcmp(pwd->pw_name, name) == 0) {
                *result = pwd;
                return 0;
            }
        } else {
            if (pwd->pw_uid == uid) {
                *result = pwd;
                return 0;
            }
        }
    }
    return -1;
}

int getpwuid_r(uid_t uid, struct passwd *pwd, char *buf, size_t buflen,
               struct passwd **result)
{
    return lookup_r(NULL, uid, 0, pwd, buf, buflen, result);
}

int getpwnam_r(const char *name, struct passwd *pwd, char *buf, size_t buflen,
               struct passwd **result)
{
    return lookup_r(name, 0, 1, pwd, buf, buflen, result);
}

#else

extern int host_getpwuid_r(uid_t, struct passwd *, char *, size_t,
                           struct passwd **) __asm("getpwuid_r");
extern int host_getpwnam_r(const char *, struct passwd *, char *, size_t,
                           struct passwd **) __asm("getpwnam_r");

int getpwuid_r(uid_t uid, struct passwd *pwd, char *buf, size_t buflen,
               struct passwd **result)
{
    return host_getpwuid_r(uid, pwd, buf, buflen, result);
}

int getpwnam_r(const char *name, struct passwd *pwd, char *buf, size_t buflen,
               struct passwd **result)
{
    return host_getpwnam_r(name, pwd, buf, buflen, result);
}

#endif
