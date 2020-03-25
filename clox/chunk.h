#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"
#include "table.h"

typedef enum {
    // Complex instructions (take arguments)
    
    // Pushes the nth constant onto the stack.
    // opcode index
    OP_CONSTANT,
    // Pushes the nth constant onto the stack. Used when there are more than
    // 255 constants. Similar long variaents exist for other constant-related
    // instructions.
    // little endian
    // opcode index1 index2 index3
    OP_CONSTANT_LONG,
    // Defines a global variable with the given constant name, popping a value
    // off of the stack to get its value
    // opcode nameIndex
    OP_DEFINE_GLOBAL,
    OP_DEFINE_GLOBAL_LONG,
    // Gets the value of a global variable and pushes it onto the stack
    // opcode nameIndex
    OP_GET_GLOBAL,
    OP_GET_GLOBAL_LONG,
    // Sets a global variable to the top value off of the stack, without
    // popping it off the stack
    // opcode nameIndex
    OP_SET_GLOBAL,
    OP_SET_GLOBAL_LONG,

    // Simple instructions (take no arguments)

    // Pushes the specified literal to the stack.
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    // Pops the top value of the stack without doing anything to it.
    OP_POP,
    // Pops the top two values from the stack, performs the specified
    // comparison, and pushes the resuling boolean on the stack.
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    // Pops the top two values from the stack, performs the specified operator,
    // and pushes them onto the stack. The left-side argument is the second
    // value popped, while the right-side argument in the first value popped.
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    // Takes the top value of the stack, determines if it's truthy,
    // and negates it.
    OP_NOT,
    // Negates the top value on the stack.
    OP_NEGATE,
    // Pops and prints the top value of the stack
    OP_PRINT,
    // Pops and returns the top value of the stack.
    OP_RETURN
} OpCode;

typedef struct {
    size_t count;
    size_t capacity;
    uint8_t* code; // The code
    size_t line_count;
    size_t line_capacity;
    // The number of symbols on each line
    // Index 0 stores the number of symbols on line 1,
    // index 1 stores the number of symbols on line 2,
    // and so on.
    uint16_t* lines;
    ValueArray constants;
#ifdef CLOX_CONST_CACHE
    Table constantTable;
#endif
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
int addConstant(Chunk* chunk, Value value);
int writeChunk(Chunk* chunk, uint8_t byte, size_t line);
// size_t addConstant(Chunk* chunk, Value value);
int writeConstant(Chunk* chunk, Value value, size_t line);
size_t getLine(Chunk* chunk, size_t index);

// Maximum 2**8-1 constants
#define CHUNK_SHORT_CONSTANTS 255
// Maximum 2**24-2 constants
#define CHUNK_LONG_CONSTANTS 16777214

#ifdef CLOX_LONG_CONSTANTS
#define CHUNK_MAX_CONSTANTS CHUNK_LONG_CONSTANTS
#else
#define CHUNK_MAX_CONSTANTS CHUNK_SHORT_CONSTANTS
#endif

#endif