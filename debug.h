#ifndef clox_debug_h
#define clox_debug_h

#include "common.h"
#include "chunk.h"
#include "scanner.h"

char *opCodeToString(OpCode);

int disassembleInstruction(Chunk *, int);

void disassembleChunk(Chunk *, char *);

char *tokenTypeToString(TokenType);

void printToken(Token *);

#endif