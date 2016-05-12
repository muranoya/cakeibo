#include "util.h"
#include <stdio.h>
    
void *
try_realloc(void *pointer, size_t size)
{
    void *p = realloc(pointer, size);
    if (p == NULL)
    {
        perror("");
        exit(EXIT_FAILURE);
    }
    return p;
}

void *
try_malloc(size_t size)
{
    void *p = malloc(size);
    if (p == NULL)
    {
        perror("");
        exit(EXIT_FAILURE);
    }
    return p;
}

void *
try_calloc(size_t n, size_t size)
{
    void *p = calloc(n, size);
    if (p == NULL)
    {
        perror("");
        exit(EXIT_FAILURE);
    }
    return p;
}

