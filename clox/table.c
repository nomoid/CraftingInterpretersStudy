#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "table.h"
#include "memory.h"
#include "object.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

void initTable(Table* table) {
    table->count = 0;
    table->capacityCount = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void freeTable(Table* table) {
    FREE_ARRAY(Entry, table->entries, table->capacity);
    initTable(table);
}

static Entry* findEntry(Entry* entries, size_t capacity, Value key) {
    size_t index = hashValue(key) % capacity;
    Entry* tombstone = NULL;
    while (true) {
        Entry* entry = &entries[index];
        if (!entry->present) {
            if (IS_NIL(entry->value)) {
                return tombstone != NULL ? tombstone : entry;
            } else {
                if (tombstone == NULL) {
                    tombstone = entry;
                }
            }
        }
        else if (valuesEqual(entry->key, key)) {
            return entry;
        }

        index = (index + 1) % capacity;
    }
}

bool tableGet(Table* table, Value key, Value* value) {
    if (table->count == 0) {
        return false;
    }

    Entry* entry = findEntry(table->entries, table->capacity, key);
    if (!entry->present) {
        return false;
    }

    *value = entry->value;
    return true;
}

bool tableDelete(Table* table, Value key) {
    if (table->count == 0) {
        return false;
    }

    Entry* entry = findEntry(table->entries, table->capacity, key);

    if (!entry->present) {
        return false;
    }

    // Does not affect capacity count due to tombstone
    table->count--;
    entry->present = false;
    entry->key = NIL_VAL;
    entry->value = BOOL_VAL(true);
    return true;
}

void tableAddAll(Table* src, Table* dest) {
    for (size_t i = 0; i < src->capacity; i++) {
        Entry* entry = &src->entries[i];
        if (entry->present) {
            tableSet(dest, entry->key, entry->value);
        }
    }
}

ObjString* tableFindString(Table* table, const char* chars, int length,
        uint32_t hash) {
    if (table->count == 0) {
        return NULL;
    }

    size_t index = hash % table->capacity;

    while (true) {
        Entry* entry = &table->entries[index];

        if (!entry->present) {
            if(IS_NIL(entry->value)) {
                return NULL;
            }
        }
        else {
            // Must be used on table with only string keys
            ObjString* string = AS_STRING(entry->key);
            if (string->length == length &&
                    string->hash == hash &&
                    memcmp(string->chars, chars, (size_t) length) == 0) {
                return string;
            }
        } 

        index = (index + 1) % table->capacity;
    }
}

static void adjustCapacity(Table* table, size_t capacity) {
    Entry* entries = ALLOCATE(Entry, capacity, false);
    for (size_t i = 0; i < capacity; i++) {
        entries[i].present = false;
        entries[i].key = NIL_VAL;
        entries[i].value = NIL_VAL;
    }

    table->count = 0;
    table->capacityCount = 0;
    for (size_t i = 0; i < table->capacity; i++) {
        Entry* entry = &table->entries[i];
        if (!entry->present) {
            continue;
        }
        Entry* dest = findEntry(entries, capacity, entry->key);
        dest->present = true;
        dest->key = entry->key;
        dest->value = entry->value;
        table->count++;
        table->capacityCount++;
    }

    FREE_ARRAY(Entry, table->entries, table->capacity);
    table->entries = entries;
    table->capacity = capacity;
}

bool tableSet(Table* table, Value key, Value value) {
    if (table->capacityCount + 1 > (size_t)
            ((double)table->capacity * TABLE_MAX_LOAD)) {
        if (table->capacity > SIZE_MAX / GROW_CAPACITY_RATIO) {
            // Hash table out of memory
            // TODO handle error case
            exit(100);
        }
        size_t capacity = GROW_CAPACITY(table->capacity);
        adjustCapacity(table, capacity);
    }

    Entry* entry = findEntry(table->entries, table->capacity, key);

    bool isNewKey = !entry->present;
    if (isNewKey) {
        table->count++;
        // Only increment capacity on non-tombstone new keys
        if (IS_NIL(entry->value)) {
            table->capacityCount++;
        }
    }

    entry->present = true;
    entry->key = key;
    entry->value = value;
    
    return isNewKey;
}

void tablePrint(Table* table) {
    bool found = false;
    printf("{");
    for (size_t i = 0; i < table->capacity; i++) {
        Entry entry = table->entries[i];
        if (entry.present) {
            if (!found) {
                found = true;
            }
            else {
                printf(", ");
            }
            printValue(entry.key);
            printf(": ");
            printValue(entry.value);
        }
    }
    printf("}\n");
}

size_t tableSize(Table* table) {
    return table->count;
}