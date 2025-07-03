/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the qsort functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "stdlib.h"

static void swap(char *a, char *b, size_t size)
{
    while (size--) {
        char tmp = *a;
        *a++ = *b;
        *b++ = tmp;
    }
}

/*
 * qsort() - sort an array using the supplied comparison function.
 * Elements are sorted in ascending order using a simple bubble sort
 * implementation as this file favors clarity over raw speed.
 */
void qsort(void *base, size_t nmemb, size_t size,
           int (*compar)(const void *, const void *))
{
    char *b = base;
    for (size_t i = 0; i < nmemb; i++) {
        for (size_t j = i + 1; j < nmemb; j++) {
            char *pi = b + i * size;
            char *pj = b + j * size;
            if (compar(pi, pj) > 0)
                swap(pi, pj, size);
        }
    }
}

/*
 * qsort_r() - reentrant variant of qsort().  An additional context pointer
 * is passed to the comparison function.
 */
void qsort_r(void *base, size_t nmemb, size_t size,
             int (*compar)(const void *, const void *, void *), void *ctx)
{
    char *b = base;
    for (size_t i = 0; i < nmemb; i++) {
        for (size_t j = i + 1; j < nmemb; j++) {
            char *pi = b + i * size;
            char *pj = b + j * size;
            if (compar(pi, pj, ctx) > 0)
                swap(pi, pj, size);
        }
    }
}

/*
 * bsearch() - binary search an array for a matching element.  The array
 * must already be sorted according to the comparison function.
 */
void *bsearch(const void *key, const void *base, size_t nmemb, size_t size,
              int (*compar)(const void *, const void *))
{
    size_t low = 0;
    size_t high = nmemb;
    const char *b = base;

    while (low < high) {
        size_t mid = (low + high) / 2;
        const void *elem = b + mid * size;
        int c = compar(key, elem);
        if (c < 0)
            high = mid;
        else if (c > 0)
            low = mid + 1;
        else
            return (void *)elem;
    }
    return NULL;
}
