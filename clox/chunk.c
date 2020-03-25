#include <stdlib.h>

#include "settings.h"
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
#ifdef CLOX_CONST_CACHE
    initTable(&chunk->constantTable);
#endif
}

// Frees a Chunk, resetting its values to initialization
void freeChunk(Chunk* chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(int, chunk->lines, chunk->capacity);
    freeValueArray(&chunk->constants);
#ifdef CLOX_CONST_CACHE
    freeTable(&chunk->constantTable);
#endif
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
int addConstant(Chunk* chunk, Value value) {
#ifdef CLOX_CONST_CACHE
    Value idValueGet;
    if (tableGet(&chunk->constantTable, value, &idValueGet)) {
#ifdef CLOX_INTEGER_TYPE
        int index = (int)(AS_INT(idValueGet));
#else
        int index = (int)(AS_FLOAT(idValueGet));
#endif
        return index;
    }
#endif
    if (chunk->constants.count > CHUNK_MAX_CONSTANTS) {
        return -1;
    }
    ERROR_GUARD(writeValueArray(&chunk->constants, value));
    int constantId = (int)chunk->constants.count - 1;
#ifdef CLOX_CONST_CACHE
#ifdef CLOX_INTEGER_TYPE
    Value idValue = INT_VAL(constantId);
#else
    // Not recommended: Potential issues with large floats when using long
    // constants?
    Value idValue = FLOAT_VAL((double)constantId);
#endif
    tableSet(&chunk->constantTable, value, idValue);
#endif
    return constantId;
}

// Writes a constant to the Chunk
// Combines addConstant and writeChunk
int writeConstant(Chunk* chunk, Value value, size_t line) {
    int index = addConstant(chunk, value);
    ERROR_GUARD(index);
#ifdef CLOX_LONG_CONSTANTS
    if (index > CHUNK_SHORT_CONSTANTS) {
        ERROR_GUARD(writeChunk(chunk, OP_CONSTANT_LONG, line));
        for (int i = 0; i < 3; i++) {
            ERROR_GUARD(writeChunk(chunk, BYTE_FROM_3WORD(index, i), line));
        }
    }
#else
    if (index > CHUNK_SHORT_CONSTANTS) {
        // Out of constants
        return -1;
    }
#endif
    else {
        ERROR_GUARD(writeChunk(chunk, OP_CONSTANT, line));
        ERROR_GUARD(writeChunk(chunk, (uint8_t)index, line));
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