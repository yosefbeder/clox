#include "chunk.h"
#include "memory.h"

void initChunk(Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;

    ValueArr constants;
    initValueArr(&constants);

    chunk->constants = constants;
}

void writeChunk(Chunk* chunk, uint8_t byte) {
    // check the capacity
    if (chunk->count == chunk->capacity) {
        chunk->capacity = GROW_CAPACITY(chunk->capacity);
        chunk->code = realloc(chunk->code, chunk->capacity * sizeof(char));

        if (chunk->code == NULL) {
            exit(1);
        }
    }

    // insert
    chunk->code[chunk->count++] = byte;
}

int addConstant(Chunk* chunk, double value) {
    writeValueArr(&chunk->constants, value);

    return chunk->constants.count - 1;
}

void freeChunk(Chunk* chunk) {
    free(chunk->code);
    freeValueArr(&chunk->constants);

    initChunk(chunk);
}