#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "value.h"

#define ALLOCATE_OBJ(vm, type, objectType) \
    (type*)allocateObject(vm, sizeof(type), objectType)

static Obj* allocateObject(FreeList* freeList, size_t size, ObjType type) {
    Obj* object = (Obj*)reallocate(NULL, 0, size, true);
    object->type = type;

    object->next = freeList->head;
    freeList->head = object;

    return object;
}

static ObjString* allocateString(FreeList* freeList, char* chars, int length) {
    ObjString* string = ALLOCATE_OBJ(freeList, ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;

    return string;
}

ObjString* takeString(FreeList* freeList, char* chars, int length) {
    return allocateString(freeList, chars, length);
}

ObjString* copyString(FreeList* freeList, const char* chars, int length) {
    char* heapChars = ALLOCATE(char, length + 1, false);
    memcpy(heapChars, chars, (size_t)length);
    heapChars[length] = '\0';

    return allocateString(freeList, heapChars, length);
}

void printObject(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING:
            printf("%s", AS_CSTRING(value));
            break;
    }
}

void printObjectType(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING:
            printf("%-4s", "STR");
            break;
    }
}