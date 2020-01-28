#include <stdio.h>

#include "debug.h"
#include "value.h"
#include "endian.h"

void disassembleChunk(Chunk* chunk, const char* name) {
    printf("== %s ==\n", name);

    for (size_t offset = 0; offset < chunk->count;) {
        offset = disassembleInstruction(chunk, offset);
    }
}

static size_t simpleInstruction(const char* name, size_t offset) {
    printf("%s\n", name);
    return offset + 1;
}

static size_t constantInstruction(const char* name, Chunk* chunk, size_t offset, bool longConstant) {
    size_t constant;
    size_t newOffset;
    if (longConstant) {
        constant = COMBINE_3WORD(
            chunk->code[offset + 1],
            chunk->code[offset + 2],
            chunk->code[offset + 3]
        );
        newOffset = offset + 4;
    }
    else {
        constant = chunk->code[offset + 1];
        newOffset = offset + 2;
    }
    printf("%-16s %4ld '", name, constant);
    printValue(chunk->constants.values[constant]);
    printf("'\n");
    return newOffset;
}

size_t disassembleInstruction(Chunk* chunk, size_t offset) {
    printf("%04ld ", offset);
    size_t thisLine = getLine(chunk, offset);
    if (offset > 0 && thisLine == getLine(chunk, offset - 1)) {
        printf("   | ");
    }
    else {
        printf("%4ld ", thisLine);
    }

    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        case OP_CONSTANT:
            return constantInstruction("OP_CONSTANT", chunk, offset, false);
        case OP_CONSTANT_LONG:
            return constantInstruction("OP_CONSTANT_LONG", chunk, offset, true);
        case OP_NIL:
            return simpleInstruction("OP_NIL", offset);
        case OP_TRUE:
            return simpleInstruction("OP_TRUE", offset);
        case OP_FALSE:
            return simpleInstruction("OP_FALSE", offset);
        case OP_EQUAL:
            return simpleInstruction("OP_EQUAL", offset);
        case OP_GREATER:
            return simpleInstruction("OP_GREATER", offset);
        case OP_LESS:
            return simpleInstruction("OP_LESS", offset);
        case OP_ADD:
            return simpleInstruction("OP_ADD", offset);
        case OP_SUBTRACT:
            return simpleInstruction("OP_SUBTRACT", offset);
        case OP_MULTIPLY:
            return simpleInstruction("OP_MULTIPLY", offset);
        case OP_DIVIDE:
            return simpleInstruction("OP_DIVIDE", offset);
        case OP_NOT:
            return simpleInstruction("OP_NOT", offset);
        case OP_NEGATE:
            return simpleInstruction("OP_NEGATE", offset);
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}