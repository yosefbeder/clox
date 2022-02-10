#include "chunk.h"

void initChunk(Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
}

void writeChunk(Chunk* chunk, uint8_t byte) {
    // check the capacity
    if (chunk->count == chunk->capacity) {
        if (chunk->code == NULL) {
            chunk->code = malloc(GROW_CAPACITY(chunk->capacity) * sizeof(char));
        } else {
            chunk->code = realloc(chunk->code, GROW_CAPACITY(chunk->capacity) * sizeof(char));

            if (chunk->code == NULL) {
                exit(1);
            }
        }
    }

    // insert
    chunk->code[chunk->count++] = byte;
}

void freeChunk(Chunk* chunk) {
    free(chunk->code);

    initChunk(chunk);
}