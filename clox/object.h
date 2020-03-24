#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "value.h"
#include "table.h"

#define OBJ_TYPE(value)  (AS_OBJ(value)->type)

#define IS_STRING(value) isObjType(value, OBJ_STRING)

#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)

typedef struct {
    Obj* head;
} FreeList;

typedef enum {
    OBJ_STRING,
} ObjType;

struct sObj {
    ObjType type;
    struct sObj* next;
};

struct sObjString {
    Obj obj;
    int length;
    char* chars;
    uint32_t hash;
};

ObjString* takeString(FreeList* freeList,Table* strings, char* chars, int length);
ObjString* copyString(FreeList* freeList,Table* strings, const char* chars, int length);
void printObject(Value value);
void printObjectType(Value value);

static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif