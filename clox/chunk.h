#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
    // Pushes the nth constant onto the stack.
    // opcode index
    OP_CONSTANT,
    // Pushes the nth constant onto the stack. Used when there are more than
    // 255 constants.
    // little endian
    // opcode index1 index2 index3
    OP_CONSTANT_LONG,
    // Pops the top two values from the stack, performs the specified operator,
    // and pushes them onto the stack. The left-side argument is the second
    // value popped, while the right-side argument in the first value popped.
    // opcode
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    // Negates the top value on the stack.
    // opcode
    OP_NEGATE,
    // Pops and returns the top value of the stack.
    // opcode
    OP_RETURN,
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
    uint16_t *lines;
    ValueArray constants;
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
int writeChunk(Chunk* chunk, uint8_t byte, size_t line);
// size_t addConstant(Chunk* chunk, Value value);
int writeConstant(Chunk* chunk, Value value, size_t line);
size_t getLine(Chunk* chunk, size_t index);

#endif