#include "common.h"
#include "debug.h"
#include "chunk.h"
#include "value.h"

int main() {
    Chunk chunk;

    initChunk(&chunk);
    writeChunk(&chunk, OP_RETURN);

    uint8_t i = addConstant(&chunk, 32);

    writeChunk(&chunk, OP_CONSTANT);
    writeChunk(&chunk, i);
    diassembleChunk(&chunk);

    freeChunk(&chunk);
    return 0;
}