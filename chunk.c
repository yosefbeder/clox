#include "chunk.h"
#include "memory.h"

void initTokenArr(TokenArr *tokenArr)
{
    tokenArr->capacity = 0;
    tokenArr->count = 0;
    tokenArr->tokens = NULL;
}

static void writeTokenArr(TokenArr *tokenArr, Token *token)
{
    if (tokenArr->count == tokenArr->capacity)
    {
        size_t oldCapacity = tokenArr->capacity;
        tokenArr->capacity = GROW_CAPACITY(tokenArr->capacity);

        tokenArr->tokens = GROW_ARRAY(Token, tokenArr->tokens, oldCapacity, tokenArr->capacity);
    }

    tokenArr->tokens[tokenArr->count++] = *token;
}

void initValueArr(ValueArr *valueArr)
{
    valueArr->count = 0;
    valueArr->capacity = 0;
    valueArr->values = NULL;
}

static void writeValueArr(ValueArr *valueArr, Value value)
{
    push(value); //? because in the next line chunk may grow up

    if (valueArr->count == valueArr->capacity)
    {
        size_t oldCapacity = valueArr->capacity;
        valueArr->capacity = GROW_CAPACITY(valueArr->capacity);
        valueArr->values = GROW_ARRAY(Value, valueArr->values, oldCapacity, valueArr->capacity);
    }

    valueArr->values[valueArr->count++] = value;
    pop();
}

void initChunk(Chunk *chunk)
{
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

void writeChunk(Chunk *chunk, uint8_t byte, Token *token)
{
    if (chunk->count == chunk->capacity)
    {
        size_t oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(chunk->capacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
    }

    chunk->code[chunk->count++] = byte;
    writeTokenArr(&chunk->tokenArr, token);
}

uint8_t addConstant(Chunk *chunk, Value value)
{

    writeValueArr(&chunk->constants, value);

    return chunk->constants.count - 1;
}
