/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the pwd functions for vlibc. Provides wrappers and helpers used by the standard library.
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
#include <stdio.h>

#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)

static const char *passwd_path(void)
{
    const char *p = getenv("VLIBC_PASSWD");
    if (p && *p)
        return p;
    return "/etc/passwd";
}

static struct passwd pw;
static char *linebuf;
static size_t linecap;

static struct passwd *parse_line(char *line)
{
    size_t len = strlen(line);
    if (len && line[len - 1] == '\n')
        line[len - 1] = '\0';
    if (len + 1 > linecap) {
        char *tmp = realloc(linebuf, len + 1);
        if (!tmp)
            return NULL;
        linebuf = tmp;
        linecap = len + 1;
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
        return NULL;
    pw.pw_uid = (uid_t)atoi(uid_s);
    pw.pw_gid = (gid_t)atoi(gid_s);
    return &pw;
}

static struct passwd *lookup(const char *name, uid_t uid, int by_name)
{
    FILE *f = fopen(passwd_path(), "r");
    if (!f)
        return NULL;

    struct passwd *ret = NULL;
    char *line = NULL;
    size_t cap = 0;
    while (getline(&line, &cap, f) != -1) {
        struct passwd *p = parse_line(line);
        if (!p)
            continue;
        if (by_name) {
            if (strcmp(p->pw_name, name) == 0) {
                ret = p;
                break;
            }
        } else {
            if (p->pw_uid == uid) {
                ret = p;
                break;
            }
        }
    }
    free(line);
    fclose(f);
    return ret;
}

struct passwd *getpwuid(uid_t uid)
{
    return lookup(NULL, uid, 0);
}

struct passwd *getpwnam(const char *name)
{
    return lookup(name, 0, 1);
}

extern void host_setpwent(void) __asm("setpwent");
extern struct passwd *host_getpwent(void) __asm("getpwent");
extern void host_endpwent(void) __asm("endpwent");

void setpwent(void)
{
    host_setpwent();
}

struct passwd *getpwent(void)
{
    return host_getpwent();
}

void endpwent(void)
{
    host_endpwent();
}

#else

extern struct passwd *host_getpwuid(uid_t) __asm("getpwuid");
extern struct passwd *host_getpwnam(const char *) __asm("getpwnam");

struct passwd *getpwuid(uid_t uid)
{
    return host_getpwuid(uid);
}

struct passwd *getpwnam(const char *name)
{
    return host_getpwnam(name);
}

static const char *passwd_path(void)
{
    const char *p = getenv("VLIBC_PASSWD");
    if (p && *p)
        return p;
    return "/etc/passwd";
}

static struct passwd pw;
static char *linebuf;
static size_t linecap;
static FILE *pw_file;

static struct passwd *parse_line(char *line)
{
    size_t len = strlen(line);
    if (len && line[len - 1] == '\n')
        line[len - 1] = '\0';
    if (len + 1 > linecap) {
        char *tmp = realloc(linebuf, len + 1);
        if (!tmp)
            return NULL;
        linebuf = tmp;
        linecap = len + 1;
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
        return NULL;
    pw.pw_uid = (uid_t)atoi(uid_s);
    pw.pw_gid = (gid_t)atoi(gid_s);
    return &pw;
}

void setpwent(void)
{
    if (pw_file) {
        rewind(pw_file);
        return;
    }

    pw_file = fopen(passwd_path(), "r");
}

struct passwd *getpwent(void)
{
    if (!pw_file)
        setpwent();
    if (!pw_file)
        return NULL;

    ssize_t len = getline(&linebuf, &linecap, pw_file);
    if (len == -1)
        return NULL;
    return parse_line(linebuf);
}

void endpwent(void)
{
    if (pw_file) {
        fclose(pw_file);
        pw_file = NULL;
    }
}

#endif
