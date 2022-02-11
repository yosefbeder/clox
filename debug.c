#include "debug.h"

void diassembleChunk(Chunk* chunk) {
    int i = 0;

    while (i < chunk->count - 1) {
        uint8_t byte = chunk->code[i];

        if(byte == OP_RETURN) {
            printf("%04d OP_RETURN\n", i);
            i++;
        }

        if (byte == OP_CONSTANT) {
            int constant_i = chunk->code[i + 1];
            printf("%04d OP_CONSTANT %d (%lf)\n", i, constant_i, chunk->constants.values[constant_i]);
            i += 2;
        }
    }
}