/*
 * BSD 2-Clause License
 *
 * Purpose: Declarations for character set conversion helpers.
 */
#ifndef ICONV_H
#define ICONV_H

#include <stddef.h>

typedef void *iconv_t;

iconv_t iconv_open(const char *tocode, const char *fromcode);
size_t iconv(iconv_t cd, char **inbuf, size_t *inbytesleft,
             char **outbuf, size_t *outbytesleft);
int iconv_close(iconv_t cd);

#endif /* ICONV_H */
