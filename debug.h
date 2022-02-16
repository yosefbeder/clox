#ifndef clox_debug_h
#define clox_debug_h

#include "common.h"
#include "chunk.h"
#include "scanner.h"

void disassembleChunk(Chunk*);

void printToken(Token*);

#endif