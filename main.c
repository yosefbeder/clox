#include "common.h"
#include "debug.h"
#include "chunk.h"

int main() {
    Chunk chunk;

    initChunk(&chunk);
    writeChunk(&chunk, OP_RETURN, 1);

    uint8_t i = addConstant(&chunk, 32);

    writeChunk(&chunk, OP_CONSTANT, 1);
    writeChunk(&chunk, i, 1);
    writeChunk(&chunk, OP_CONSTANT, 2);
    writeChunk(&chunk, i, 2);
    writeChunk(&chunk, OP_CONSTANT, 3);
    writeChunk(&chunk, i, 3);
    writeChunk(&chunk, OP_CONSTANT, 3);
    writeChunk(&chunk, i, 3);
    writeChunk(&chunk, OP_CONSTANT, 3);
    writeChunk(&chunk, i, 3);
    diassembleChunk(&chunk);

    freeChunk(&chunk);
    return 0;
}