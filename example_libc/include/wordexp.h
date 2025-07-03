/*
 * BSD 2-Clause License
 *
 * Purpose: Declarations for shell word expansion helpers.
 */
#ifndef WORDEXP_H
#define WORDEXP_H

#include <stddef.h>

/* result structure for word expansion */
typedef struct {
    size_t we_wordc;  /* count of words */
    char **we_wordv;  /* NULL terminated list */
    size_t we_offs;   /* reserved */
} wordexp_t;

/* error codes */
#define WRDE_NOSPACE 1
#define WRDE_BADCHAR 2
#define WRDE_SYNTAX  3

int wordexp(const char *words, wordexp_t *pwordexp);
void wordfree(wordexp_t *pwordexp);

#endif /* WORDEXP_H */
