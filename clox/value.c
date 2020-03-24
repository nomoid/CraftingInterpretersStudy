#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include "object.h"
#include "value.h"
#include "memory.h"
#include "math.h"

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
        case VAL_OBJ: printObject(value); break;
#ifdef CLOX_INTEGER_TYPE
        case VAL_INT: printf("%" PRId64, AS_INT(value)); break;
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
        case VAL_OBJ:   return AS_OBJ(a) == AS_OBJ(b);
#ifdef CLOX_INTEGER_TYPE
        case VAL_INT:   return AS_INT(a) == AS_INT(b);
#endif
        default:
            return false; // Unreachable.
    }
}

void printValueType(Value value) {
    switch (value.type) {
        case VAL_BOOL:  printf("%-8s", "BOOL"); break;
        case VAL_NIL:   printf("%-8s", "NIL"); break;
        case VAL_FLOAT: printf("%-8s", "FLOAT"); break;
        case VAL_OBJ:   { 
            printf("%s", "OBJ-");
            printObjectType(value);
            break;
        }
#ifdef CLOX_INTEGER_TYPE
        case VAL_INT:   printf("%-8s", "INT"); break;
#endif
    }
}

uint32_t hashString(const char* key, int length) {
    uint32_t hash = 2166136261u;

    for (int i = 0; i < length; i++) {
        uint32_t c = (uint32_t) key[i];
        hash = hash ^ c;
        hash *= 16777619;
    }

    return hash;
}

bool initHash = false;
uint32_t hashNil = 0;
uint32_t hashFalse = 0;
uint32_t hashTrue = 0;

uint32_t hashInt(vint_t number) {
    return hashString((const char*) &number, VINT_SIZE);
}

uint32_t hashValue(Value value) {
    if (!initHash) {
        initHash = true;
        hashNil = hashInt(hashInt(0) + 1);
        hashFalse = hashInt(hashInt(1) + 1);
        hashTrue = hashInt(hashInt(2) + 1);
    }
    switch (value.type) {
        case VAL_BOOL: {
            bool b = AS_BOOL(value);
            if (b) {
                return hashFalse;
            }
            else {
                return hashTrue;
            }
        }
        case VAL_NIL:
            return hashNil;
        case VAL_FLOAT: {
            double d = AS_FLOAT(value);
            // Cast raw bits
            vint_t i = *((vint_t *)&d);
            return hashInt(i ^ (hashNil + 1));
        }
        case VAL_OBJ:   {
            if (IS_STRING(value)) {
                ObjString* string = AS_STRING(value);
                return string->hash;
            }
            else {
                // TODO add support for other object types
                Obj* obj = AS_OBJ(value);
                // Cast address into number as hash
                return hashInt((vint_t)obj);
            }
        }
#ifdef CLOX_INTEGER_TYPE
        case VAL_INT: {
            vint_t i = AS_INT(value);
            return hashInt(i);
        }
#endif
        default:
            return false; // Unreachable.
    }
}