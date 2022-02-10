#include "debug.h"

int diassembleInstruction(Chunk* chunk, int offset) {
    if (chunk->code[offset] == OP_RETURN) {
        printf("%04d OP_RETURN\n", offset);
        return offset + 1;
    }
}

void diassembleChunk(Chunk* chunk) {
    for (int i = 0; i < chunk->count; i = diassembleInstruction(chunk, i));
}