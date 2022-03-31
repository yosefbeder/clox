#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"
#include "scanner.h"

typedef enum
{
    OP_RETURN,
    OP_CONSTANT,
    OP_NEGATE,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_EQUAL,
    OP_NOT_EQUAL,
    OP_GREATER,
    OP_GREATER_OR_EQUAL,
    OP_LESS,
    OP_LESS_OR_EQUAL,
    OP_BANG,
    OP_NIL,
    OP_DEFINE_GLOBAL,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
    OP_POP,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_JUMP_IF_FALSE,
    OP_JUMP_IF_TRUE,
    OP_JUMP,
    OP_JUMP_BACKWARDS,
    OP_CALL,
    OP_CLOSURE,
    OP_GET_UPVALUE,
    OP_SET_UPVALUE,
    OP_CLOSE_UPVALUE,
    OP_CLASS,
    OP_GET_PROPERTY,
    OP_SET_FIELD, // for setting static fields (on classes) and fields (on objects)
    OP_SET_METHOD,
    OP_SET_SUPER,
    OP_INVOKE,
} OpCode;

typedef struct
{
    size_t count;
    size_t capacity;
    Token *tokens;
} TokenArr;

typedef struct
{
    size_t count;
    size_t capacity;
    Value *values;
} ValueArr;

typedef struct
{
    size_t count;
    size_t capacity;
    uint8_t *code;
    ValueArr constants;
    TokenArr tokenArr;
} Chunk;

void initChunk(Chunk *);

void initTokenArr(TokenArr *);

void initValueArr(ValueArr *);

void writeChunk(Chunk *, uint8_t, Token *);

uint8_t addConstant(Chunk *, Value);

#endif