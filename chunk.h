#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"
#include "line.h"

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
    uint8_t* code;
    ValueArr constants;
    LineArr lineArr;
} Chunk;

void initChunk(Chunk*);

void writeChunk(Chunk*, uint8_t, int);

int addConstant(Chunk*, double);

void freeChunk(Chunk*);

#endif