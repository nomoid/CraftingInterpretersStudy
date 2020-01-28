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
    switch (value.type) {
        case VAL_BOOL: printf(AS_BOOL(value) ? "true" : "false"); break;
        case VAL_NIL: printf("nil"); break;
        case VAL_FLOAT: printf("%g", AS_FLOAT(value)); break;
#ifdef CLOX_INTEGER_TYPE
        case VAL_INT: printf("%ld", AS_INT(value)); break;
#endif
    }
}

#ifdef CLOX_INTEGER_TYPE
double numberToFloat(Value in) {
    double vfloat;
    if (IS_INT(in)) {
        vfloat = (double)AS_INT(in);
    }
    else {
        vfloat = AS_FLOAT(in);
    }
    return vfloat;
}
#endif

bool valuesEqual(Value a, Value b) {
    if (a.type != b.type) {
#ifdef CLOX_INTEGER_TYPE
        // Edge case: int and float equal
        if (IS_NUMBER(a) && IS_NUMBER(b)) {
            double vfloata = NUMBER_TO_FLOAT(a);
            double vfloatb = NUMBER_TO_FLOAT(b);
            return vfloata == vfloatb;
        }
        else {
            return false;
        }
#else
        return false;
#endif
    }

    switch (a.type) {
        case VAL_BOOL:  return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NIL:   return true;
        case VAL_FLOAT: return AS_FLOAT(a) == AS_FLOAT(b);
#ifdef CLOX_INTEGER_TYPE
        case VAL_INT:   return AS_INT(a) == AS_INT(b);
#endif
        default:
            return false; // Unreachable.
    }
}