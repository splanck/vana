/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms,
 * with or without modification, are permitted provided that the copyright
 * notice and this permission notice appear in all copies. This software is
 * provided "as is" without warranty.
 *
 * Purpose: Implements program name helpers for vlibc.
 */

#include "string.h"
#include "util.h"

static const char *progname = "";

/*
 * getprogname() - retrieve the stored program name.  If no name has been
 * set then an empty string is returned.
 */
const char *getprogname(void)
{
    return progname;
}

/*
 * setprogname() - store the basename of the provided executable path so it
 * can be returned by getprogname().  Passing NULL resets the name to an
 * empty string.
 */
void setprogname(const char *name)
{
    if (!name) {
        progname = "";
        return;
    }
    const char *base = strrchr(name, '/');
    progname = base ? base + 1 : name;
}
