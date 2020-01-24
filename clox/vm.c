#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "vm.h"
#include "endian.h"
#include "debug.h"
#include "memory.h"

#define UNUSED(x) (void)(x)

static void resetStack(VM *vm) {
#ifdef CLOX_VARIABLE_STACK
    FREE_ARRAY(Value, vm->stack, STACK_CAPACITY(vm));
    vm->stack = GROW_ARRAY(NULL, Value, 0, STACK_DEFAULT);
    if (vm->stack == NULL) {
        // Out of memory
        // TODO handle error case
        exit(100);
    }
    vm->stackMax = vm->stack + STACK_DEFAULT;
#endif
    vm->stackTop = vm->stack;
}

void initVM(VM *vm) {
#ifdef CLOX_VARIABLE_STACK
    vm->stack = NULL;
#endif
    resetStack(vm);
}

void freeVM(VM *vm) {
#ifdef CLOX_VARIABLE_STACK
    size_t capacity = STACK_CAPACITY(vm);
    FREE_ARRAY(Value, vm->stack, capacity);
#else
    UNUSED(vm);
#endif
}


#ifdef CLOX_VARIABLE_STACK
static void growStack(VM *vm) {
    size_t position = STACK_POSITION(vm);
    size_t capacity = STACK_CAPACITY(vm);
    size_t newCapacity = GROW_CAPACITY(capacity);
    vm->stack = GROW_ARRAY(vm->stack, Value, capacity, newCapacity);
    if (vm->stack == NULL) {
        // Out of memory
        // TODO handle error case
        exit(100);
    }
    vm->stackTop = vm->stack + position;
    vm->stackMax = vm->stack + newCapacity;
}
#endif

void push(VM *vm, Value value) {
    *vm->stackTop = value;
    vm->stackTop++;
#ifdef CLOX_VARIABLE_STACK
    if (vm->stackTop == vm->stackMax) {
        growStack(vm);
    }
#endif
}

Value pop(VM *vm) {
    vm->stackTop--;
    return *vm->stackTop;
}

static InterpretResult run(VM *vm) {
#define READ_BYTE() (*vm->ip++)
#define READ_CONSTANT() (vm->chunk->constants.values[READ_BYTE()])
#define READ_CONSTANT_POS(pos) (vm->chunk->constants.values[(pos)])
    while (1) {
#define PUSH(value) (push(vm, (value)))
#define POP() (pop(vm))
#define BINARY_OP(op) \
    do { \
        double b = POP(); \
        double a = POP(); \
        PUSH(a op b); \
    } while(false)
#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for (Value* slot = vm->stack; slot < vm->stackTop; slot++) {
            printf("[ ");
            printValue(*slot);
            printf(" ]");
        }
        printf("\n");
        disassembleInstruction(vm->chunk, (size_t)(vm->ip - vm->chunk->code));
#endif
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                PUSH(constant);
                break;
            }
            case OP_CONSTANT_LONG: {
                uint8_t v1 = READ_BYTE();
                uint8_t v2 = READ_BYTE();
                uint8_t v3 = READ_BYTE();
                Value constant = READ_CONSTANT_POS(
                    COMBINE_3WORD(v1, v2, v3));
                PUSH(constant);
                break;
            }
            case OP_ADD:      BINARY_OP(+); break;
            case OP_SUBTRACT: BINARY_OP(-); break;
            case OP_MULTIPLY: BINARY_OP(*); break;
            case OP_DIVIDE:   BINARY_OP(/); break;
            case OP_NEGATE:   PUSH(-POP()); break;
            case OP_RETURN: {
                printValue(POP());
                printf("\n");
                return INTERPRET_OK;
            }
        }
    }
#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_CONSTANT_POS
#undef PUSH
#undef POP
#undef BINARY_OP
}

InterpretResult interpret(VM *vm, Chunk *chunk) {
    vm->chunk = chunk;
    vm->ip = vm->chunk->code;
    return run(vm);
}