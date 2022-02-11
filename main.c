#include "common.h"
#include "debug.h"
#include "chunk.h"

int main() {
    Chunk chunk;

    initChunk(&chunk);

    uint8_t i1 = addConstant(&chunk, 2);
    uint8_t i2 = addConstant(&chunk, 2.5);
    uint8_t i3 = addConstant(&chunk, 5);

    writeChunk(&chunk, OP_CONSTANT, 1);
    writeChunk(&chunk, i1, 1);
    writeChunk(&chunk, OP_CONSTANT, 1);
    writeChunk(&chunk, i2, 1);

    writeChunk(&chunk, OP_MULTIPLY, 1);

    writeChunk(&chunk, OP_CONSTANT, 1);
    writeChunk(&chunk, i3, 1);

    writeChunk(&chunk, OP_ADD, 1);

    writeChunk(&chunk, OP_RETURN, 1);

    diassembleChunk(&chunk);

    freeChunk(&chunk);
    return 0;
}