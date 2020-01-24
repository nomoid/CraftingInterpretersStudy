#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "value.h"

#define CLOX_VARIABLE_STACK
#define STACK_DEFAULT 256

#ifdef CLOX_VARIABLE_STACK
// Capacity in terms of number of Values it stores
#define STACK_POSITION(vm) \
    (size_t)((vm->stackTop)-(vm->stack))
#define STACK_CAPACITY(vm) \
    (size_t)((vm->stackMax)-(vm->stack))
#endif

typedef struct {
    Chunk* chunk;
    uint8_t* ip;
    Value* stackTop;
#ifdef CLOX_VARIABLE_STACK
    Value* stack;
    Value* stackMax;
#else
    Value stack[STACK_DEFAULT];
#endif
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

void initVM(VM *vm);
void freeVM(VM *vm);
InterpretResult interpret(VM *vm, Chunk* chunk);
void push(VM *vm, Value value);
Value pop(VM *vm);

#endif