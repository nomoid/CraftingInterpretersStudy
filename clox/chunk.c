#include <stdlib.h>

#include "chunk.h"
#include "memory.h"
#include "endian.h"

// Initializes an empty Chunk with capacity 0
void initChunk(Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->line_count = 0;
    chunk->line_capacity = 0;
    chunk->lines = NULL;
    initValueArray(&chunk->constants);
}

// Frees a Chunk, resetting its values to initialization
void freeChunk(Chunk* chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(int, chunk->lines, chunk->capacity);
    freeValueArray(&chunk->constants);
    initChunk(chunk);
}

// Adds a single byte to the Chunk's code
// Returns -1 for failed allocation, 0 otherwise
int writeChunk(Chunk* chunk, uint8_t byte, size_t line) {
    if (chunk->capacity < chunk->count + 1) {
        size_t oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(chunk->code, uint8_t,
            oldCapacity, chunk->capacity);
        if (chunk->code == NULL) {
            return -1;
        }
    }

    while (chunk->line_capacity < line) {
        size_t oldLineCapacity = chunk->line_capacity;
        chunk->line_capacity = GROW_CAPACITY(oldLineCapacity);
        chunk->lines = GROW_ARRAY_ZERO(chunk->lines, uint16_t,
            oldLineCapacity, chunk->line_capacity);
        if (chunk->lines == NULL) {
            return -1;
        }
    }

    chunk->code[chunk->count] = byte;
    chunk->lines[line - 1]++;
    chunk->count++;
    return 0;
}

// Adds a single constant to the Chunk
// Returns index where Value was added
// Returns -1 if failed to add
static size_t addConstant(Chunk* chunk, Value value) {
    // Maximum 2**24-2 constants
    if (chunk->constants.count >= 16777214) {
        return (size_t)-1;
    }
    if (writeValueArray(&chunk->constants, value) == -1) {
        return (size_t)-1;
    }
    return (size_t)(chunk->constants.count - 1);
}

// Writes a constant to the Chunk
// Combines addConstant and writeChunk
int writeConstant(Chunk* chunk, Value value, size_t line) {
    size_t index = addConstant(chunk, value);
    if (index == (size_t) -1) {
        return -1;
    }
    if (index >= 256) {
        writeChunk(chunk, OP_CONSTANT_LONG, line);
        for (int i = 0; i < 3; i++) {
            writeChunk(chunk, BYTE_FROM_3WORD(index, i), line);
        }
    }
    else {
        writeChunk(chunk, OP_CONSTANT, line);
        writeChunk(chunk, (uint8_t)index, line);
    }
    return 0;
}

size_t getLine(Chunk* chunk, size_t index) {
    size_t remaining = index;
    size_t line = 0;
    // Numerical overflow when remaining is negative
    while (remaining <= index) {
        remaining -= chunk->lines[line];
        line++;
    }
    return line;
}