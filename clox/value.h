#ifndef clox_value_h
#define clox_value_h

#include "common.h"

typedef double Value;

typedef struct {
    size_t capacity;
    size_t count;
    Value* values;
} ValueArray;

void initValueArray(ValueArray* array);
void freeValueArray(ValueArray* array);
int writeValueArray(ValueArray* array, Value value);
void printValue(Value value);

#endif