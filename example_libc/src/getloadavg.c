/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that the copyright notice and
 * this permission notice appear in all copies. This software is provided "as is"
 * without warranty.
 *
 * Purpose: Implements the getloadavg functions for vlibc. Provides wrappers and
 * helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "stdlib.h"

#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__) || defined(__APPLE__)
#include <sys/sysctl.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/sysctl.h>

/* BSD variant reading VM_LOADAVG via sysctl */

int getloadavg(double loadavg[], int nelem)
{
    if (!loadavg || nelem <= 0)
        return 0;
    struct loadavg load;
    size_t len = sizeof(load);
    int mib[2] = { CTL_VM, VM_LOADAVG };
    if (sysctl(mib, 2, &load, &len, NULL, 0) == -1)
        return -1;
    int n = nelem > 3 ? 3 : nelem;
    for (int i = 0; i < n; i++)
        loadavg[i] = (double)load.ldavg[i] / load.fscale;
    return n;
}

#else /* Linux and others */
#include "stdio.h"

/* Linux variant parsing /proc/loadavg */

int getloadavg(double loadavg[], int nelem)
{
    if (!loadavg || nelem <= 0)
        return 0;
    FILE *f = fopen("/proc/loadavg", "r");
    if (!f)
        return -1;
    double a = 0.0, b = 0.0, c = 0.0;
    int r = fscanf(f, "%lf %lf %lf", &a, &b, &c);
    fclose(f);
    if (r < 3)
        return -1;
    int n = nelem > 3 ? 3 : nelem;
    if (n >= 1) loadavg[0] = a;
    if (n >= 2) loadavg[1] = b;
    if (n >= 3) loadavg[2] = c;
    return n;
}
#endif
