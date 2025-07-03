/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the iconv functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "iconv.h"
#include "string.h"
#include "stdlib.h"
#include "errno.h"

#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
#define iconv_open host_iconv_open
#define iconv host_iconv_func
#define iconv_close host_iconv_close
#include "host/iconv.h"
#undef iconv_open
#undef iconv
#undef iconv_close
extern iconv_t host_iconv_open(const char *, const char *);
extern size_t host_iconv(iconv_t, char **, size_t *, char **, size_t *);
extern int host_iconv_close(iconv_t);
#endif

enum {
    CD_ASCII_ASCII,
    CD_ASCII_UTF8,
    CD_UTF8_ASCII,
    CD_UTF8_UTF8,
    CD_HOST
};

typedef struct iconv_cd {
    int type;
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    iconv_t host;
#endif
} iconv_cd;

/* Determine whether a character set name refers to ASCII. */
static int is_ascii_cs(const char *name)
{
    return name && (!strcasecmp(name, "ASCII") || !strcasecmp(name, "US-ASCII"));
}

/* Determine whether a character set name refers to UTF-8. */
static int is_utf8_cs(const char *name)
{
    return name && (!strcasecmp(name, "UTF-8") || !strcasecmp(name, "UTF8"));
}

/*
 * iconv_open() - acquire a conversion descriptor. On BSD systems the
 * host iconv implementation is used when available; otherwise simple
 * ASCII and UTF-8 conversions are provided.
 */
iconv_t iconv_open(const char *tocode, const char *fromcode)
{
    if (!tocode || !fromcode) {
        errno = EINVAL;
        return (iconv_t)-1;
    }

#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    iconv_t h = host_iconv_open(tocode, fromcode);
    if (h != (iconv_t)-1) {
        iconv_cd *cd = malloc(sizeof(*cd));
        if (!cd) {
            host_iconv_close(h);
            errno = ENOMEM;
            return (iconv_t)-1;
        }
        cd->type = CD_HOST;
        cd->host = h;
        return (iconv_t)cd;
    }
#endif

    iconv_cd *cd = malloc(sizeof(*cd));
    if (!cd) {
        errno = ENOMEM;
        return (iconv_t)-1;
    }

    if (is_ascii_cs(fromcode) && is_ascii_cs(tocode))
        cd->type = CD_ASCII_ASCII;
    else if (is_ascii_cs(fromcode) && is_utf8_cs(tocode))
        cd->type = CD_ASCII_UTF8;
    else if (is_utf8_cs(fromcode) && is_ascii_cs(tocode))
        cd->type = CD_UTF8_ASCII;
    else if (is_utf8_cs(fromcode) && is_utf8_cs(tocode))
        cd->type = CD_UTF8_UTF8;
    else {
        free(cd);
        errno = EINVAL;
        return (iconv_t)-1;
    }

    return (iconv_t)cd;
}

/*
 * iconv() - convert a sequence of characters according to the
 * conversion descriptor. Supports basic ASCII and UTF-8 handling
 * and forwards to the host iconv implementation when present.
 */
size_t iconv(iconv_t cd_, char **inbuf, size_t *inbytesleft,
             char **outbuf, size_t *outbytesleft)
{
    iconv_cd *cd = (iconv_cd *)cd_;
    if (!cd) {
        errno = EINVAL;
        return (size_t)-1;
    }

#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    if (cd->type == CD_HOST)
        return host_iconv(cd->host, inbuf, inbytesleft, outbuf, outbytesleft);
#endif

    char *src = inbuf ? *inbuf : NULL;
    char *dst = outbuf ? *outbuf : NULL;
    size_t inleft = inbytesleft ? *inbytesleft : 0;
    size_t outleft = outbytesleft ? *outbytesleft : 0;
    size_t converted = 0;

    while (inleft > 0) {
        if (outleft == 0) {
            errno = E2BIG;
            return (size_t)-1;
        }
        unsigned char c = (unsigned char)*src;
        if (cd->type == CD_ASCII_ASCII || cd->type == CD_ASCII_UTF8) {
            if (c >= 0x80) {
                errno = EILSEQ;
                return (size_t)-1;
            }
            *dst++ = *src++;
            inleft--; outleft--; converted++;
        } else if (cd->type == CD_UTF8_ASCII) {
            if (c >= 0x80) {
                errno = EILSEQ;
                return (size_t)-1;
            }
            *dst++ = *src++;
            inleft--; outleft--; converted++;
        } else if (cd->type == CD_UTF8_UTF8) {
            *dst++ = *src++;
            inleft--; outleft--; converted++;
        } else {
            errno = EINVAL;
            return (size_t)-1;
        }
    }

    if (inbuf)
        *inbuf = src;
    if (inbytesleft)
        *inbytesleft = inleft;
    if (outbuf)
        *outbuf = dst;
    if (outbytesleft)
        *outbytesleft = outleft;

    return converted;
}

/*
 * iconv_close() - release a conversion descriptor obtained from
 * iconv_open(). Forwards to the host implementation on BSD when
 * necessary.
 */
int iconv_close(iconv_t cd_)
{
    iconv_cd *cd = (iconv_cd *)cd_;
    if (!cd)
        return -1;

#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    if (cd->type == CD_HOST) {
        int r = host_iconv_close(cd->host);
        free(cd);
        return r;
    }
#endif

    free(cd);
    return 0;
}

