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
    printf("%-16s %4" FORMAT_SIZE_T " ", name, constant);
    Value value = chunk->constants.values[constant];
#ifdef DEBUG_PRINT_VALUE_TYPE
    printValueType(value);
#endif
    printf("'");
    printValue(value);
    printf("'\n");
    return newOffset;
}

static const char* opName(uint8_t opcode) {
    switch (opcode) {
        case OP_CONSTANT:
            return "OP_CONSTANT";
        case OP_CONSTANT_LONG:
            return "OP_CONSTANT_LONG";
        case OP_DEFINE_GLOBAL:
            return "OP_DEFINE_GLOBAL";
        case OP_DEFINE_GLOBAL_LONG:
            return "OP_DEFINE_GLOBAL_LONG";
        case OP_GET_GLOBAL:
            return "OP_GET_GLOBAL";
        case OP_GET_GLOBAL_LONG:
            return "OP_GET_GLOBAL_LONG";
        case OP_SET_GLOBAL:
            return "OP_SET_GLOBAL";
        case OP_SET_GLOBAL_LONG:
            return "OP_SET_GLOBAL_LONG";
        case OP_NIL:
            return "OP_NIL";
        case OP_TRUE:
            return "OP_TRUE";
        case OP_FALSE:
            return "OP_FALSE";
        case OP_POP:
            return "OP_POP";
        case OP_EQUAL:
            return "OP_EQUAL";
        case OP_GREATER:
            return "OP_GREATER";
        case OP_LESS:
            return "OP_LESS";
        case OP_ADD:
            return "OP_ADD";
        case OP_SUBTRACT:
            return "OP_SUBTRACT";
        case OP_MULTIPLY:
            return "OP_MULTIPLY";
        case OP_DIVIDE:
            return "OP_DIVIDE";
        case OP_NOT:
            return "OP_NOT";
        case OP_NEGATE:
            return "OP_NEGATE";
        case OP_PRINT:
            return "OP_PRINT";
        case OP_RETURN:
            return "OP_RETURN";
        default:
            return NULL;
    }
}

size_t disassembleInstruction(Chunk* chunk, size_t offset) {
    printf("%04" FORMAT_SIZE_T " ", offset);
    size_t thisLine = getLine(chunk, offset);
    if (offset > 0 && thisLine == getLine(chunk, offset - 1)) {
        printf("   | ");
    }
    else {
        printf("%4" FORMAT_SIZE_T " ", thisLine);
    }

    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        case OP_CONSTANT:
        case OP_DEFINE_GLOBAL:
        case OP_GET_GLOBAL:
        case OP_SET_GLOBAL:
            return constantInstruction(opName(instruction), chunk, offset, false);
        case OP_CONSTANT_LONG:
        case OP_DEFINE_GLOBAL_LONG:
        case OP_GET_GLOBAL_LONG:
        case OP_SET_GLOBAL_LONG:
            return constantInstruction(opName(instruction), chunk, offset, true);
        default: {
            const char* name = opName(instruction);
            if (name == NULL) {
                printf("Unknown opcode %d\n", instruction);
                return offset + 1;
            }
            return simpleInstruction(name, offset);
        }   
    }
}