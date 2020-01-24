#include <stdlib.h>
#include <string.h>

#include "memory.h"

void* reallocate(void* previous, size_t oldSize, size_t newSize, bool zero) {
    if (newSize == 0) {
        free(previous);
        return NULL;
    }
    void* alloc = realloc(previous, newSize);
    if (newSize > oldSize && zero) {
        memset((void *)((size_t)alloc + oldSize), 0, newSize - oldSize);
    }
    return alloc;
}