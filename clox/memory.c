#include <stdlib.h>
#include <string.h>

#include "memory.h"

void* reallocate(void* previous, size_t oldSize, size_t newSize, bool zero) {
    if (newSize == 0) {
        free(previous);
        return NULL;
    }
    void* alloc = realloc(previous, newSize);
    if (alloc == NULL) {
        // Out of memory
        return NULL;
    }
    if (newSize > oldSize && zero) {
        memset((void *)((size_t)alloc + oldSize), 0, newSize - oldSize);
    }
    return alloc;
}

static void freeObject(Obj* object) {
    switch (object->type) {
        case OBJ_STRING: {
            ObjString* string = (ObjString*)object;
            FREE_ARRAY(char, string->chars, string->length + 1);
            FREE(ObjString, object);
            break;
        }
    }
}

void freeObjects(FreeList* freeList) {
    Obj* object = freeList->head;
    while (object != NULL) {
        Obj* next = object->next;
        freeObject(object);
        object = next;
    }
    freeList->head = NULL;
}