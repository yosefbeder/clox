#include "debug.h"

void diassembleChunk(Chunk* chunk) {
    int i = 0;

    while (i < chunk->count - 1) {
        uint8_t byte = chunk->code[i];
        int line = getLine(&chunk->lineArr, i);
        int prevLine = getLine(&chunk->lineArr, i - 1);
    
        printf("%04d | ", i);

        if (i == 0 || prevLine != line) {
            printf("%04d | ", line);    
        } else {
            printf("//// | ");
        }

        if(byte == OP_RETURN) {
            printf("OP_RETURN\n");
            i++;
        }

        if (byte == OP_CONSTANT) {
            int constant_i = chunk->code[i + 1];

            printf("OP_CONSTANT %d (%lf)\n", constant_i, chunk->constants.values[constant_i]);
            i += 2;
        }

        if (byte == OP_NEGATE) {
            printf("OP_NEGATE\n");
            i++;
        }
        if (byte == OP_ADD) {
            printf("OP_ADD\n");
            i++;
        }
        if (byte == OP_SUBTRACT) {
            printf("OP_SUBTRACT\n");
            i++;
        }
        if (byte == OP_MULTIPLY) {
            printf("OP_MULTIPLY\n");
            i++;
        }
        if (byte == OP_DEVIDE) {
            printf("OP_DEVIDE\n");
            i++;
        }
    }
}