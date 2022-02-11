#include "chunk.h"
#include "memory.h"

void initChunk(Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;

    ValueArr constants;
    initValueArr(&constants);

    chunk->constants = constants;

    LineArr lineArr;
    initLineArr(&lineArr);

    chunk->lineArr = lineArr;
}

void writeChunk(Chunk* chunk, uint8_t byte, int line) {
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
    writeLineArr(&chunk->lineArr, line);
}

int addConstant(Chunk* chunk, double value) {
    writeValueArr(&chunk->constants, value);

    return chunk->constants.count - 1;
}

void freeChunk(Chunk* chunk) {
    free(chunk->code);
    freeValueArr(&chunk->constants);
    freeLineArr(&chunk->lineArr);

    initChunk(chunk);
}