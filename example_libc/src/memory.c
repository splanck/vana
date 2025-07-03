/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the memory functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "memory.h"
#include "string.h"
#include "pthread.h"
#include <stdint.h>
#include <errno.h>

/*
 * When running the unit tests we want the ability to force the next
 * allocation to fail in order to exercise error paths. Tests set this
 * flag and the next call to malloc will return NULL with errno set to
 * ENOMEM.
 */
/*
 * Tests can set this value to N to fail the Nth allocation call. A value
 * of -1 disables the failure mechanism.
 */
int vlibc_test_alloc_fail_after = -1;

struct posix_align_hdr {
    uint32_t magic;
    void    *orig;
};

#define POSIX_ALIGN_MAGIC 0x50414C47 /* 'PALG' */

#ifdef HAVE_SBRK
#include <unistd.h>
/* Declare sbrk for strict environments */
extern void *sbrk(intptr_t increment);

/*
 * Very small free list allocator. Each allocation stores a size header and
 * freed blocks are placed on a singly linked list so they can be reused by
 * subsequent malloc calls. Blocks are not coalesced or split which keeps the
 * implementation simple while still allowing memory to be recycled.
 */
struct block_header {
    size_t size;
    struct block_header *next; /* valid only when block is free */
};

static struct block_header *free_list = NULL;
static pthread_mutex_t free_lock = PTHREAD_MUTEX_INITIALIZER;

/*
 * free_impl() - return a block to the internal free list.
 */
static void free_impl(void *ptr)
{
    if (!ptr)
        return;

    struct block_header *hdr = (struct block_header *)ptr - 1;
    pthread_mutex_lock(&free_lock);
    hdr->next = free_list;
    free_list = hdr;
    pthread_mutex_unlock(&free_lock);
}

/*
 * Allocates a block from the tiny heap allocator.
 * Reuses a free-list block or gets new memory via sbrk/mmap using a size header.
 */
void *malloc(size_t size)
{
    if (vlibc_test_alloc_fail_after >= 0 && vlibc_test_alloc_fail_after-- == 0) {
        errno = ENOMEM;
        vlibc_test_alloc_fail_after = -1;
        return NULL;
    }
    if (size == 0)
        return NULL;

    /*
     * When unit tests force the next sbrk call to fail we need to bypass
     * the free list so malloc actually triggers that failure. Detect this
     * by probing sbrk(0) which will consume the failure flag used by the
     * tests and return (void *)-1 with errno set to ENOMEM.
     */
    void *cur_brk = sbrk(0);
    if (cur_brk == (void *)-1) {
        errno = ENOMEM;
        return NULL;
    }

    if (size > SIZE_MAX - sizeof(struct block_header)) {
        errno = ENOMEM;
        return NULL;
    }

    /* first-fit search through free list */
    pthread_mutex_lock(&free_lock);
    struct block_header *prev = NULL;
    struct block_header *b = free_list;
    while (b) {
        if (b->size >= size) {
            if (prev)
                prev->next = b->next;
            else
                free_list = b->next;
            pthread_mutex_unlock(&free_lock);
            b->size = size;
            return (void *)(b + 1);
        }
        prev = b;
        b = b->next;
    }
    pthread_mutex_unlock(&free_lock);

    /* allocate new block from the system */
    struct block_header *hdr = sbrk(sizeof(struct block_header) + size);
    if (hdr == (void *)-1) {
        errno = ENOMEM;
        return NULL;
    }
    hdr->size = size;
    return (void *)(hdr + 1);
}

/*
 * Releases a block back to the allocator.
 * Handles aligned allocations then returns it to the free list or unmaps it.
 */
void free(void *ptr)
{
    if (!ptr)
        return;

    struct posix_align_hdr *ph = (struct posix_align_hdr *)ptr - 1;
    if (ph->magic == POSIX_ALIGN_MAGIC) {
        void *orig = ph->orig;
        ph->magic = 0;
        free_impl(orig);
        return;
    }

    free_impl(ptr);
}

#else /* HAVE_SBRK */
#include <sys/mman.h>

struct mmap_header {
    size_t size;
};

/*
 * Allocates a block from the tiny heap allocator.
 * Reuses a free-list block or gets new memory via sbrk/mmap using a size header.
 */
void *malloc(size_t size)
{
    if (vlibc_test_alloc_fail_after >= 0 && vlibc_test_alloc_fail_after-- == 0) {
        errno = ENOMEM;
        vlibc_test_alloc_fail_after = -1;
        return NULL;
    }
    if (size == 0)
        return NULL;

    if (size > SIZE_MAX - sizeof(struct mmap_header)) {
        errno = ENOMEM;
        return NULL;
    }

    size_t total = sizeof(struct mmap_header) + size;
    struct mmap_header *hdr = mmap(NULL, total, PROT_READ | PROT_WRITE,
                                   MAP_PRIVATE | MAP_ANON, -1, 0);
    if (hdr == MAP_FAILED)
        return NULL;

    hdr->size = size;
    return (void *)(hdr + 1);
}

/*
 * free_impl() - unmap a block allocated with mmap.
 */
static void free_impl(void *ptr)
{
    if (!ptr)
        return;

    struct mmap_header *hdr = (struct mmap_header *)ptr - 1;
    munmap(hdr, hdr->size + sizeof(struct mmap_header));
}

/*
 * Releases a block back to the allocator.
 * Handles aligned allocations then returns it to the free list or unmaps it.
 */
void free(void *ptr)
{
    if (!ptr)
        return;

    struct posix_align_hdr *ph = (struct posix_align_hdr *)ptr - 1;
    if (ph->magic == POSIX_ALIGN_MAGIC) {
        void *orig = ph->orig;
        ph->magic = 0;
        free_impl(orig);
        return;
    }

    free_impl(ptr);
}

#endif /* HAVE_SBRK */

/*
 * Allocates an array and zeroes it.
 * Uses malloc for the combined size then fills the memory with zeros.
 */
void *calloc(size_t nmemb, size_t size)
{
    if (size != 0 && nmemb > SIZE_MAX / size) {
        errno = ENOMEM;
        return NULL;
    }
    size_t total = nmemb * size;
    void *ptr = malloc(total);
    if (ptr)
        vmemset(ptr, 0, total);
    return ptr;
}

/*
 * Resizes an allocated block.
 * Allocates a new block, copies the old contents, then frees the original.
 */
void *realloc(void *ptr, size_t size)
{
    if (!ptr)
        return malloc(size);

    if (size == 0) {
        free(ptr);
        return NULL;
    }

#ifdef HAVE_SBRK
    struct block_header *old_hdr = (struct block_header *)ptr - 1;
#else
    struct mmap_header *old_hdr = (struct mmap_header *)ptr - 1;
#endif
    size_t copy = old_hdr->size < size ? old_hdr->size : size;

    void *new_ptr = malloc(size);
    if (!new_ptr)
        return NULL;
    vmemcpy(new_ptr, ptr, copy);
    free(ptr);
    return new_ptr;
}

/*
 * Resizes a block but frees the original pointer if the reallocation fails.
 */
void *reallocf(void *ptr, size_t size)
{
    void *new_ptr = realloc(ptr, size);
    if (!new_ptr && size != 0)
        free(ptr);
    return new_ptr;
}
/*
 * Allocates an aligned block.
 * Reserves extra space, aligns the result and records the original pointer.
 */
int posix_memalign(void **memptr, size_t alignment, size_t size)
{
    if (!memptr)
        return EINVAL;

    if (alignment == 0 || (alignment & (alignment - 1)) != 0 ||
        alignment % sizeof(void *) != 0)
        return EINVAL;

    /*
     * Check that size plus alignment and the header won't exceed SIZE_MAX.
     * If overflow would occur, return ENOMEM without allocating.
     */
    size_t extra = alignment - 1;
    if (extra > SIZE_MAX - sizeof(struct posix_align_hdr))
        return ENOMEM;
    extra += sizeof(struct posix_align_hdr);
    if (size > SIZE_MAX - extra)
        return ENOMEM;
    size_t total = size + extra;
    void *orig = malloc(total);
    if (!orig)
        return ENOMEM;

    uintptr_t addr = (uintptr_t)orig + sizeof(struct posix_align_hdr);
    uintptr_t aligned = (addr + alignment - 1) & ~(uintptr_t)(alignment - 1);
    struct posix_align_hdr *ph = (struct posix_align_hdr *)aligned - 1;
    ph->magic = POSIX_ALIGN_MAGIC;
    ph->orig = orig;

    *memptr = (void *)aligned;
    return 0;
}

/*
 * Allocates a block with the requested alignment using posix_memalign.
 * Returns NULL on failure like standard aligned_alloc.
 */
void *aligned_alloc(size_t alignment, size_t size)
{
    if (size % alignment != 0) {
        errno = EINVAL;
        return NULL;
    }

    void *ptr = NULL;
    if (posix_memalign(&ptr, alignment, size) != 0)
        return NULL;
    return ptr;
}

/*
 * Resizes an array with overflow checking.
 * Verifies nmemb * size won't overflow and delegates to realloc.
 */
void *reallocarray(void *ptr, size_t nmemb, size_t size)
{
    if (size != 0 && nmemb > SIZE_MAX / size) {
        errno = ENOMEM;
        return NULL;
    }
    return realloc(ptr, nmemb * size);
}

/*
 * Resizes an array and zeros any newly allocated memory.
 * Preserves existing contents and behaves like calloc when ptr is NULL.
 */
void *recallocarray(void *ptr, size_t nmemb, size_t size)
{
    if (size != 0 && nmemb > SIZE_MAX / size) {
        errno = ENOMEM;
        return NULL;
    }

    size_t total = nmemb * size;

    if (!ptr)
        return calloc(nmemb, size);

    if (total == 0) {
        free(ptr);
        return NULL;
    }

#ifdef HAVE_SBRK
    struct block_header *old_hdr = (struct block_header *)ptr - 1;
#else
    struct mmap_header *old_hdr = (struct mmap_header *)ptr - 1;
#endif
    size_t old_size = old_hdr->size;

    void *new_ptr = realloc(ptr, total);
    if (!new_ptr)
        return NULL;

    if (total > old_size)
        vmemset((unsigned char *)new_ptr + old_size, 0, total - old_size);

    return new_ptr;
}
