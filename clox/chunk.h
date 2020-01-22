#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
    // opcode index
    OP_CONSTANT,
    // opcode
    OP_RETURN,
} OpCode;

typedef struct {
    size_t count;
    size_t capacity;
    uint8_t* code;
    int *lines;
    ValueArray constants;
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
int writeChunk(Chunk* chunk, uint8_t byte, int line);
uint8_t addConstant(Chunk* chunk, Value value);

#endif