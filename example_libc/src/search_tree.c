/*
 * BSD 2-Clause License
 *
 * Purpose: Implements simple binary search tree functions tsearch,
 * tfind, tdelete and twalk used by various utilities.
 */

#include "search.h"
#include "stdlib.h"
#include "memory.h"

struct node {
    const void *key;
    struct node *left;
    struct node *right;
};

static __attribute__((unused)) void *node_key(struct node *n)
{
    return n ? (void *)&n->key : NULL;
}

void *tsearch(const void *key, void **rootp,
              int (*compar)(const void *, const void *))
{
    struct node **p = (struct node **)rootp;
    if (!*p) {
        *p = malloc(sizeof(struct node));
        if (!*p)
            return NULL;
        (*p)->key = key;
        (*p)->left = (*p)->right = NULL;
        return &(*p)->key;
    }
    int r = compar(key, (*p)->key);
    if (r < 0)
        return tsearch(key, (void **)&(*p)->left, compar);
    if (r > 0)
        return tsearch(key, (void **)&(*p)->right, compar);
    return &(*p)->key;
}

void *tfind(const void *key, void *const *rootp,
            int (*compar)(const void *, const void *))
{
    struct node * const *p = (struct node * const *)rootp;
    if (!*p)
        return NULL;
    int r = compar(key, (*p)->key);
    if (r < 0)
        return tfind(key, (void *const *)&(*p)->left, compar);
    if (r > 0)
        return tfind(key, (void *const *)&(*p)->right, compar);
    return (void *)&(*p)->key;
}

void *tdelete(const void *key, void **rootp,
              int (*compar)(const void *, const void *))
{
    if (!rootp || !*rootp)
        return NULL;

    struct node **p = (struct node **)rootp;
    struct node *parent = NULL;

    while (*p) {
        int r = compar(key, (*p)->key);
        if (r == 0)
            break;
        parent = *p;
        p = (r < 0) ? &(*p)->left : &(*p)->right;
    }

    if (!*p)
        return NULL;

    struct node *t = *p;
    void *ret = parent ? (void *)&parent->key : (void *)&t->key; /* POSIX */

    if (!t->left) {
        *p = t->right;
        free(t);
    } else if (!t->right) {
        *p = t->left;
        free(t);
    } else {
        struct node **minp = &t->right;
        struct node *min = t->right;
        while (min->left) {
            minp = &min->left;
            min = min->left;
        }
        t->key = min->key;
        struct node *child = min->right;
        free(min);
        *minp = child;
    }
    return ret;
}

static void walk(const struct node *n,
                 void (*action)(const void *, VISIT, int), int level)
{
    if (!n)
        return;
    if (!n->left && !n->right) {
        action(&n->key, leaf, level);
        return;
    }
    action(&n->key, preorder, level);
    if (n->left)
        walk(n->left, action, level + 1);
    action(&n->key, postorder, level);
    if (n->right)
        walk(n->right, action, level + 1);
    action(&n->key, endorder, level);
}

void twalk(const void *root, void (*action)(const void *, VISIT, int))
{
    walk((const struct node *)root, action, 0);
}

