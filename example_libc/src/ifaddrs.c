/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the ifaddrs functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#define _GNU_SOURCE
#include "ifaddrs.h"
#include "memory.h"
#include "string.h"
#include "errno.h"
#include "unistd.h"
#include "sys/socket.h"
#include "sys/ioctl.h"
#include <sys/types.h>
#include <net/if.h>
#include <netinet/in.h>

#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)

extern int host_getifaddrs(struct ifaddrs **) __asm("getifaddrs");
extern void host_freeifaddrs(struct ifaddrs *) __asm("freeifaddrs");

/*
 * getifaddrs() - BSD forwarder that calls the host implementation
 * to retrieve the linked list of interface addresses.
 */
int getifaddrs(struct ifaddrs **ifap)
{
    return host_getifaddrs(ifap);
}

/*
 * freeifaddrs() - release a list obtained via getifaddrs().
 * Simply forwards to the host implementation on BSD systems.
 */
void freeifaddrs(struct ifaddrs *ifa)
{
    host_freeifaddrs(ifa);
}

#else

/*
 * free_list() - helper used by the fallback implementation to
 * deallocate the dynamically created ifaddrs list.
 */
static void free_list(struct ifaddrs *ifa)
{
    while (ifa) {
        struct ifaddrs *next = ifa->ifa_next;
        free(ifa->ifa_name);
        free(ifa->ifa_addr);
        free(ifa->ifa_netmask);
        free(ifa->ifa_ifu.ifu_broadaddr);
        free(ifa);
        ifa = next;
    }
}

/*
 * getifaddrs() - enumerate network interfaces using ioctl
 * fallback. Builds a linked list describing each interface
 * and stores it in *ifap.
 */
int getifaddrs(struct ifaddrs **ifap)
{
    if (!ifap) {
        errno = EINVAL;
        return -1;
    }

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
        return -1;

    struct ifconf ifc;
    char buf[4096];
    memset(buf, 0, sizeof(buf));
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if (ioctl(fd, SIOCGIFCONF, &ifc) < 0) {
        close(fd);
        return -1;
    }

    struct ifaddrs *head = NULL;
    struct ifaddrs **nextp = &head;
    int n = ifc.ifc_len / sizeof(struct ifreq);
    struct ifreq *ifr = ifc.ifc_req;

    for (int i = 0; i < n; i++) {
        struct ifreq *r = &ifr[i];
        struct ifaddrs *cur = calloc(1, sizeof(*cur));
        if (!cur) {
            free_list(head);
            close(fd);
            return -1;
        }
        cur->ifa_name = strdup(r->ifr_name);
        if (r->ifr_addr.sa_family != AF_UNSPEC) {
            cur->ifa_addr = malloc(sizeof(struct sockaddr));
            if (cur->ifa_addr)
                memcpy(cur->ifa_addr, &r->ifr_addr, sizeof(struct sockaddr));
        }

        struct ifreq req;
        memset(&req, 0, sizeof(req));
        strncpy(req.ifr_name, r->ifr_name, IFNAMSIZ - 1);
        cur->ifa_flags = 0;
        if (ioctl(fd, SIOCGIFFLAGS, &req) == 0)
            cur->ifa_flags = (unsigned int)req.ifr_flags;
        if (ioctl(fd, SIOCGIFNETMASK, &req) == 0) {
            cur->ifa_netmask = malloc(sizeof(struct sockaddr));
            if (cur->ifa_netmask)
                memcpy(cur->ifa_netmask, &req.ifr_netmask,
                       sizeof(struct sockaddr));
        }
#ifdef SIOCGIFBRDADDR
        if (ioctl(fd, SIOCGIFBRDADDR, &req) == 0) {
            cur->ifa_ifu.ifu_broadaddr = malloc(sizeof(struct sockaddr));
            if (cur->ifa_ifu.ifu_broadaddr)
                memcpy(cur->ifa_ifu.ifu_broadaddr, &req.ifr_broadaddr,
                       sizeof(struct sockaddr));
        }
#endif
        *nextp = cur;
        nextp = &cur->ifa_next;
    }

    close(fd);
    *ifap = head;
    return 0;
}

/*
 * freeifaddrs() - free the list allocated by getifaddrs().
 */
void freeifaddrs(struct ifaddrs *ifa)
{
    free_list(ifa);
}

#endif

