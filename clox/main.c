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

    uint8_t constant = addConstant(&chunk, 1.2);
    writeChunk(&chunk, OP_CONSTANT, 123);
    writeChunk(&chunk, constant, 123);

    writeChunk(&chunk, OP_RETURN, 123);

    disassembleChunk(&chunk, "test chunk");
    freeChunk(&chunk);
    return 0;
}