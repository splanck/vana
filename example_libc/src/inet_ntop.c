/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the inet_ntop functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "arpa/inet.h"
#include "errno.h"
#include "stdio.h"
#include <arpa/inet.h>

static char *hex16(char *p, uint16_t val)
{
    int started = 0;
    for (int i = 12; i >= 0; i -= 4) {
        int d = (val >> i) & 0xF;
        if (d || started || i == 0) {
            *p++ = d < 10 ? '0' + d : 'a' + d - 10;
            started = 1;
        }
    }
    return p;
}

const char *inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
    if (!src || !dst || size == 0) {
        errno = EINVAL;
        return NULL;
    }
    if (af == AF_INET) {
        if (size < INET_ADDRSTRLEN) {
            errno = ENOSPC;
            return NULL;
        }
        const struct in_addr *in = src;
        uint32_t addr = ntohl(in->s_addr);
        unsigned char a = (addr >> 24) & 0xFF;
        unsigned char b = (addr >> 16) & 0xFF;
        unsigned char c = (addr >> 8) & 0xFF;
        unsigned char d = addr & 0xFF;
        snprintf(dst, size, "%u.%u.%u.%u", a, b, c, d);
        return dst;
    } else if (af == AF_INET6) {
        if (size < INET6_ADDRSTRLEN) {
            errno = ENOSPC;
            return NULL;
        }
        const struct in6_addr *in6 = src;
        uint16_t words[8];
        for (int i = 0; i < 8; i++)
            words[i] = ((uint16_t)in6->s6_addr[i * 2] << 8) |
                       in6->s6_addr[i * 2 + 1];
        int best_base = -1, best_len = 0;
        int cur_base = -1, cur_len = 0;
        for (int i = 0; i < 8; i++) {
            if (words[i] == 0) {
                if (cur_base == -1) {
                    cur_base = i;
                    cur_len = 1;
                } else {
                    cur_len++;
                }
            } else {
                if (cur_base != -1) {
                    if (cur_len > best_len) {
                        best_base = cur_base;
                        best_len = cur_len;
                    }
                    cur_base = -1;
                    cur_len = 0;
                }
            }
        }
        if (cur_base != -1 && cur_len > best_len) {
            best_base = cur_base;
            best_len = cur_len;
        }
        if (best_len < 2)
            best_base = -1;

        char buf[INET6_ADDRSTRLEN];
        char *p = buf;
        for (int i = 0; i < 8; i++) {
            if (best_base == i) {
                *p++ = ':'; *p++ = ':';
                i += best_len - 1;
                continue;
            }
            if (i > 0 && p[-1] != ':')
                *p++ = ':';
            p = hex16(p, words[i]);
        }
        if (best_base != -1 && best_base + best_len == 8)
            *p++ = ':';
        *p = '\0';
        snprintf(dst, size, "%s", buf);
        return dst;
    } else {
        errno = EAFNOSUPPORT;
        return NULL;
    }
}
