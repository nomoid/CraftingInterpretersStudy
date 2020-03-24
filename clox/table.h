#ifndef clox_table_h
#define clox_table_h

#include "common.h"
#include "value.h"

typedef struct {
    // Permitted key types:
    // Number (Integer and Float)
    // Nil
    // Boolean
    // ObjString*
    bool present;
    Value key;
    Value value;
} Entry;

typedef struct {
    size_t count;
    size_t capacity;
    Entry* entries;
} Table;

void initTable(Table* table);
void freeTable(Table* table);
bool tableGet(Table* table, Value key, Value* value);
bool tableSet(Table* table, Value key, Value value);
bool tableDelete(Table* table, Value key);
void tableAddAll(Table* src, Table* dest);
ObjString* tableFindString(Table* table, const char* chars,
    int length, uint32_t hash);

#endif