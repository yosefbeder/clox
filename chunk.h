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
} OpCode;

typedef struct {
    int count;
    int capacity;
    Token** tokens;
} TokenArr;

typedef struct {
    int count;
    int capacity;    
    uint8_t* code;
    ValueArr constants;
    TokenArr tokenArr;
} Chunk;

void initChunk(Chunk*);

void writeChunk(Chunk*, uint8_t, Token*);

int addConstant(Chunk*, double);

void freeChunk(Chunk*);

#endif