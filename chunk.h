#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#define GROW_CAPACITY(capacity) capacity < 8? 8: capacity * 2

typedef enum {
    OP_RETURN
} OpCode;

typedef struct {
    int count;
    int capacity;    
    uint8_t* code;
} Chunk;

void initChunk(Chunk*);

void writeChunk(Chunk*, uint8_t);

void freeChunk(Chunk*);

#endif