/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements basic wide character I/O helpers for vlibc.
 */

#include "wchar.h"
#include "stdio.h"
/*
 * Read the next wide character from the given stream. If the stream
 * comes from open_wmemstream() the value is read directly. Otherwise
 * the next multibyte sequence is converted with mbrtowc(). Returns
 * WEOF on failure.
 */

wint_t fgetwc(FILE *stream)
{
    if (!stream)
        return -1;
    if (stream->is_wmem) {
        wchar_t wc = 0;
        if (fread(&wc, sizeof(wchar_t), 1, stream) != 1)
            return -1;
        return (wint_t)wc;
    }
    int c = fgetc(stream);
    if (c == -1)
        return -1;
    wchar_t wc = 0;
    if (mbrtowc(&wc, (char[]){(char)c, '\0'}, 1, NULL) == (size_t)-1)
        return -1;
    return (wint_t)wc;
}
/*
 * Write a wide character to the stream. Memory backed streams
 * store the character directly while normal streams use wcrtomb()
 * to encode it as multibyte bytes. Returns the character written or
 * WEOF on error.
 */

wint_t fputwc(wchar_t wc, FILE *stream)
{
    if (!stream)
        return -1;
    if (stream->is_wmem) {
        if (fwrite(&wc, sizeof(wchar_t), 1, stream) != 1)
            return -1;
        return (wint_t)wc;
    }
    char buf[8];
    size_t len = wcrtomb(buf, wc, NULL);
    if (len == (size_t)-1)
        return -1;
    if (fwrite(buf, 1, len, stream) != len)
        return -1;
    return (wint_t)wc;
}

