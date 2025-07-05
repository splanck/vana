#include "../include/stdlib.h"
#include "../include/string.h"

void *calloc(size_t nmemb, size_t size)
{
    if (size != 0 && nmemb > SIZE_MAX / size)
        return (void*)0;

    size_t total = nmemb * size;
    void *ptr = malloc(total);
    if (ptr)
        memset(ptr, 0, total);
    return ptr;
}
