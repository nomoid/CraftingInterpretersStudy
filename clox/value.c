#include <stdlib.h>
#include <stdio.h>

#include "value.h"
#include "memory.h"

// Initializes an empty value array with capacity 0
void initValueArray(ValueArray* array) {
    array->count = 0;
    array->capacity = 0;
    array->values = NULL;
}

// Frees a value array, resetting its values to initialization
void freeValueArray(ValueArray* array) {
    FREE_ARRAY(Value, array->values, array->capacity);
    initValueArray(array);
}

// Adds a single Value to the value array
// Returns -1 for failed allocation, 0 otherwise
int writeValueArray(ValueArray* array, Value value) {
    if (array->capacity < array->count + 1) {
        size_t oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->values = GROW_ARRAY(array->values, Value,
            oldCapacity, array->capacity);
        if (array->values == NULL) {
            // Out of memory
            return -1;
        }
    }

    array->values[array->count] = value;
    array->count++;
    return 0;
}

// Prints a single value
void printValue(Value value) {
    printf("%g", value);
}