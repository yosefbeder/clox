#include "chunk.h"
#include "memory.h"

void initTokenArr(TokenArr* tokenArr) {
    tokenArr->capacity = 0;
    tokenArr->count = 0;
    tokenArr->tokens = NULL;
}

void writeTokenArr(TokenArr* tokenArr, Token token) {
    if (tokenArr->count == tokenArr->capacity) {
        tokenArr->capacity = GROW_CAPACITY(tokenArr->capacity);
        tokenArr->tokens = realloc(tokenArr->tokens, tokenArr->capacity * sizeof(Token));

        if (tokenArr->tokens == NULL) {
            exit(1);
        }
    }

    tokenArr->tokens[tokenArr->count++] = token;
}

void freeTokenArr(TokenArr* tokenArr) {
    free(tokenArr->tokens);

    initTokenArr(tokenArr);
}

void initChunk(Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;

    ValueArr constants;
    initValueArr(&constants);

    chunk->constants = constants;

    TokenArr tokenArr;
    initTokenArr(&tokenArr);

    chunk->tokenArr = tokenArr;
}

void writeChunk(Chunk* chunk, uint8_t byte, Token token) {
    if (chunk->count == chunk->capacity) {
        chunk->capacity = GROW_CAPACITY(chunk->capacity);
        chunk->code = realloc(chunk->code, chunk->capacity * sizeof(char));

        if (chunk->code == NULL) {
            exit(1);
        }
    }

    chunk->code[chunk->count++] = byte;
    writeTokenArr(&chunk->tokenArr, token);
}

uint8_t addConstant(Chunk* chunk, double value) {
    writeValueArr(&chunk->constants, value);

    return chunk->constants.count - 1;
}

void freeChunk(Chunk* chunk) {
    free(chunk->code);
    freeValueArr(&chunk->constants);
    freeTokenArr(&chunk->tokenArr);

    initChunk(chunk);
}