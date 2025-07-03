/*
 * BSD 2-Clause License
 *
 * Purpose: Declarations for heap allocation functions. The allocator
 * is thread-safe because access to the free list is guarded by a mutex.
 */
#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>

/* Allocate memory from free list or via sbrk/mmap. */
void *malloc(size_t size);
/* Release memory and return it to the allocator or unmap. */
void free(void *ptr);
/* Allocate zero-filled array using malloc. */
void *calloc(size_t nmemb, size_t size);
/* Resize a block by allocating and copying. */
void *realloc(void *ptr, size_t size);
/* Resize using realloc and free original on failure. */
void *reallocf(void *ptr, size_t size);
/* Resize an array with overflow check. */
void *reallocarray(void *ptr, size_t nmemb, size_t size);
/* Resize an array and zero any newly allocated memory. */
void *recallocarray(void *ptr, size_t nmemb, size_t size);
/* Allocate aligned memory and record the original pointer. */
int posix_memalign(void **memptr, size_t alignment, size_t size);
/* Allocate memory with a given alignment. Returns NULL on failure. */
void *aligned_alloc(size_t alignment, size_t size);

/*
 * Unit tests can set this variable to N to fail the Nth allocation call.
 * Set to -1 to disable the failure mechanism.
 */
extern int vlibc_test_alloc_fail_after;

#endif /* MEMORY_H */
