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
 * Purpose: Implements the grp functions for vlibc.
 */

#include "grp.h"
#include "io.h"
#include "string.h"
#include "stdlib.h"
#include "memory.h"
#include "env.h"
#include "pthread.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include "errno.h"
#include <sys/syscall.h>
#include "syscall.h"

static pthread_mutex_t gr_lock = { ATOMIC_FLAG_INIT, PTHREAD_MUTEX_RECURSIVE, 0, 0 };

#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)

/* group_path() - return location of the group file */
static const char *group_path(void)
{
    const char *p = getenv("VLIBC_GROUP");
    if (p && *p)
        return p;
    return "/etc/group";
}

static __thread struct group gr;
static __thread char *members[64];
static __thread char linebuf[256];
static __thread FILE *gr_file;
static __thread char *file_line;
static __thread size_t file_cap;

/* parse_line() - parse an entry from the group file */
static struct group *parse_line(const char *line)
{
    strncpy(linebuf, line, sizeof(linebuf) - 1);
    linebuf[sizeof(linebuf) - 1] = '\0';

    char *save;
    gr.gr_name = strtok_r(linebuf, ":", &save);
    gr.gr_passwd = strtok_r(NULL, ":", &save);
    char *gid_s = strtok_r(NULL, ":", &save);
    char *mem_list = strtok_r(NULL, ":\n", &save);
    if (!gr.gr_name || !gr.gr_passwd || !gid_s)
        return NULL;
    gr.gr_gid = (gid_t)atoi(gid_s);

    char *save_mem;
    int i = 0;
    if (mem_list && *mem_list) {
        for (char *m = strtok_r(mem_list, ",", &save_mem); m &&
             i < (int)(sizeof(members)/sizeof(members[0]) - 1);
             m = strtok_r(NULL, ",", &save_mem)) {
            members[i++] = m;
        }
    }
    members[i] = NULL;
    gr.gr_mem = members;
    return &gr;
}

/* lookup() - search the group file by name or gid */
static struct group *lookup(const char *name, gid_t gid, int by_name)
{
#ifdef O_CLOEXEC
    int flags = O_RDONLY | O_CLOEXEC;
#else
    int flags = O_RDONLY;
#endif
    int fd = open(group_path(), flags, 0);
    if (fd < 0)
        return NULL;
    char buf[4096];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0)
        return NULL;
    buf[n] = '\0';

    char *save_line;
    for (char *line = strtok_r(buf, "\n", &save_line); line;
         line = strtok_r(NULL, "\n", &save_line)) {
        struct group *g = parse_line(line);
        if (!g)
            continue;
        if (by_name) {
            if (strcmp(g->gr_name, name) == 0)
                return g;
        } else {
            if (g->gr_gid == gid)
                return g;
        }
    }
    return NULL;
}

/* getgrgid() - retrieve group entry by gid */
struct group *getgrgid(gid_t gid)
{
    return lookup(NULL, gid, 0);
}

/* getgrnam() - retrieve group entry by name */
struct group *getgrnam(const char *name)
{
    return lookup(name, 0, 1);
}

extern void host_setgrent(void) __asm("setgrent");
extern struct group *host_getgrent(void) __asm("getgrent");
extern void host_endgrent(void) __asm("endgrent");

/* setgrent() - reset group iteration */
void setgrent(void)
{
    pthread_mutex_lock(&gr_lock);
    const char *path = getenv("VLIBC_GROUP");
    if (path && *path) {
        if (gr_file) {
            rewind(gr_file);
        } else {
#ifdef O_CLOEXEC
            int flags = O_RDONLY | O_CLOEXEC;
#else
            int flags = O_RDONLY;
#endif
            int fd = open(path, flags, 0);
            if (fd >= 0) {
                gr_file = fdopen(fd, "r");
                if (!gr_file)
                    close(fd);
            }
        }
        pthread_mutex_unlock(&gr_lock);
        return;
    }
    host_setgrent();
    pthread_mutex_unlock(&gr_lock);
}

/* getgrent() - read next group entry */
struct group *getgrent(void)
{
    pthread_mutex_lock(&gr_lock);
    const char *path = getenv("VLIBC_GROUP");
    if (path && *path) {
        if (!gr_file)
            setgrent();
        if (!gr_file) {
            pthread_mutex_unlock(&gr_lock);
            return NULL;
        }
        ssize_t len = getline(&file_line, &file_cap, gr_file);
        if (len == -1) {
            pthread_mutex_unlock(&gr_lock);
            return NULL;
        }
        struct group *g = parse_line(file_line);
        pthread_mutex_unlock(&gr_lock);
        return g;
    }
    struct group *g = host_getgrent();
    pthread_mutex_unlock(&gr_lock);
    return g;
}

/* endgrent() - finish group iteration */
void endgrent(void)
{
    pthread_mutex_lock(&gr_lock);
    const char *path = getenv("VLIBC_GROUP");
    if (path && *path) {
        if (gr_file) {
            fclose(gr_file);
            gr_file = NULL;
        }
        pthread_mutex_unlock(&gr_lock);
        return;
    }
    host_endgrent();
    pthread_mutex_unlock(&gr_lock);
}

#else

extern struct group *host_getgrgid(gid_t) __asm("getgrgid");
extern struct group *host_getgrnam(const char *) __asm("getgrnam");

struct group *getgrgid(gid_t gid)
{
    return host_getgrgid(gid);
}

struct group *getgrnam(const char *name)
{
    return host_getgrnam(name);
}

/* group_path() - return location of the group file */
static const char *group_path(void)
{
    const char *p = getenv("VLIBC_GROUP");
    if (p && *p)
        return p;
    return "/etc/group";
}

static __thread struct group gr;
static __thread char *members[64];
static __thread char linebuf[256];
static __thread char filebuf[4096];
static __thread char *next_line;
static __thread int gr_initialized;

/* parse_line() - parse an entry from the group file */
static struct group *parse_line(const char *line)
{
    strncpy(linebuf, line, sizeof(linebuf) - 1);
    linebuf[sizeof(linebuf) - 1] = '\0';

    char *save;
    gr.gr_name = strtok_r(linebuf, ":", &save);
    gr.gr_passwd = strtok_r(NULL, ":", &save);
    char *gid_s = strtok_r(NULL, ":", &save);
    char *mem_list = strtok_r(NULL, ":\n", &save);
    if (!gr.gr_name || !gr.gr_passwd || !gid_s || !mem_list)
        return NULL;
    gr.gr_gid = (gid_t)atoi(gid_s);

    char *save_mem;
    int i = 0;
    for (char *m = strtok_r(mem_list, ",", &save_mem); m &&
         i < (int)(sizeof(members)/sizeof(members[0]) - 1);
         m = strtok_r(NULL, ",", &save_mem)) {
        members[i++] = m;
    }
    members[i] = NULL;
    gr.gr_mem = members;
    return &gr;
}

/* setgrent() - open group file for iteration */
void setgrent(void)
{
    pthread_mutex_lock(&gr_lock);
#ifdef O_CLOEXEC
    int flags = O_RDONLY | O_CLOEXEC;
#else
    int flags = O_RDONLY;
#endif
    int fd = open(group_path(), flags, 0);
    gr_initialized = 1;
    if (fd < 0) {
        next_line = NULL;
        pthread_mutex_unlock(&gr_lock);
        return;
    }
    ssize_t n = read(fd, filebuf, sizeof(filebuf) - 1);
    close(fd);
    if (n <= 0) {
        next_line = NULL;
        pthread_mutex_unlock(&gr_lock);
        return;
    }
    filebuf[n] = '\0';
    next_line = filebuf;
    pthread_mutex_unlock(&gr_lock);
}

/* getgrent() - read next group from file */
struct group *getgrent(void)
{
    pthread_mutex_lock(&gr_lock);
    if (!gr_initialized)
        setgrent();
    if (!next_line) {
        pthread_mutex_unlock(&gr_lock);
        return NULL;
    }
    char *line = strtok_r(next_line, "\n", &next_line);
    if (!line) {
        pthread_mutex_unlock(&gr_lock);
        return NULL;
    }
    struct group *g = parse_line(line);
    pthread_mutex_unlock(&gr_lock);
    return g;
}

/* endgrent() - close group file */
void endgrent(void)
{
    pthread_mutex_lock(&gr_lock);
    next_line = NULL;
    gr_initialized = 0;
    pthread_mutex_unlock(&gr_lock);
}

#endif

#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)

extern int host_getgrouplist(const char *, gid_t, gid_t *, int *)
    __asm("getgrouplist");
extern int host_initgroups(const char *, gid_t) __asm("initgroups");

/* getgrouplist() - obtain supplementary group IDs */
int getgrouplist(const char *user, gid_t group, gid_t *groups, int *ngroups)
{
    return host_getgrouplist(user, group, groups, ngroups);
}

/* initgroups() - initialize group access list */
int initgroups(const char *user, gid_t group)
{
    return host_initgroups(user, group);
}

#else

/* do_setgroups() - helper to apply group list */
static int do_setgroups(int n, const gid_t *groups)
{
#ifdef SYS_setgroups
    long ret = vlibc_syscall(SYS_setgroups, n, (long)groups, 0, 0, 0, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
#else
    extern int host_setgroups(int, const gid_t *) __asm("setgroups");
    return host_setgroups(n, groups);
#endif
}
/* getgrouplist() - parse group file for user */

int getgrouplist(const char *user, gid_t group, gid_t *groups, int *ngroups)
{
    if (!user || !groups || !ngroups || *ngroups <= 0)
        return -1;

    int limit = *ngroups;
    int count = 0;
    groups[count++] = group;

#ifdef O_CLOEXEC
    int flags = O_RDONLY | O_CLOEXEC;
#else
    int flags = O_RDONLY;
#endif
    int fd = open(group_path(), flags, 0);
    if (fd < 0)
        return -1;
    char buf[4096];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0)
        return -1;
    buf[n] = '\0';

    char *save_line;
    for (char *line = strtok_r(buf, "\n", &save_line); line;
         line = strtok_r(NULL, "\n", &save_line)) {
        struct group *g = parse_line(line);
        if (!g)
            continue;
        for (char **m = g->gr_mem; m && *m; m++) {
            if (strcmp(*m, user) == 0) {
                int dup = 0;
                for (int i = 0; i < count; i++) {
                    if (groups[i] == g->gr_gid) {
                        dup = 1;
                        break;
                    }
                }
                if (!dup) {
                    if (count < limit)
                        groups[count] = g->gr_gid;
                    count++;
                }
                break;
            }
        }
    }

    if (count > limit) {
        *ngroups = count;
        return -1;
    }
    *ngroups = count;
    return count;
}
/* initgroups() - apply groups parsed from file */

int initgroups(const char *user, gid_t group)
{
    gid_t stack[32];
    gid_t *list = stack;
    int ng = (int)(sizeof(stack) / sizeof(stack[0]));
    int r = getgrouplist(user, group, list, &ng);
    if (r == -1) {
        list = malloc((size_t)ng * sizeof(gid_t));
        if (!list)
            return -1;
        if (getgrouplist(user, group, list, &ng) == -1) {
            free(list);
            return -1;
        }
    }
    int ret = do_setgroups(ng, list);
    if (list != stack)
        free(list);
    return ret;
}

#endif
