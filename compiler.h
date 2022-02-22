#include "common.h"
#include "scanner.h"
#include "chunk.h"
#include "vm.h"

typedef struct {
    Scanner* scanner;
    Chunk* chunk;
    Vm* vm;
    Token previous;
    Token current;
    bool hadError;
    bool panicMode;
    int groupingDepth;
    int stringDepth;
} Compiler;

void initCompiler(Compiler*, Scanner*, Chunk*, Vm*);

bool compile(Compiler*, int);