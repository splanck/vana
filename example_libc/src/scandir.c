/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the scandir functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "dirent.h"
#include "memory.h"
#include "string.h"
#include "stdlib.h"
#include "errno.h"
#include <stddef.h>

int alphasort(const struct dirent **a, const struct dirent **b)
{
    return strcmp((*a)->d_name, (*b)->d_name);
}

int scandir(const char *path, struct dirent ***namelist,
            int (*filter)(const struct dirent *),
            int (*compar)(const struct dirent **, const struct dirent **))
{
    if (!path || !namelist) {
        errno = EINVAL;
        return -1;
    }

    DIR *dir = opendir(path);
    if (!dir)
        return -1;

    size_t cap = 16;
    size_t count = 0;
    struct dirent **list = malloc(cap * sizeof(struct dirent *));
    if (!list) {
        closedir(dir);
        errno = ENOMEM;
        return -1;
    }

    struct dirent *ent;
    while ((ent = readdir(dir))) {
        if (filter && !filter(ent))
            continue;

        size_t len = offsetof(struct dirent, d_name) + strlen(ent->d_name) + 1;
        struct dirent *copy = malloc(len);
        if (!copy) {
            errno = ENOMEM;
            goto fail;
        }
        memcpy(copy, ent, len);

        if (count == cap) {
            size_t new_cap = cap * 2;
            struct dirent **tmp = realloc(list, new_cap * sizeof(struct dirent *));
            if (!tmp) {
                free(copy);
                errno = ENOMEM;
                goto fail;
            }
            list = tmp;
            cap = new_cap;
        }
        list[count++] = copy;
    }
    closedir(dir);

    if (compar && count > 1)
        qsort(list, count, sizeof(struct dirent *),
              (int (*)(const void *, const void *))compar);

    *namelist = list;
    return (int)count;

fail:
    closedir(dir);
    for (size_t i = 0; i < count; i++)
        free(list[i]);
    free(list);
    return -1;
}
