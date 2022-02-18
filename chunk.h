#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"
#include "scanner.h"

typedef enum {
    OP_RETURN,
    OP_CONSTANT,
    OP_NEGATE,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_OR,
    OP_AND,
    OP_EQUAL,
    OP_NOT_EQUAL,
    OP_GREATER,
    OP_GREATER_OR_EQUAL,
    OP_LESS,
    OP_LESS_OR_EQUAL,
    OP_BANG,
} OpCode;

typedef struct {
    int count;
    int capacity;
    Token* tokens;
} TokenArr;

typedef struct {
    int count;
    int capacity;    
    uint8_t* code;
    ValueArr constants;
    TokenArr tokenArr;
} Chunk;

void initChunk(Chunk*);

// take a reference and copy it inside
void writeChunk(Chunk*, uint8_t, Token);

uint8_t addConstant(Chunk*, Value);

void freeChunk(Chunk*);

#endif