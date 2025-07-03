/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the inet_aton and inet_ntoa functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "arpa/inet.h"
#include "stdio.h"
#include "stdlib.h"
#include <stdint.h>
#include <string.h>

static int parse_ipv4(const char *s, uint32_t *out)
{
    char *end;
    long a = strtol(s, &end, 10);
    if (a < 0 || a > 255 || *end != '.')
        return -1;
    s = end + 1;
    long b = strtol(s, &end, 10);
    if (b < 0 || b > 255 || *end != '.')
        return -1;
    s = end + 1;
    long c = strtol(s, &end, 10);
    if (c < 0 || c > 255 || *end != '.')
        return -1;
    s = end + 1;
    long d = strtol(s, &end, 10);
    if (d < 0 || d > 255 || (*end && *end != ' ' && *end != '\t'))
        return -1;
    *out = ((uint32_t)a << 24) | ((uint32_t)b << 16) |
           ((uint32_t)c << 8) | (uint32_t)d;
    return 0;
}

int inet_aton(const char *cp, struct in_addr *inp)
{
    if (!cp || !inp)
        return 0;
    uint32_t ip;
    if (parse_ipv4(cp, &ip) != 0)
        return 0;
    inp->s_addr = htonl(ip);
    return 1;
}

char *inet_ntoa(struct in_addr in)
{
    static __thread char buf[INET_ADDRSTRLEN];
    uint32_t addr = ntohl(in.s_addr);
    unsigned char a = (addr >> 24) & 0xFF;
    unsigned char b = (addr >> 16) & 0xFF;
    unsigned char c = (addr >> 8) & 0xFF;
    unsigned char d = addr & 0xFF;
    snprintf(buf, sizeof(buf), "%u.%u.%u.%u", a, b, c, d);
    return buf;
}
