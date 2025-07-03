/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the env functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "env.h"
#include "memory.h"
#include "string.h"
#include <errno.h>

/* pointer to current environment */
char **environ = 0;
/* whether environ array was allocated by vlibc */
static int environ_owned = 0;
/* per-entry ownership flags when environ_owned */
static unsigned char *environ_flags = 0;

/*
 * env_init() - set the global environ pointer used by vlibc.
 * Programs with a custom entry point should invoke this before
 * using environment functions.
 */
void env_init(char **envp)
{
    environ = envp;
    environ_owned = 0;
    environ_flags = 0;
}

/*
 * getenv() - return the value associated with NAME or NULL
 * if the variable is not present.
 */
char *getenv(const char *name)
{
    if (!environ || !name)
        return NULL;
    size_t len = strlen(name);
    for (char **e = environ; *e; ++e) {
        if (strncmp(*e, name, len) == 0 && (*e)[len] == '=')
            return *e + len + 1;
    }
    return NULL;
}

/*
 * find_env_index() - helper used to locate NAME in environ.
 * Returns its index or -1 when not found.
 */
static int find_env_index(const char *name, size_t len)
{
    if (!environ)
        return -1;
    for (int i = 0; environ[i]; ++i) {
        if (strncmp(environ[i], name, len) == 0 && environ[i][len] == '=')
            return i;
    }
    return -1;
}

/*
 * setenv() - add or update NAME with VALUE. If OVERWRITE is
 * zero, an existing variable is left unchanged.
 */
int setenv(const char *name, const char *value, int overwrite)
{
    if (!name || strchr(name, '=')) {
        errno = EINVAL;
        return -1;
    }
    if (!value)
        value = "";
    size_t nlen = strlen(name);
    size_t vlen = strlen(value);

    int idx = find_env_index(name, nlen);
    if (idx >= 0 && !overwrite)
        return 0;

    char *entry = malloc(nlen + vlen + 2); /* name=value\0 */
    if (!entry)
        return -1;
    vmemcpy(entry, name, nlen);
    entry[nlen] = '=';
    vmemcpy(entry + nlen + 1, value, vlen);
    entry[nlen + 1 + vlen] = '\0';

    if (idx >= 0) {
        if (environ_owned)
            free(environ[idx]);
        environ[idx] = entry;
        return 0;
    }

    /* count items */
    int count = 0;
    if (environ) {
        while (environ[count])
            count++;
    }

    char **oldenv = environ;
    unsigned char *oldflags = environ_flags;
    char **newenv = malloc(sizeof(char *) * (count + 2));
    unsigned char *newflags = malloc(sizeof(unsigned char) * (count + 2));
    if (!newenv || !newflags) {
        free(newenv);
        free(newflags);
        free(entry);
        errno = ENOMEM;
        return -1;
    }
    for (int i = 0; i < count; ++i) {
        if (environ_owned && oldflags && oldflags[i]) {
            newenv[i] = oldenv[i];
            newflags[i] = 1;
        } else {
            newenv[i] = strdup(oldenv[i]);
            if (!newenv[i]) {
                for (int j = 0; j < i; ++j)
                    if (newflags[j])
                        free(newenv[j]);
                free(newenv);
                free(newflags);
                free(entry);
                errno = ENOMEM;
                return -1;
            }
            newflags[i] = 1;
        }
    }
    newenv[count] = entry;
    newflags[count] = 1;
    newenv[count + 1] = NULL;
    if (environ_owned) {
        free(environ);
        free(environ_flags);
    }
    environ_owned = 1;
    environ = newenv;
    environ_flags = newflags;
    return 0;
}

/*
 * putenv() - insert the string NAME=VALUE directly into the
 * environment without copying. The provided string must remain
 * valid for the lifetime of the variable.
 */
int putenv(const char *str)
{
    if (!str) {
        errno = EINVAL;
        return -1;
    }
    const char *eq = strchr(str, '=');
    if (!eq || eq == str) {
        errno = EINVAL;
        return -1;
    }

    size_t nlen = (size_t)(eq - str);

    int idx = find_env_index(str, nlen);
    if (idx >= 0) {
        environ[idx] = (char *)str;
        return 0;
    }

    int count = 0;
    if (environ) {
        while (environ[count])
            count++;
    }

    char **oldenv = environ;
    unsigned char *oldflags = environ_flags;
    char **newenv = malloc(sizeof(char *) * (count + 2));
    unsigned char *newflags = malloc(sizeof(unsigned char) * (count + 2));
    if (!newenv || !newflags) {
        free(newenv);
        free(newflags);
        errno = ENOMEM;
        return -1;
    }
    for (int i = 0; i < count; ++i) {
        if (environ_owned && oldflags && oldflags[i]) {
            newenv[i] = oldenv[i];
            newflags[i] = 1;
        } else {
            newenv[i] = strdup(oldenv[i]);
            if (!newenv[i]) {
                for (int j = 0; j < i; ++j)
                    if (newflags[j])
                        free(newenv[j]);
                free(newenv);
                free(newflags);
                errno = ENOMEM;
                return -1;
            }
            newflags[i] = 1;
        }
    }
    newenv[count] = (char *)str;
    newflags[count] = 0;
    newenv[count + 1] = NULL;
    if (environ_owned) {
        free(environ);
        free(environ_flags);
    }
    environ_owned = 1;
    environ = newenv;
    environ_flags = newflags;
    return 0;
}

/*
 * unsetenv() - remove NAME from the environment.
 */
int unsetenv(const char *name)
{
    if (!environ || !name || strchr(name, '='))
        return -1;
    size_t len = strlen(name);
    int idx = find_env_index(name, len);
    if (idx < 0)
        return 0;
    if (environ_owned && environ_flags && environ_flags[idx])
        free(environ[idx]);
    int i;
    for (i = idx; environ[i]; ++i) {
        environ[i] = environ[i + 1];
        if (environ_owned)
            environ_flags[i] = environ_flags[i + 1];
    }
    if (environ_owned)
        environ_flags[i] = 0;
    return 0;
}

/*
 * clearenv() - remove all environment variables and reset
 * environ to an empty list.
 */
int clearenv(void)
{
    if (!environ)
        return 0;
    if (!environ_owned) {
        for (int i = 0; environ[i]; ++i)
            environ[i] = NULL;
        return 0;
    }
    for (int i = 0; environ[i]; ++i) {
        if (environ_flags && environ_flags[i])
            free(environ[i]);
    }
    free(environ);
    free(environ_flags);
    environ = malloc(sizeof(char *));
    environ_flags = malloc(sizeof(unsigned char));
    if (!environ || !environ_flags)
        return -1;
    environ_owned = 1;
    environ[0] = NULL;
    environ_flags[0] = 0;
    return 0;
}
