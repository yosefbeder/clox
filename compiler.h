#include "common.h"
#include "scanner.h"
#include "chunk.h"
#include "object.h"
#include "vm.h"

typedef struct
{
    Token name;
    int depth;
    bool captured;
} Local;

typedef struct
{
    bool local;
    uint8_t index;
} UpValue;

typedef enum
{
    TYPE_SCRIPT,
    TYPE_FUNCTION,
    TYPE_METHOD,
    TYPE_INITIALIZER,
} FunctionType;

typedef struct Compiler
{
    ObjFunction *function;
    FunctionType type;
    Local locals[UINT8_MAX + 1];
    uint8_t currentLocal;
    UpValue upValues[UINT8_MAX + 1];
    uint8_t currentUpValue;

    struct Compiler *enclosing;
    Scanner *scanner;

    Token previous;
    Token current;

    bool hadError;
    bool panicMode;
    bool canAssign;
    bool inFunGrouping;
    int groupingDepth;
    int stringDepth;
    int scopeDepth;
    int ternaryDepth;
    int loopStartIndex;
    int loopEndIndex;
} Compiler;

extern Compiler compiler;

ObjFunction *compile(Scanner *);