/*
 * BSD 2-Clause License
 *
 * Purpose: Network address structures and byte order helpers.
 */
#ifndef NETINET_IN_H
#define NETINET_IN_H

#include <sys/types.h>
#include <stdint.h>

#if defined(__has_include)
#  if __has_include("/usr/include/x86_64-linux-gnu/netinet/in.h")
#    include "/usr/include/x86_64-linux-gnu/netinet/in.h"
#    define VLIBC_NETINET_IN_NATIVE 1
#  elif __has_include("/usr/include/netinet/in.h")
#    include "/usr/include/netinet/in.h"
#    define VLIBC_NETINET_IN_NATIVE 1
#  endif
#endif

#ifndef VLIBC_NETINET_IN_NATIVE
typedef uint16_t in_port_t;
typedef uint32_t in_addr_t;

struct in_addr {
    in_addr_t s_addr;
};

struct sockaddr_in {
    sa_family_t    sin_family;
    in_port_t      sin_port;
    struct in_addr sin_addr;
    unsigned char  sin_zero[8];
};

struct in6_addr {
    unsigned char s6_addr[16];
};

struct sockaddr_in6 {
    sa_family_t     sin6_family;
    in_port_t       sin6_port;
    uint32_t        sin6_flowinfo;
    struct in6_addr sin6_addr;
    uint32_t        sin6_scope_id;
};
#endif /* !VLIBC_NETINET_IN_NATIVE */

#ifndef htons
static inline uint16_t htons(uint16_t v)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return __builtin_bswap16(v);
#else
    return v;
#endif
}
#endif

#ifndef ntohs
#define ntohs(x) htons(x)
#endif

#ifndef htonl
static inline uint32_t htonl(uint32_t v)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return __builtin_bswap32(v);
#else
    return v;
#endif
}
#endif

#ifndef ntohl
#define ntohl(x) htonl(x)
#endif

#endif /* NETINET_IN_H */
