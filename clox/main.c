#include <stdio.h>

#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

#define UNUSED(x) (void)(x)

VM vm;

int main(int argc, const char* argv[]) {
    // Suppress unused warnings
    UNUSED(argc);
    UNUSED(argv);

    initVM(&vm);
    
    Chunk chunk;
    initChunk(&chunk);

    writeConstant(&chunk, 1.2, 123);
    writeConstant(&chunk, 3.4, 123);

    writeChunk(&chunk, OP_ADD, 123);

    writeConstant(&chunk, 5.6, 123);

    writeChunk(&chunk, OP_DIVIDE, 123);
    writeChunk(&chunk, OP_NEGATE, 123);

    writeChunk(&chunk, OP_RETURN, 123);

    // disassembleChunk(&chunk, "test chunk");
    interpret(&vm, &chunk);
    freeVM(&vm);
    freeChunk(&chunk);
    return 0;
}