#ifndef clox_debug_h
#define clox_debug_h

#include "common.h"
#include "chunk.h"
#include "scanner.h"
#include "value.h"
#include "object.h"

char *opCodeToString(OpCode);

int disassembleInstruction(Chunk *, int);

void disassembleFunction(ObjFunction *);

char *tokenTypeToString(TokenType);

void printToken(Token *);

#endif