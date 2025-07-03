/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the getlogin functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "unistd.h"
#include "pwd.h"
#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include "errno.h"

/*
 * Obtain the login name for the current user by calling
 * getpwuid(getuid()). The resulting pw_name field is copied into a
 * thread-local buffer so repeated calls avoid further lookups.
 */
static struct passwd *lookup_self(void)
{
    uid_t uid = getuid();
    FILE *f = fopen("/etc/passwd", "r");
    if (!f)
        return NULL;

    static __thread struct passwd pw;
    static __thread char *linebuf;
    static __thread size_t cap;
    char *line = NULL;
    size_t lcap = 0;
    struct passwd *ret = NULL;

    while (getline(&line, &lcap, f) != -1) {
        size_t len = strlen(line);
        if (len && line[len - 1] == '\n')
            line[len - 1] = '\0';
        if (len + 1 > cap) {
            char *tmp = realloc(linebuf, len + 1);
            if (!tmp) {
                free(line);
                fclose(f);
                return NULL;
            }
            linebuf = tmp;
            cap = len + 1;
        }
        memcpy(linebuf, line, len + 1);

        char *save;
        pw.pw_name = strtok_r(linebuf, ":", &save);
        pw.pw_passwd = strtok_r(NULL, ":", &save);
        char *uid_s = strtok_r(NULL, ":", &save);
        char *gid_s = strtok_r(NULL, ":", &save);
        pw.pw_gecos = strtok_r(NULL, ":", &save);
        pw.pw_dir = strtok_r(NULL, ":", &save);
        pw.pw_shell = strtok_r(NULL, ":\n", &save);
        if (!pw.pw_name || !pw.pw_passwd || !uid_s || !gid_s ||
            !pw.pw_gecos || !pw.pw_dir || !pw.pw_shell)
            continue;
        pw.pw_uid = (uid_t)atoi(uid_s);
        pw.pw_gid = (gid_t)atoi(gid_s);
        if (pw.pw_uid == uid) {
            ret = &pw;
            break;
        }
    }
    free(line);
    fclose(f);
    return ret;
}

static int getlogin_impl(char *buf, size_t len)
{
    if (!buf || len == 0)
        return EINVAL;

    struct passwd *pw = lookup_self();
    if (!pw || !pw->pw_name)
        return ENOENT;

    size_t n = strlcpy(buf, pw->pw_name, len);
    if (n >= len)
        return ERANGE;
    return 0;
}

int getlogin_r(char *buf, size_t len)
{
    return getlogin_impl(buf, len);
}

char *getlogin(void)
{
    static __thread char name[64];
    if (name[0])
        return name;

    if (getlogin_r(name, sizeof(name)) != 0)
        return NULL;
    return name;
}
