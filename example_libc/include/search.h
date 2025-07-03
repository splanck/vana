/*
 * BSD 2-Clause License
 *
 * Purpose: Declarations for hash table and binary search tree helpers.
 */
#ifndef SEARCH_H
#define SEARCH_H

#include <stddef.h>

typedef struct entry {
    char *key;
    void *data;
} ENTRY;

typedef enum { FIND, ENTER } ACTION;

int hcreate(size_t nel);
void hdestroy(void);
ENTRY *hsearch(ENTRY item, ACTION action);

typedef enum { preorder, postorder, endorder, leaf } VISIT;
void *tsearch(const void *key, void **rootp,
              int (*compar)(const void *, const void *));
void *tfind(const void *key, void *const *rootp,
            int (*compar)(const void *, const void *));
void *tdelete(const void *key, void **rootp,
              int (*compar)(const void *, const void *));
void twalk(const void *root, void (*action)(const void *, VISIT, int));

#endif /* SEARCH_H */
