#include "common.h"
#include "scanner.h"
#include "chunk.h"

typedef struct {
    Scanner* scanner;
    Chunk* chunk;
    Token previous;
    Token current;
    bool hadError;
    bool panicMode;
    int groupingDepth;
    int stringDepth;
} Compiler;

void initCompiler(Compiler*, Scanner*, Chunk*);

bool compile(Compiler*, int);