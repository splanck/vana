/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the inet_pton functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "arpa/inet.h"
#include "errno.h"
#include "stdlib.h"
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>

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

static int parse_ipv6(const char *s, unsigned char *out)
{
    uint16_t words[8] = {0};
    int cur = 0;
    int zpos = -1;
    if (*s == ':') {
        if (s[1] != ':')
            return -1;
        zpos = cur;
        s += 2;
    }
    while (*s && cur < 8) {
        if (*s == ':') {
            if (zpos != -1)
                return -1;
            zpos = cur;
            s++;
            if (*s == ':')
                return -1;
            continue;
        }
        char *end;
        long val = strtol(s, &end, 16);
        if (val < 0 || val > 0xffff)
            return -1;
        words[cur++] = (uint16_t)val;
        s = end;
        if (*s == '.') {
            if (cur > 6)
                return -1;
            uint32_t ipv4;
            if (parse_ipv4(s, &ipv4) != 0)
                return -1;
            words[cur++] = (uint16_t)((ipv4 >> 16) & 0xffff);
            words[cur++] = (uint16_t)(ipv4 & 0xffff);
            s += strlen(s);
            break;
        }
        if (*s == ':')
            s++;
        else if (*s)
            return -1;
    }
    if (*s)
        return -1;
    int fill = 8 - cur;
    if (zpos == -1 && cur != 8)
        return -1;
    if (zpos != -1) {
        for (int i = cur - 1; i >= zpos; i--)
            words[i + fill] = words[i];
        for (int i = zpos; i < zpos + fill; i++)
            words[i] = 0;
    }
    for (int i = 0; i < 8; i++) {
        out[i * 2] = words[i] >> 8;
        out[i * 2 + 1] = words[i] & 0xff;
    }
    return 0;
}

int inet_pton(int af, const char *src, void *dst)
{
    if (!src || !dst) {
        errno = EINVAL;
        return -1;
    }
    if (af == AF_INET) {
        uint32_t ip;
        if (parse_ipv4(src, &ip) != 0)
            return 0;
        struct in_addr *in = dst;
        in->s_addr = htonl(ip);
        return 1;
    } else if (af == AF_INET6) {
        unsigned char buf[16];
        if (parse_ipv6(src, buf) != 0)
            return 0;
        struct in6_addr *in6 = dst;
        for (int i = 0; i < 16; i++)
            in6->s6_addr[i] = buf[i];
        return 1;
    } else {
        errno = EAFNOSUPPORT;
        return -1;
    }
}
