#include "../include/stdlib.h"
#include <stdint.h>

extern void *sbrk(intptr_t inc);

static char *heap_end;

void *malloc(size_t size)
{
    if (!heap_end)
        heap_end = sbrk(0);

    void *prev = heap_end;
    void *res = sbrk(size);
    if (res == (void*)-1)
        return (void*)0;
    heap_end = (char*)res + size;
    return prev;
}
