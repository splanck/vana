/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the netdb functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "netdb.h"
#include "memory.h"
#include "string.h"
#include "io.h"
#include "errno.h"
#include "stdlib.h"
#include "stdio.h"
#include "limits.h"
#include "arpa/inet.h"
#include <stdint.h>
#include <netinet/in.h>

#ifndef O_RDONLY
#define O_RDONLY 0
#endif

#ifndef HOSTS_MAX_LINE
#define HOSTS_MAX_LINE 1024
#endif

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
    *out = (uint32_t)((a << 24) | (b << 16) | (c << 8) | d);
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

static int hosts_lookup(const char *node, uint32_t *ip)
{
    FILE *f = fopen("/etc/hosts", "r");
    if (!f)
        return -1;

    char *line = NULL;
    size_t cap = 0;
    int ret = -1;
    ssize_t len;
    while ((len = getline(&line, &cap, f)) != -1) {
        if (len >= HOSTS_MAX_LINE && line[len - 1] != '\n') {
            int c;
            while ((c = fgetc(f)) != '\n' && c != -1)
                ;
            continue;
        }
        char *p = line;
        while (*p == ' ' || *p == '\t')
            p++;
        if (*p == '#' || *p == '\n' || *p == '\0')
            continue;
        char *save;
        char *tok = strtok_r(p, " \t\n", &save);
        if (!tok)
            continue;
        uint32_t addr;
        if (parse_ipv4(tok, &addr) != 0)
            continue;
        addr = htonl(addr);
        while ((tok = strtok_r(NULL, " \t\n", &save))) {
            if (strcmp(tok, node) == 0) {
                *ip = addr;
                ret = 0;
                goto out;
            }
        }
    }
out:
    free(line);
    fclose(f);
    return ret;
}

static int hosts_reverse_lookup(uint32_t ip, char *name, size_t len)
{
    FILE *f = fopen("/etc/hosts", "r");
    if (!f)
        return -1;

    char *line = NULL;
    size_t cap = 0;
    int ret = -1;
    ssize_t len_read;
    while ((len_read = getline(&line, &cap, f)) != -1) {
        if (len_read >= HOSTS_MAX_LINE && line[len_read - 1] != '\n') {
            int c;
            while ((c = fgetc(f)) != '\n' && c != -1)
                ;
            continue;
        }
        char *p = line;
        while (*p == ' ' || *p == '\t')
            p++;
        if (*p == '#' || *p == '\n' || *p == '\0')
            continue;
        char *save;
        char *tok = strtok_r(p, " \t\n", &save);
        if (!tok)
            continue;
        uint32_t addr;
        if (parse_ipv4(tok, &addr) != 0)
            continue;
        addr = htonl(addr);
        if (addr == ip) {
            tok = strtok_r(NULL, " \t\n", &save);
            if (!tok)
                break;
            strncpy(name, tok, len - 1);
            name[len - 1] = '\0';
            ret = 0;
            break;
        }
    }
    free(line);
    fclose(f);
    return ret;
}


int getaddrinfo(const char *node, const char *service,
                const struct addrinfo *hints, struct addrinfo **res)
{
    (void)hints;
    if (!node && !service)
        return EAI_NONAME;

    uint32_t ip4 = 0;
    unsigned char ip6[16];
    int family = AF_UNSPEC;
    if (node) {
        if (parse_ipv6(node, ip6) == 0) {
            family = AF_INET6;
        } else if (parse_ipv4(node, &ip4) == 0) {
            ip4 = htonl(ip4);
            family = AF_INET;
        } else {
            if (hosts_lookup(node, &ip4) != 0) {
                return EAI_NONAME;
            }
            family = AF_INET;
        }
    } else {
        family = AF_INET;
    }

    uint16_t port = 0;
    if (service)
        port = (uint16_t)atoi(service);

    struct addrinfo *ai = malloc(sizeof(struct addrinfo));
    if (!ai)
        return EAI_MEMORY;
    ai->ai_flags = 0;
    ai->ai_family = family;
    ai->ai_socktype = 0;
    ai->ai_protocol = 0;
    ai->ai_next = NULL;

    if (family == AF_INET6) {
        struct sockaddr_in6 *sa6 = malloc(sizeof(struct sockaddr_in6));
        if (!sa6) {
            free(ai);
            return EAI_MEMORY;
        }
        sa6->sin6_family = AF_INET6;
        sa6->sin6_port = htons(port);
        memcpy(&sa6->sin6_addr, ip6, 16);
        sa6->sin6_flowinfo = 0;
        sa6->sin6_scope_id = 0;
        ai->ai_addrlen = sizeof(struct sockaddr_in6);
        ai->ai_addr = (struct sockaddr *)sa6;
    } else {
        struct sockaddr_in *sa = malloc(sizeof(struct sockaddr_in));
        if (!sa) {
            free(ai);
            return EAI_MEMORY;
        }
        sa->sin_family = AF_INET;
        sa->sin_port = htons(port);
        sa->sin_addr.s_addr = ip4;
        memset(sa->sin_zero, 0, sizeof(sa->sin_zero));
        ai->ai_addrlen = sizeof(struct sockaddr_in);
        ai->ai_addr = (struct sockaddr *)sa;
    }
    ai->ai_canonname = NULL;

    *res = ai;
    return 0;
}

void freeaddrinfo(struct addrinfo *res)
{
    while (res) {
        struct addrinfo *next = res->ai_next;
        if (res->ai_addr)
            free(res->ai_addr);
        if (res->ai_canonname)
            free(res->ai_canonname);
        free(res);
        res = next;
    }
}

int getnameinfo(const struct sockaddr *sa, socklen_t salen,
                char *host, socklen_t hostlen,
                char *serv, socklen_t servlen, int flags)
{
    (void)flags;
    if (!sa)
        return -1;
    if (sa->sa_family == AF_INET) {
        if (salen < (socklen_t)sizeof(struct sockaddr_in))
            return -1;
        const struct sockaddr_in *sin = (const struct sockaddr_in *)sa;
        if (host && hostlen > 0)
            inet_ntop(AF_INET, &sin->sin_addr, host, hostlen);
        if (serv && servlen > 0)
            snprintf(serv, servlen, "%u", ntohs(sin->sin_port));
        return 0;
    } else if (sa->sa_family == AF_INET6) {
        if (salen < (socklen_t)sizeof(struct sockaddr_in6))
            return -1;
        const struct sockaddr_in6 *sin6 = (const struct sockaddr_in6 *)sa;
        if (host && hostlen > 0)
            inet_ntop(AF_INET6, &sin6->sin6_addr, host, hostlen);
        if (serv && servlen > 0)
            snprintf(serv, servlen, "%u", ntohs(sin6->sin6_port));
        return 0;
    }
    return -1;
}

static struct hostent he;
static char *he_aliases[1];
static char *he_addr_list[2];
static struct in_addr he_addr4;
static struct in6_addr he_addr6;
static char he_name[NI_MAXHOST];

struct hostent *gethostbyname(const char *name)
{
    uint32_t ip4;
    if (hosts_lookup(name, &ip4) == 0) {
        he_addr4.s_addr = ip4;
        he.h_name = (char *)name;
        he.h_aliases = he_aliases;
        he_aliases[0] = NULL;
        he.h_addrtype = AF_INET;
        he.h_length = sizeof(struct in_addr);
        he_addr_list[0] = (char *)&he_addr4;
        he_addr_list[1] = NULL;
        he.h_addr_list = he_addr_list;
        return &he;
    }

    struct addrinfo *ai;
    if (getaddrinfo(name, NULL, NULL, &ai) != 0)
        return NULL;

    if (ai->ai_family == AF_INET) {
        struct sockaddr_in *sa = (struct sockaddr_in *)ai->ai_addr;
        he_addr4.s_addr = sa->sin_addr.s_addr;
        he.h_addrtype = AF_INET;
        he.h_length = sizeof(struct in_addr);
        he_addr_list[0] = (char *)&he_addr4;
    } else if (ai->ai_family == AF_INET6) {
        struct sockaddr_in6 *sa6 = (struct sockaddr_in6 *)ai->ai_addr;
        memcpy(&he_addr6, &sa6->sin6_addr, sizeof(struct in6_addr));
        he.h_addrtype = AF_INET6;
        he.h_length = sizeof(struct in6_addr);
        he_addr_list[0] = (char *)&he_addr6;
    } else {
        freeaddrinfo(ai);
        return NULL;
    }
    he_addr_list[1] = NULL;
    he.h_name = (char *)name;
    he.h_aliases = he_aliases;
    he_aliases[0] = NULL;
    he.h_addr_list = he_addr_list;
    freeaddrinfo(ai);
    return &he;
}

struct hostent *gethostbyaddr(const void *addr, socklen_t len, int type)
{
    he.h_aliases = he_aliases;
    he_aliases[0] = NULL;
    he.h_addr_list = he_addr_list;
    he_addr_list[1] = NULL;

    if (type == AF_INET && len == sizeof(struct in_addr)) {
        uint32_t ip = *(const uint32_t *)addr;
        if (hosts_reverse_lookup(ip, he_name, sizeof(he_name)) != 0) {
            struct sockaddr_in sa;
            sa.sin_family = AF_INET;
            sa.sin_port = 0;
            sa.sin_addr.s_addr = ip;
            if (getnameinfo((struct sockaddr *)&sa, sizeof(sa),
                            he_name, sizeof(he_name), NULL, 0, 0) != 0)
                return NULL;
        }
        he_addr4.s_addr = ip;
        he.h_name = he_name;
        he.h_addrtype = AF_INET;
        he.h_length = sizeof(struct in_addr);
        he_addr_list[0] = (char *)&he_addr4;
        return &he;
    } else if (type == AF_INET6 && len == sizeof(struct in6_addr)) {
        memcpy(&he_addr6, addr, sizeof(struct in6_addr));
        struct sockaddr_in6 sa6;
        memset(&sa6, 0, sizeof(sa6));
        sa6.sin6_family = AF_INET6;
        memcpy(&sa6.sin6_addr, addr, sizeof(struct in6_addr));
        if (getnameinfo((struct sockaddr *)&sa6, sizeof(sa6),
                        he_name, sizeof(he_name), NULL, 0, 0) != 0)
            return NULL;
        he.h_name = he_name;
        he.h_addrtype = AF_INET6;
        he.h_length = sizeof(struct in6_addr);
        he_addr_list[0] = (char *)&he_addr6;
        return &he;
    }
    errno = EAFNOSUPPORT;
    return NULL;
}

static int fill_hostent(const char *name, int type, const void *addr,
                        struct hostent *ret, char *buf, size_t buflen)
{
    size_t need = sizeof(char *) + 2 * sizeof(char *);
    need += strlen(name) + 1;
    need += (type == AF_INET) ? sizeof(struct in_addr)
                              : sizeof(struct in6_addr);
    if (need > buflen)
        return -1;

    char **aliases = (char **)buf;
    buf += sizeof(char *);
    char **addrs = (char **)buf;
    buf += 2 * sizeof(char *);

    size_t nlen = strlcpy(buf, name, buflen - 2 * sizeof(char *));
    if (nlen >= buflen - 2 * sizeof(char *))
        return -1;
    ret->h_name = buf;
    buf += nlen + 1;

    size_t alen = (type == AF_INET) ? sizeof(struct in_addr)
                                    : sizeof(struct in6_addr);
    memcpy(buf, addr, alen);

    aliases[0] = NULL;
    addrs[0] = buf;
    addrs[1] = NULL;

    ret->h_aliases = aliases;
    ret->h_addrtype = type;
    ret->h_length = alen;
    ret->h_addr_list = addrs;
    return 0;
}

int gethostbyname_r(const char *name, struct hostent *ret,
                    char *buf, size_t buflen, struct hostent **result)
{
    if (!ret || !buf || !result)
        return -1;
    *result = NULL;

    uint32_t ip4;
    if (hosts_lookup(name, &ip4) == 0) {
        if (fill_hostent(name, AF_INET, &ip4, ret, buf, buflen) != 0)
            return -1;
        *result = ret;
        return 0;
    }

    struct addrinfo *ai;
    if (getaddrinfo(name, NULL, NULL, &ai) != 0)
        return -1;

    int r = -1;
    if (ai->ai_family == AF_INET) {
        struct sockaddr_in *sa = (struct sockaddr_in *)ai->ai_addr;
        r = fill_hostent(name, AF_INET, &sa->sin_addr, ret, buf, buflen);
    } else if (ai->ai_family == AF_INET6) {
        struct sockaddr_in6 *sa6 = (struct sockaddr_in6 *)ai->ai_addr;
        r = fill_hostent(name, AF_INET6, &sa6->sin6_addr, ret, buf, buflen);
    }
    freeaddrinfo(ai);
    if (r == 0)
        *result = ret;
    return r;
}

int gethostbyaddr_r(const void *addr, socklen_t len, int type,
                    struct hostent *ret, char *buf, size_t buflen,
                    struct hostent **result)
{
    if (!ret || !buf || !result)
        return -1;
    *result = NULL;

    char namebuf[NI_MAXHOST];

    if (type == AF_INET && len == sizeof(struct in_addr)) {
        uint32_t ip = *(const uint32_t *)addr;
        if (hosts_reverse_lookup(ip, namebuf, sizeof(namebuf)) != 0) {
            struct sockaddr_in sa;
            sa.sin_family = AF_INET;
            sa.sin_port = 0;
            sa.sin_addr = *(const struct in_addr *)addr;
            if (getnameinfo((struct sockaddr *)&sa, sizeof(sa),
                            namebuf, sizeof(namebuf), NULL, 0, 0) != 0)
                return -1;
        }
        if (fill_hostent(namebuf, AF_INET, addr, ret, buf, buflen) != 0)
            return -1;
        *result = ret;
        return 0;
    } else if (type == AF_INET6 && len == sizeof(struct in6_addr)) {
        struct sockaddr_in6 sa6;
        memset(&sa6, 0, sizeof(sa6));
        sa6.sin6_family = AF_INET6;
        memcpy(&sa6.sin6_addr, addr, sizeof(struct in6_addr));
        if (getnameinfo((struct sockaddr *)&sa6, sizeof(sa6),
                        namebuf, sizeof(namebuf), NULL, 0, 0) != 0)
            return -1;
        if (fill_hostent(namebuf, AF_INET6, addr, ret, buf, buflen) != 0)
            return -1;
        *result = ret;
        return 0;
    }
    errno = EAFNOSUPPORT;
    return -1;
}

struct gai_entry {
    int code;
    const char *msg;
};

static const struct gai_entry gai_table[] = {
    { EAI_BADFLAGS, "Invalid value for ai_flags" },
    { EAI_NONAME, "Name or service not known" },
    { EAI_AGAIN, "Temporary failure in name resolution" },
    { EAI_FAIL, "Non-recoverable failure in name resolution" },
    { EAI_FAMILY, "Address family not supported" },
    { EAI_SOCKTYPE, "Socket type not supported" },
    { EAI_SERVICE, "Service not supported for socket type" },
    { EAI_MEMORY, "Memory allocation failure" },
    { EAI_SYSTEM, "System error" },
    { EAI_OVERFLOW, "Argument buffer overflow" },
    { 0, NULL }
};

const char *gai_strerror(int errcode)
{
    for (size_t i = 0; gai_table[i].msg; i++) {
        if (gai_table[i].code == errcode)
            return gai_table[i].msg;
    }
    static char buf[32];
    snprintf(buf, sizeof(buf), "Unknown error %d", errcode);
    return buf;
}
