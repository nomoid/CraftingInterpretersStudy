#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "compiler.h"
#include "vm.h"
#include "endian.h"
#include "debug.h"
#include "memory.h"

#define UNUSED(x) (void)(x)

static void resetStack(VM* vm) {
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

static void runtimeError(VM* vm, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t instruction = (size_t)(vm->ip - vm->chunk->code);
    size_t line = getLine(vm->chunk, instruction);
    fprintf(stderr, "[line %ld] in script\n", line);

    resetStack(vm);
}

void initVM(VM* vm) {
#ifdef CLOX_VARIABLE_STACK
    vm->stack = NULL;
#endif
    resetStack(vm);
}

void freeVM(VM* vm) {
#ifdef CLOX_VARIABLE_STACK
    size_t capacity = STACK_CAPACITY(vm);
    FREE_ARRAY(Value, vm->stack, capacity);
#else
    UNUSED(vm);
#endif
}


#ifdef CLOX_VARIABLE_STACK
static void growStack(VM* vm) {
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

void push(VM* vm, Value value) {
    *vm->stackTop = value;
    vm->stackTop++;
#ifdef CLOX_VARIABLE_STACK
    if (vm->stackTop == vm->stackMax) {
        growStack(vm);
    }
#endif
}

Value pop(VM* vm) {
    vm->stackTop--;
    return *vm->stackTop;
}

static Value peek(VM* vm, int distance) {
    return vm->stackTop[-1 - distance];
}

static Value negate(Value input) {
#ifdef CLOX_INTEGER_TYPE
    if (IS_FLOAT(input)) {
        return FLOAT_VAL(-AS_FLOAT(input));
    }
    else {
        return INT_VAL(-AS_INT(input));
    }
#else
    return FLOAT_VAL(-AS_FLOAT(input));
#endif
}

static bool isFalsey(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static InterpretResult run(VM* vm) {
#define READ_BYTE() (*vm->ip++)
#define READ_CONSTANT() (vm->chunk->constants.values[READ_BYTE()])
#define READ_CONSTANT_POS(pos) (vm->chunk->constants.values[(pos)])
    while (1) {
#define PUSH(value) (push(vm, (value)))
#define POP() (pop(vm))
#define PEEK(value) (peek(vm, (value)))
#ifdef CLOX_INTEGER_TYPE
    #define BINARY_OP(fn1, fn2, op) \
        do { \
            if (!IS_NUMBER(PEEK(0)) || !IS_NUMBER(PEEK(1))) { \
                runtimeError(vm, "Operands must be numbers."); \
                return INTERPRET_RUNTIME_ERROR; \
            } \
            Value b = POP(); \
            Value a = POP(); \
            if (IS_FLOAT(b) || IS_FLOAT(a)) { \
                PUSH(fn1( \
                    NUMBER_TO_FLOAT(a) op \
                    NUMBER_TO_FLOAT(b) \
                )); \
            } \
            else { \
                PUSH(fn2(AS_INT(a) op AS_INT(b))); \
            } \
        } while(false)
    #define BINARY_OP_NUMBER(op) BINARY_OP(FLOAT_VAL, INT_VAL, op)
    #define BINARY_OP_BOOL(op) BINARY_OP(BOOL_VAL, BOOL_VAL, op)
#else
    #define BINARY_OP(fn, op) \
    do { \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
            runtimeError(vm, "Operands must be numbers."); \
            return INTERPRET_RUNTIME_ERROR; \
        } \
        Value b = POP(); \
        Value a = POP(); \
        PUSH(fn(AS_FLOAT(a) op AS_FLOAT(b))); \
    } while(false)
    #define BINARY_OP_NUMBER(op) BINARY_OP(FLOAT_VAL, op)
    #define BINARY_OP_BOOL(op) BINARY_OP(BOOL_VAL, op)
#endif
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
            case OP_NIL:   PUSH(NIL_VAL); break;
            case OP_TRUE:  PUSH(BOOL_VAL(true)); break;
            case OP_FALSE: PUSH(BOOL_VAL(false)); break;
            case OP_EQUAL: {
                Value b = POP();
                Value a = POP();
                PUSH(BOOL_VAL(valuesEqual(a, b)));
                break;
            }
            case OP_GREATER:  BINARY_OP_BOOL(>); break;
            case OP_LESS:     BINARY_OP_BOOL(<); break;
            case OP_ADD:      BINARY_OP_NUMBER(+); break;
            case OP_SUBTRACT: BINARY_OP_NUMBER(-); break;
            case OP_MULTIPLY: BINARY_OP_NUMBER(*); break;
            case OP_DIVIDE:   BINARY_OP_NUMBER(/); break;
            case OP_NOT: PUSH(BOOL_VAL(isFalsey(POP()))); break;
            case OP_NEGATE:
                if (!IS_NUMBER(PEEK(0))) {
                    runtimeError(vm, "Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                PUSH(negate(POP())); break;
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

InterpretResult interpret(VM* vm, const char* source) {
    Chunk chunk;
    initChunk(&chunk);

    if (!compile(source, &chunk)) {
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }
    
    vm->chunk = &chunk;
    vm->ip = vm->chunk->code;

    InterpretResult result = run(vm);

    freeChunk(&chunk);
    return result;
}