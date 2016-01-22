#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>

void *try_realloc(void *pointer, size_t size);
void *try_malloc(size_t size);
void *try_calloc(size_t n, size_t size);

#endif
