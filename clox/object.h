#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "value.h"

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
};

ObjString* takeString(FreeList* freeList, char* chars, int length);
ObjString* copyString(FreeList* freeList, const char* chars, int length);
void printObject(Value value);
void printObjectType(Value value);

static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif