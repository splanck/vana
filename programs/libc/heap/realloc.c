#include "../include/stdlib.h"
#include "../include/string.h"

void *realloc(void *ptr, size_t size)
{
    if (!ptr)
        return malloc(size);

    void *new_ptr = malloc(size);
    if (new_ptr)
        memcpy(new_ptr, ptr, size);
    free(ptr);
    return new_ptr;
}
