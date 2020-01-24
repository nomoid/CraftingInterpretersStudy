#include <stdio.h>

#include "common.h"
#include "chunk.h"
#include "debug.h"

#define UNUSED(x) (void)(x)

int main(int argc, const char* argv[]) {
    // Suppress unused warnings
    UNUSED(argc);
    UNUSED(argv);
    
    Chunk chunk;
    initChunk(&chunk);

    for (int i = 0; i < 1000; i++) {
        writeConstant(&chunk, 1.2, 123);
    }

    writeChunk(&chunk, OP_RETURN, 123);

    // disassembleChunk(&chunk, "test chunk");
    freeChunk(&chunk);
    printf("Hello, world!\n");
    return 0;
}