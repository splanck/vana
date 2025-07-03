/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that the copyright notice and
 * this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the inet_addr function for vlibc. Parses IPv4 strings
 * using inet_aton to return an address in network byte order.
 *
 * Copyright (c) 2025
 */

#include "arpa/inet.h"
#include "netinet/in.h"

in_addr_t inet_addr(const char *cp)
{
    struct in_addr in;
    if (!inet_aton(cp, &in))
        return (in_addr_t)0xffffffff; /* INADDR_NONE */
    return in.s_addr;
}

