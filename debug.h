#ifndef clox_debug_h
#define clox_debug_h

#include "common.h"
#include "chunk.h"

int diassembleInstruction(Chunk*, int);

void diassembleChunk(Chunk*);

#endif