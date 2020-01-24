#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
    // opcode index
    OP_CONSTANT,
    // little endian
    // opcode index1 index2 index3
    OP_CONSTANT_LONG,
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