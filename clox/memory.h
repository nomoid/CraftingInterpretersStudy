#ifndef clox_memory_h
#define clox_memory_h

#include <stdbool.h>

#include "object.h"

#define ALLOCATE(type, count, zero) \
    (type*)reallocate(NULL, 0, sizeof(type) * (size_t)(count), (zero))

#define FREE(type, pointer) \
    reallocate(pointer, sizeof(type), 0, false)

#define GROW_CAPACITY(capacity) \
    ((capacity < 8) ? 8 : (capacity) * 2);

#define GROW_ARRAY(previous, type, oldCount, count) \
    (type*)reallocate(previous, (size_t)(sizeof(type) * (oldCount)), \
        sizeof(type) * (count), false);

#define GROW_ARRAY_ZERO(previous, type, oldCount, count) \
    (type*)reallocate(previous, (size_t)(sizeof(type) * (oldCount)), \
        sizeof(type) * (count), true);

#define FREE_ARRAY(type, pointer, oldCount) \
    reallocate(pointer, (size_t)(sizeof(type) * (size_t)(oldCount)), 0, false);

void* reallocate(void* previous, size_t oldSize, size_t newSize, bool zero);
void freeObjects(FreeList* freeList);

#endif