#include <stdlib.h>

#include "chunk.h"
#include "memory.h"

// Initializes an empty Chunk with capacity 0
void initChunk(Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
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
int writeChunk(Chunk* chunk, uint8_t byte, int line) {
    if (chunk->capacity < chunk->count + 1) {
        size_t oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(chunk->code, uint8_t,
            oldCapacity, chunk->capacity);
        chunk->lines = GROW_ARRAY(chunk->lines, int,
            oldCapacity, chunk->capacity);
        if (chunk->code == NULL) {
            return -1;
        }
    }

    chunk->code[chunk->count] = byte;
    chunk->lines[chunk->count] = line;
    chunk->count++;
    return 0;
}

// Adds a single constant to the Chunk
// Returns index where Value was added
// Returns -1 if failed to add
uint8_t addConstant(Chunk* chunk, Value value) {
    // Maximum 254 constants
    if (chunk->constants.capacity >= 254) {
        return (uint8_t)-1;
    }
    writeValueArray(&chunk->constants, value);
    return (uint8_t)(chunk->constants.count - 1);
}