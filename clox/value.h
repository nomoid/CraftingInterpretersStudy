#ifndef clox_value_h
#define clox_value_h

#include "settings.h"
#include "common.h"

typedef struct sObj Obj;
typedef struct sObjString ObjString;

typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_FLOAT,
    VAL_OBJ,
#ifdef CLOX_INTEGER_TYPE
    VAL_INT,
#endif
} ValueType;

typedef struct {
    ValueType type;
    union {
        bool vbool;
        double vfloat;
        Obj* vobj;
#ifdef CLOX_INTEGER_TYPE
        int64_t vint;
#endif
    } as;
} Value;

#define BOOL_VAL(value)    ((Value){ VAL_BOOL, { .vbool = value } })
#define NIL_VAL            ((Value){ VAL_NIL, { .vfloat = 0 } })
#define FLOAT_VAL(value)   ((Value){ VAL_FLOAT, { .vfloat = value } })
#ifdef CLOX_INTEGER_TYPE
    #define INT_VAL(value) ((Value){ VAL_INT, { .vint = value } })
#endif
#define OBJ_VAL(object)    ((Value){ VAL_OBJ, { .vobj = (Obj*)object } })

#define AS_BOOL(value)     ((value).as.vbool)
#define AS_FLOAT(value)    ((value).as.vfloat)
#ifdef CLOX_INTEGER_TYPE
    #define AS_INT(value)  ((value).as.vint)
#endif
#define AS_OBJ(value)      ((value).as.vobj)

#define IS_BOOL(value)     ((value).type == VAL_BOOL)
#define IS_NIL(value)      ((value).type == VAL_NIL)
#define IS_FLOAT(value)    ((value).type == VAL_FLOAT)
#ifdef CLOX_INTEGER_TYPE
    #define IS_INT(value)  ((value).type == VAL_INT)
#else
    #define IS_INT(value)  (false)
#endif
#define IS_NUMBER(value)   (IS_FLOAT(value) || IS_INT(value))
#define IS_OBJ(value)      ((value).type == VAL_OBJ)

typedef struct {
    size_t capacity;
    size_t count;
    Value* values;
} ValueArray;

bool valuesEqual(Value a, Value b);
void initValueArray(ValueArray* array);
void freeValueArray(ValueArray* array);
int writeValueArray(ValueArray* array, Value value);
void printValue(Value value);
void printValueType(Value value);

#ifdef CLOX_INTEGER_TYPE
    double numberToFloat(Value in);
    #define NUMBER_TO_FLOAT(value) (numberToFloat(value))
#else
    #define NUMBER_TO_FLOAT(value) (value)
#endif

#endif