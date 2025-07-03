/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the string_extra functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "string.h"
#include "ctype.h"

/* Locate first occurrence of byte c in the first n bytes of s */
void *memchr(const void *s, int c, size_t n)
{
    const unsigned char *p = s;
    unsigned char ch = (unsigned char)c;
    while (n--) {
        if (*p == ch)
            return (void *)p;
        p++;
    }
    return NULL;
}

/* Find byte c in s searching backwards */
void *memrchr(const void *s, int c, size_t n)
{
    const unsigned char *p = (const unsigned char *)s + n;
    unsigned char ch = (unsigned char)c;
    while (n--) {
        if (*--p == ch)
            return (void *)p;
    }
    return NULL;
}

/* Search haystack for the first occurrence of needle */
void *memmem(const void *haystack, size_t haystacklen,
             const void *needle, size_t needlelen)
{
    if (needlelen == 0)
        return (void *)haystack;
    if (haystacklen < needlelen)
        return NULL;
    const unsigned char *h = haystack;
    const unsigned char *n = needle;
    size_t end = haystacklen - needlelen + 1;
    for (size_t i = 0; i < end; i++) {
        if (h[i] == n[0] && memcmp(h + i, n, needlelen) == 0)
            return (void *)(h + i);
    }
    return NULL;
}

/* Locate last occurrence of character c in string s */
char *strrchr(const char *s, int c)
{
    const char *ret = NULL;
    unsigned char ch = (unsigned char)c;
    while (1) {
        if (*s == ch)
            ret = s;
        if (*s == '\0')
            break;
        s++;
    }
    return (char *)ret;
}

/* Locate substring needle in haystack */
char *strstr(const char *haystack, const char *needle)
{
    if (!*needle)
        return (char *)haystack;
    size_t nlen = vstrlen(needle);
    while (*haystack) {
        if (*haystack == *needle && vstrncmp(haystack, needle, nlen) == 0)
            return (char *)haystack;
        haystack++;
    }
    return NULL;
}

/* Compare two strings ignoring case for up to n chars */
int strncasecmp(const char *s1, const char *s2, size_t n)
{
    while (n--) {
        unsigned char c1 = (unsigned char)*s1++;
        unsigned char c2 = (unsigned char)*s2++;
        int diff = tolower(c1) - tolower(c2);
        if (diff)
            return diff;
        if (c1 == '\0')
            break;
    }
    return 0;
}

/* Compare two strings ignoring case */
int strcasecmp(const char *s1, const char *s2)
{
    return strncasecmp(s1, s2, (size_t)-1);
}

/* Case-insensitive search of needle in haystack */
char *strcasestr(const char *haystack, const char *needle)
{
    if (!*needle)
        return (char *)haystack;
    size_t nlen = vstrlen(needle);
    while (*haystack) {
        if (tolower((unsigned char)*haystack) ==
            tolower((unsigned char)*needle) &&
            strncasecmp(haystack, needle, nlen) == 0)
            return (char *)haystack;
        haystack++;
    }
    return NULL;
}

/* Copy src to dst ensuring null termination */
size_t strlcpy(char *dst, const char *src, size_t size)
{
    size_t len = vstrlen(src);
    if (size) {
        size_t copy = len >= size ? size - 1 : len;
        vmemcpy(dst, src, copy);
        dst[copy] = '\0';
    }
    return len;
}

/* Append src to dst ensuring space for size bytes */
size_t strlcat(char *dst, const char *src, size_t size)
{
    size_t dlen = vstrlen(dst);
    size_t slen = vstrlen(src);

    if (dlen < size) {
        size_t copy = slen >= size - dlen ? size - dlen - 1 : slen;
        vmemcpy(dst + dlen, src, copy);
        dst[dlen + copy] = '\0';
    }

    return dlen + slen;
}

/* Count length of initial segment of s consisting of accept chars */
size_t strspn(const char *s, const char *accept)
{
    size_t count = 0;
    while (*s) {
        const char *a = accept;
        int found = 0;
        while (*a) {
            if (*s == *a) {
                found = 1;
                break;
            }
            a++;
        }
        if (!found)
            break;
        ++count;
        ++s;
    }
    return count;
}

/* Count length of initial segment of s containing no reject chars */
size_t strcspn(const char *s, const char *reject)
{
    size_t count = 0;
    while (*s) {
        const char *r = reject;
        int stop = 0;
        while (*r) {
            if (*s == *r) {
                stop = 1;
                break;
            }
            r++;
        }
        if (stop)
            break;
        ++count;
        ++s;
    }
    return count;
}

/* Find first matching character from accept in s */
char *strpbrk(const char *s, const char *accept)
{
    while (*s) {
        const char *a = accept;
        while (*a) {
            if (*s == *a)
                return (char *)s;
            a++;
        }
        s++;
    }
    return NULL;
}

/* Compare strings using current locale */
int strcoll(const char *s1, const char *s2)
{
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    const char *loc = setlocale(LC_COLLATE, NULL);
    if (loc && strcmp(loc, "C") != 0 && strcmp(loc, "POSIX") != 0) {
        extern int host_strcoll(const char *, const char *) __asm("strcoll");
        return host_strcoll(s1, s2);
    }
#endif
    return strcmp(s1, s2);
}

/* Transform src for locale collation into dest */
size_t strxfrm(char *dest, const char *src, size_t n)
{
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    const char *loc = setlocale(LC_COLLATE, NULL);
    if (loc && strcmp(loc, "C") != 0 && strcmp(loc, "POSIX") != 0) {
        extern size_t host_strxfrm(char *, const char *, size_t) __asm("strxfrm");
        return host_strxfrm(dest, src, n);
    }
#endif
    size_t len = vstrlen(src);
    if (n) {
        size_t copy = len >= n ? n - 1 : len;
        if (dest) {
            vmemcpy(dest, src, copy);
            dest[copy] = '\0';
        }
    }
    return len;
}

/* Tokenize stringp using delimiters in delim */
char *strsep(char **stringp, const char *delim)
{
    if (!stringp || !*stringp)
        return NULL;
    char *s = *stringp;
    char *tok = s;
    while (*s) {
        const char *d = delim;
        while (*d) {
            if (*s == *d) {
                *s = '\0';
                *stringp = s + 1;
                return tok;
            }
            d++;
        }
        s++;
    }
    *stringp = NULL;
    return tok;
}

/* Copy src to dest returning pointer to trailing null */
char *stpcpy(char *dest, const char *src)
{
    char *d = dest;
    while ((*d++ = *src++) != '\0')
        ;
    return d - 1;
}

/* Copy up to n chars from src to dest returning end pointer */
char *stpncpy(char *dest, const char *src, size_t n)
{
    char *d = dest;
    while (n && *src) {
        *d++ = *src++;
        --n;
    }
    if (n) {
        vmemset(d, 0, n);
        return d + n;
    }
    return d;
}

/* Copy bytes until c is found or n exhausted */
void *memccpy(void *dest, const void *src, int c, size_t n)
{
    unsigned char *d = dest;
    const unsigned char *s = src;
    unsigned char ch = (unsigned char)c;
    while (n--) {
        unsigned char byte = *s++;
        *d++ = byte;
        if (byte == ch)
            return d;
    }
    return NULL;
}

/* Copy n bytes from src to dest returning pointer past end */
void *mempcpy(void *dest, const void *src, size_t n)
{
    vmemcpy(dest, src, n);
    return (unsigned char *)dest + n;
}

