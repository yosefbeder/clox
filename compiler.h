#include "common.h"
#include "scanner.h"
#include "chunk.h"

typedef struct {
    Scanner* scanner;
    Chunk* chunk;
    Token previous;
    Token current;
    int hadError;
    int panicMode;
} Compiler;

void initCompiler(Compiler*, Scanner*, Chunk*);

int compile(Compiler*, int);