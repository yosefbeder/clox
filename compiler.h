#include "common.h"
#include "scanner.h"
#include "chunk.h"
#include "object.h"
#include "vm.h"

typedef struct {
    Token name;
    int depth;
} Local;

typedef enum {
    TYPE_SCRIPT,
    TYPE_FUNCTION,
} FunctionType;

typedef struct {
    ObjFunction* function;
    FunctionType type;

    Scanner* scanner;
    Vm* vm;
    Token previous;
    Token current;
    Local locals[UINT8_MAX + 1];
    uint8_t currentLocal;
    bool hadError;
    bool panicMode;
    bool canAssign;
    int groupingDepth;
    int stringDepth;
    int scopeDepth;
    int ternaryDepth;
    int loopStartIndex;
    int loopEndIndex;
} Compiler;

void initCompiler(Compiler*, Scanner*, Vm*, FunctionType);

ObjFunction* compile(Compiler*);