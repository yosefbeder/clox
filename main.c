#include "common.h"
#include "debug.h"
#include "chunk.h"
#include "vm.h"

int main() {
    Chunk chunk;
    initChunk(&chunk);

    uint8_t i1 = addConstant(&chunk, 1);
    uint8_t i2 = addConstant(&chunk, 2);
    uint8_t i3 = addConstant(&chunk, 3);
    uint8_t i4 = addConstant(&chunk, 4);
    uint8_t i5 = addConstant(&chunk, 5);

    // 1 + 2 * 3 - 4 / -5

    writeChunk(&chunk, OP_CONSTANT, 1);
    writeChunk(&chunk, i1, 1);
    writeChunk(&chunk, OP_CONSTANT, 1);
    writeChunk(&chunk, i2, 2);
    writeChunk(&chunk, OP_CONSTANT, 1);
    writeChunk(&chunk, i3, 3);

    writeChunk(&chunk, OP_MULTIPLY, 1);

    writeChunk(&chunk, OP_ADD, 1);

    writeChunk(&chunk, OP_CONSTANT, 1);
    writeChunk(&chunk, i4, 4);
    writeChunk(&chunk, OP_CONSTANT, 1);
    writeChunk(&chunk, i5, 5);

    writeChunk(&chunk, OP_NEGATE, 1);

    writeChunk(&chunk, OP_DIVIDE, 1);

    writeChunk(&chunk, OP_SUBTRACT, 1);

    writeChunk(&chunk, OP_RETURN, 1);

    diassembleChunk(&chunk);

    Vm vm;
    initVm(&vm);

    runChunk(&vm, &chunk);

    freeChunk(&chunk);
    return 0;
}