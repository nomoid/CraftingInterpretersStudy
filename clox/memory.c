#include <stdlib.h>

#define UNUSED(x) (void)(x)

#include "memory.h"

void* reallocate(void* previous, size_t oldSize, size_t newSize) {
    UNUSED(oldSize);
    if (newSize == 0) {
        free(previous);
        return NULL;
    }
    return realloc(previous, newSize);
}