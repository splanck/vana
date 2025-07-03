/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the dirent functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#define _GNU_SOURCE
#include <dirent.h>
#include "dirent.h"
#undef opendir
#undef readdir
#undef closedir

/* Open a directory stream, returning a DIR handle. */
DIR *vlibc_opendir(const char *name)
{
    return opendir(name);
}

/* Read the next directory entry from the stream. */
struct dirent *vlibc_readdir(DIR *dirp)
{
    return readdir(dirp);
}

/* Close a directory stream opened with vlibc_opendir(). */
int vlibc_closedir(DIR *dirp)
{
    return closedir(dirp);
}
