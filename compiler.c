#include "compiler.h"
#include "reporter.h"
#include <string.h>
#include "object.h"
#include "debug.h"

static void errorAt(Token *, char[]);

static void warningAt(Token *, char[]);

static void emitByte(uint8_t, Token *);

static void emitConstant(Value, Token *);

static void emitNumber(char *, Token *);

static void emitIdentifier(char *, int, Token *);

static void emitString(char *, int, Token *);

static int emitJump(OpCode, Token *);

static void patchJump(int);

static void emitReturn(Token *);

static void getPrefixBP(int[2], TokenType);

static void getInfixBP(int[2], TokenType);

static void advance(void);

static void initCompiler(Scanner *, FunctionType, Compiler *);

static void consume(TokenType, char[]);

static Token peek(void);

static Token next(void);

static bool check(TokenType);

static bool atEnd(void);

static bool match(TokenType);

static bool sameIdentifier(Token *, Token *);

static int resolveVariable(Compiler *, Token *);

static uint8_t addUpValue(bool, uint8_t);

static int resolveUpValue(Compiler *, Token *);

static void expression(int);

static void startScope(void);

static void endScope(void);

static void block(void);

static void ifStatement(void);

static void whileStatement(void);

static void continueStatement(void);

static void returnStatement(void);

static void expressionStatement(void);

static void statement(void);

static void defineVariable(Token *, Token *);

static void varDeclaration(void);

static void funDeclaration(void);

static void declaration(void);

static void synchronize(void);

ObjFunction *compile(Scanner *);

Compiler compiler;

static void errorAt(Token *token, char msg[])
{
    if (compiler.panicMode)
        return; // We avoid throwing meaningless errors until we recover

    compiler.panicMode = true;

    Compiler *curCompiler = &compiler;

    while (curCompiler != NULL)
    {
        curCompiler->hadError = true;
        curCompiler = curCompiler->enclosing;
    }

    report(token->type == TOKEN_ERROR ? REPORT_SCAN_ERROR : REPORT_PARSE_ERROR, token, msg);
};

static void warningAt(Token *token, char msg[])
{
    if (compiler.panicMode)
        return; // We avoid throwing meaningless errors until we recover

    report(REPORT_WARNING, token, msg);
}

static void emitByte(uint8_t byte, Token *token)
{
    writeChunk(&compiler.function->chunk, byte, token);
}

static void emitBytes(uint8_t byte1, uint8_t byte2, Token *token)
{
    emitByte(byte1, token);
    emitByte(byte2, token);
}

static void emitConstant(Value value, Token *token)
{
    uint8_t i = addConstant(&compiler.function->chunk, value);

    if (i > UINT8_MAX)
    {
        errorAt(&compiler.previous, "Too many constants in one chunk");

        return;
    }

    emitByte(i, token);
};

static void emitNumber(char *s, Token *token)
{
    double value = strtod(s, NULL);

    emitByte(OP_CONSTANT, token);
    emitConstant(NUMBER(value), token);
}

static void emitIdentifier(char *s, int length, Token *token)
{
    ObjString *identifier = allocateObjString(s, length);

    emitConstant(OBJ(identifier), token);
}

static void emitString(char *s, int length, Token *token)
{
    ObjString *objString = allocateObjString(s, length);

    emitByte(OP_CONSTANT, token);
    emitConstant(OBJ(objString), token);
}

static int emitJump(OpCode type, Token *token)
{
    emitBytes(type, (uint8_t)1, token);

    return compiler.function->chunk.count - 1;
}

static void patchJump(int index)
{
    int value = compiler.function->chunk.count - index;

    if (value > UINT8_MAX)
    {
        errorAt(&compiler.function->chunk.tokenArr.tokens[index], "Too many code to jump over!");
    }

    compiler.function->chunk.code[index] = value;
}

static void emitReturn(Token *token)
{
    emitBytes(OP_NIL, OP_RETURN, token);
}

/*
    = -> [2, 1]
    ?: -> [4, 3]
    or -> [5, 6]
    and -> [7, 8]
    == != -> [9, 10]
    < > <= >= -> [11, 12]
    + - -> [13, 14]
    * / -> [15, 16]
    (unary) - ! -> [?, 17];
    . () -> [18, ?];
*/

static void getPrefixBP(int bp[2], TokenType type)
{
    switch (type)
    {
    case TOKEN_MINUS:
    case TOKEN_BANG:
        bp[1] = 17;
        break;
    default:;
    }
}

static void getInfixBP(int bp[2], TokenType type)
{
    switch (type)
    {
    case TOKEN_EQUAL:
        bp[0] = 2;
        bp[1] = 1;
        break;
    case TOKEN_QUESTION_MARK:
        bp[0] = 4;
        bp[1] = 3;
        break;
    case TOKEN_OR:
        bp[0] = 5;
        bp[1] = 6;
        break;
    case TOKEN_AND:
        bp[0] = 7;
        bp[1] = 8;
        break;
    case TOKEN_EQUAL_EQUAL:
    case TOKEN_BANG_EQUAL:
        bp[0] = 9;
        bp[1] = 10;
        break;
    case TOKEN_GREATER:
    case TOKEN_GREATER_EQUAL:
    case TOKEN_LESS:
    case TOKEN_LESS_EQUAL:
        bp[0] = 11;
        bp[1] = 12;
        break;
    case TOKEN_PLUS:
    case TOKEN_MINUS:
        bp[0] = 13;
        bp[1] = 14;
        break;
    case TOKEN_STAR:
    case TOKEN_SLASH:
        bp[0] = 15;
        bp[1] = 16;
    case TOKEN_LEFT_PAREN:
        bp[0] = 18;
        break;
    default:;
    }
}

static void advance()
{
    compiler.previous = compiler.current;

    while (true)
    {
        compiler.current = scanToken(compiler.scanner);

        if (compiler.current.type != TOKEN_ERROR)
            break;

        errorAt(&compiler.current, compiler.current.errorMsg);
    }
}

static void initCompiler(Scanner *scanner, FunctionType type, Compiler *enclosing)
{
    compiler.function = NULL;
    compiler.type = type;
    compiler.currentLocal = 0;
    compiler.currentUpValue = 0;

    compiler.enclosing = enclosing;
    compiler.function = allocateObjFunction();
    compiler.scanner = scanner;

    compiler.hadError = false;
    compiler.panicMode = false;
    compiler.canAssign = true;
    compiler.inFunGrouping = false;
    compiler.groupingDepth = 0;
    compiler.stringDepth = 0;
    compiler.scopeDepth = 0;
    compiler.ternaryDepth = 0;
    compiler.loopStartIndex = -1;
    compiler.loopEndIndex = -1;

    if (type == TYPE_SCRIPT)
    {
        Local *mainSlot = &compiler.locals[compiler.currentLocal++]; // We do this because the first stack slot in the vm points to the <script> fun (the one produced by this function)
        mainSlot->name.start = "";
        mainSlot->name.length = 0;
        mainSlot->depth = 0;
        mainSlot->captured = false;
    }
}

static void consume(TokenType type, char msg[])
{
    if (compiler.current.type == type)
    {
        advance();
        return;
    }

    errorAt(&compiler.current, msg);
}

static Token peek(void)
{
    return compiler.current;
}

static Token next(void)
{
    advance();

    return compiler.previous;
}

static bool check(TokenType type)
{
    return peek().type == type;
}

static bool atEnd(void)
{
    return check(TOKEN_EOF);
}

static bool match(TokenType type)
{
    if (check(type))
    {
        advance();
        return true;
    }

    return false;
}

// compares two identifier tokens
static bool sameIdentifier(Token *token1, Token *token2)
{
    return token1->length == token2->length & strncmp(token1->start, token2->start, token1->length) == 0;
}

static int resolveLocal(Compiler *compiler, Token *token)
{
    for (int i = compiler->currentLocal - 1; i >= 0; i--)
    {
        Local local = compiler->locals[i];

        if (sameIdentifier(token, &local.name))
        {
            return i;
        }
    }

    return -1;
}

static uint8_t addUpValue(bool local, uint8_t index)
{
    if (compiler.currentUpValue == UINT8_MAX)
        errorAt(&compiler.previous, "Too many closed-over variables");

    // check if it's already captured
    for (int i = 0; i < compiler.currentUpValue; i++)
    {
        UpValue upValue = compiler.upValues[i];

        if (upValue.local == local && upValue.index == index)
        {
            return i;
        }
    }

    UpValue upValue = {local, index};

    int i = compiler.currentUpValue++;

    compiler.upValues[i] = upValue;

    if (local)
    {
        compiler.enclosing->locals[index].captured = true;
    }

    return i;
}

// walks through the locals of parent functions
// then emits an upvalue to the compiler then
// returns its index (or -1 if not found)
static int resolveUpValue(Compiler *compiler, Token *token)
{
    if (compiler->enclosing == NULL)
        return -1;

    int local = resolveLocal(compiler->enclosing, token);

    if (local != -1)
        return addUpValue(true, local);

    int upValue = resolveUpValue(compiler->enclosing, token);

    if (upValue != -1)
        return addUpValue(false, upValue);

    return -1;
}

static void expression(int minBP)
{
    Token token = next();

    switch (token.type)
    {
    case TOKEN_IDENTIFIER:
    {
        int arg;
        OpCode opCode;
        bool setter = false;

        if (match(TOKEN_EQUAL))
        {
            Token equal = compiler.previous;
            setter = true;

            if (compiler.canAssign)
            {
                int bp[2];
                getInfixBP(bp, equal.type);

                expression(bp[1]);
            }
            else
                errorAt(&equal, "Bad assignment target");
        }

        if ((arg = resolveLocal(&compiler, &token)) != -1)
        {
            opCode = setter ? OP_ASSIGN_LOCAL : OP_GET_LOCAL;
        }
        else if ((arg = resolveUpValue(&compiler, &token)) != -1)
        {
            opCode = setter ? OP_ASSIGN_UPVALUE : OP_GET_UPVALUE;
        }
        else
        {
            opCode = setter ? OP_ASSIGN_GLOBAL : OP_GET_GLOBAL;
        }

        writeChunk(&compiler.function->chunk, opCode, &token);

        if (arg != -1)
        {
            writeChunk(&compiler.function->chunk, (uint8_t)arg, &token);
        }
        else
        {
            emitIdentifier(token.start, token.length, &token);
        }

        break;
    }
    case TOKEN_LEFT_PAREN:
        compiler.canAssign = true;
        compiler.groupingDepth++;
        expression(0);
        consume(TOKEN_RIGHT_PAREN, "Expected ')' after the group");
        compiler.groupingDepth--;
        break;
    case TOKEN_NUMBER:
        compiler.canAssign = false;
        emitNumber(token.start, &token);
        break;
    case TOKEN_STRING:
        compiler.canAssign = false;
        emitString(token.start + 1, token.length - 2, &token);
        break;
    case TOKEN_TEMPLATE_HEAD:
    {
        compiler.canAssign = false;
        compiler.stringDepth++;

        // emit the head
        emitString(token.start + 1, token.length - 3, &token);

        // emit the expression
        expression(0);

        // add them
        emitByte(OP_ADD, &token);

        // emitting the middle
        while (check(TOKEN_TEMPLATE_MIDDLE))
        {
            Token token = next();

            // emit the string
            emitString(token.start + 1, token.length - 3, &token);

            // emit the expression
            expression(0);

            // add them both
            emitByte(OP_ADD, &token);

            // add this to the head
            emitByte(OP_ADD, &token);
        }

        // emitting the tail
        Token token = peek();

        consume(TOKEN_TEMPLATE_TAIL, "Expected a template terminator");
        emitString(compiler.previous.start + 1, compiler.previous.length - 2, &compiler.previous);

        // add it to the head
        emitByte(OP_ADD, &compiler.previous);

        compiler.stringDepth--;
        break;
    }
    case TOKEN_TRUE:
    {
        compiler.canAssign = false;

        emitByte(OP_CONSTANT, &token);
        emitConstant(BOOL(1), &token);
        break;
    }
    case TOKEN_FALSE:
    {
        compiler.canAssign = false;

        emitByte(OP_CONSTANT, &token);
        emitConstant(BOOL(0), &token);
        break;
    }
    case TOKEN_NIL:
    {
        compiler.canAssign = false;

        emitByte(OP_CONSTANT, &token);
        emitConstant(NIL, &token);
        break;
    }
    case TOKEN_MINUS:
    case TOKEN_BANG:
    {
        compiler.canAssign = false;
        int bp[2];
        getPrefixBP(bp, token.type);

        expression(bp[1]);

        OpCode opCode = token.type == TOKEN_MINUS ? OP_NEGATE : OP_BANG;

        emitByte(opCode, &token);
        break;
    }
    default:
        errorAt(&token, "Expected an expression");
    }

    while (!check(TOKEN_EOF))
    {
        compiler.canAssign = false;
        Token operator= peek();

        if (operator.type == TOKEN_TEMPLATE_TAIL)
        {
            if (compiler.stringDepth)
            {
                break;
            }
            else
            {
                errorAt(&operator, "This tail doesn't terminate a template");
            }
        }

        if (operator.type == TOKEN_TEMPLATE_MIDDLE)
        {
            if (compiler.stringDepth)
            {
                break;
            }
            else
            {
                errorAt(&operator, "This template middle doesn't belong to any template");
            }
        }

        if (operator.type == TOKEN_RIGHT_PAREN)
        {
            if (compiler.groupingDepth || compiler.inFunGrouping)
            {
                break;
            }
            else
            {
                errorAt(&operator, "This parenthese doesn't terminate a group");
            }
        }

        if (operator.type == TOKEN_COLON)
        {
            if (compiler.ternaryDepth)
            {
                break;
            }
            else
            {
                errorAt(&operator, "This colon is trivial");
            }
        }

        if (operator.type == TOKEN_COMMA)
        {
            if (compiler.inFunGrouping)
            {
                compiler.canAssign = true;
                break;
            }
            else
            {
                errorAt(&operator, "Unexpected ','");
            }
        }

        if (operator.type == TOKEN_EQUAL)
            errorAt(&operator, "Bad assignment target");

        if (operator.type == TOKEN_SEMICOLON)
            break;

        OpCode opCode;

        switch (operator.type)
        {
        case TOKEN_AND:
        case TOKEN_OR:
        case TOKEN_QUESTION_MARK:
        case TOKEN_LEFT_PAREN:
            opCode = -1;
            break;
        case TOKEN_EQUAL_EQUAL:
            opCode = OP_EQUAL;
            break;
        case TOKEN_BANG_EQUAL:
            opCode = OP_NOT_EQUAL;
            break;
        case TOKEN_GREATER:
            opCode = OP_GREATER;
            break;
        case TOKEN_GREATER_EQUAL:
            opCode = OP_GREATER_OR_EQUAL;
            break;
        case TOKEN_LESS:
            opCode = OP_LESS;
            break;
        case TOKEN_LESS_EQUAL:
            opCode = OP_LESS_OR_EQUAL;
            break;
        case TOKEN_PLUS:
            opCode = OP_ADD;
            break;
        case TOKEN_MINUS:
            opCode = OP_SUBTRACT;
            break;
        case TOKEN_STAR:
            opCode = OP_MULTIPLY;
            break;
        case TOKEN_SLASH:
            opCode = OP_DIVIDE;
            break;
        default:
            errorAt(&operator, "Unexpected token");
        }

        int bp[2];
        getInfixBP(bp, operator.type);

        if (minBP > bp[0])
            break;

        next();

        if (opCode == -1)
        {
            switch (operator.type)
            {
            case TOKEN_AND:
            {
                int index = emitJump(OP_JUMP_IF_FALSE, &operator);
                emitByte(OP_POP, &operator);
                expression(bp[1]);
                patchJump(index);
                break;
            }
            case TOKEN_OR:
            {
                int index = emitJump(OP_JUMP_IF_TRUE, &operator);
                emitByte(OP_POP, &operator);
                expression(bp[1]);
                patchJump(index);
                break;
            }
            case TOKEN_QUESTION_MARK:
            {
                compiler.canAssign = true;
                compiler.ternaryDepth++;
                int elseJumpIndex = emitJump(OP_JUMP_IF_FALSE, &operator);
                emitByte(OP_POP, &operator);
                expression(0);
                int ifJumpIndex = emitJump(OP_JUMP, &operator);
                patchJump(elseJumpIndex);
                emitByte(OP_POP, &operator);
                consume(TOKEN_COLON, "Expected a colon that separates the two expressions");
                expression(bp[1]);
                patchJump(ifJumpIndex);
                compiler.ternaryDepth--;
                break;
            }
            case TOKEN_LEFT_PAREN:
            {
                compiler.canAssign = true;
                int prevInFunGrouping = compiler.inFunGrouping;
                compiler.inFunGrouping = true;
                int argsCount = 0;

                if (!match(TOKEN_RIGHT_PAREN))
                {
                    expression(0);
                    argsCount++;

                    while (match(TOKEN_COMMA))
                    {
                        if (argsCount == 255)
                        {
                            errorAt(&compiler.previous, "Can't have more than 255 arguments");
                        }

                        expression(0);
                        argsCount++;
                    }

                    consume(TOKEN_RIGHT_PAREN, "Expected ')'");
                }

                compiler.inFunGrouping = prevInFunGrouping;

                emitBytes(OP_CALL, argsCount, &operator);
                break;
            }
            default:;
            }
        }
        else
        {
            expression(bp[1]);
            emitByte(opCode, &operator);
        }
    }
}

static void synchronize()
{
    compiler.panicMode = false;

    while (!atEnd())
    {
        if (compiler.previous.type == TOKEN_SEMICOLON)
            return;

        switch (compiler.current.type)
        {
        case TOKEN_CLASS:
        case TOKEN_FUN:
        case TOKEN_VAR:
        case TOKEN_FOR:
        case TOKEN_IF:
        case TOKEN_WHILE:
        case TOKEN_RETURN:
        case TOKEN_CONTINUE:
            return;
        }

        advance();
    }
}

static void startScope()
{
    compiler.scopeDepth++;
}

static void endScope()
{
    compiler.scopeDepth--;

    for (int i = compiler.currentLocal - 1; i >= 1; i--)
    {
        Local local = compiler.locals[i];

        if (local.depth == compiler.scopeDepth)
            break;

        if (local.captured)
            emitByte(OP_CLOSE_UPVALUE, &compiler.previous);
        else
            emitByte(OP_POP, &compiler.previous);
        compiler.currentLocal--;
    }
}

static void block()
{
    startScope();

    while (!check(TOKEN_RIGHT_BRACE) && !atEnd())
    {
        declaration();
    }

    consume(TOKEN_RIGHT_BRACE, "Expected '}'");

    endScope();
}

static void ifStatement()
{
    Token token = compiler.previous;

    consume(TOKEN_LEFT_PAREN, "Expected '('");
    compiler.groupingDepth++;
    expression(0);
    compiler.canAssign = true;
    consume(TOKEN_RIGHT_PAREN, "Expected ')'");
    compiler.groupingDepth--;

    int elseJumpIndex = emitJump(OP_JUMP_IF_FALSE, &token);

    statement();

    int ifJumpIndex = emitJump(OP_JUMP, &token);

    patchJump(elseJumpIndex);

    if (match(TOKEN_ELSE))
    {
        statement();
        patchJump(ifJumpIndex);
    }
}

static void whileStatement()
{
#define CURRENT_INDEX compiler.function->chunk.count
    Token token = compiler.previous;

    consume(TOKEN_LEFT_PAREN, "Expected '('");
    compiler.groupingDepth++;

    int prevLoopStartIndex = compiler.loopStartIndex;
    compiler.loopStartIndex = CURRENT_INDEX;

    expression(0);
    compiler.canAssign = true;
    consume(TOKEN_RIGHT_PAREN, "Expected ')'");
    compiler.groupingDepth--;

    int prevLoopEndIndex = compiler.loopEndIndex;
    compiler.loopEndIndex = emitJump(OP_JUMP_IF_FALSE, &token);

    statement();
    emitBytes(OP_JUMP_BACKWARDS, CURRENT_INDEX - compiler.loopStartIndex, &token);

    patchJump(compiler.loopEndIndex);

    compiler.loopStartIndex = prevLoopStartIndex;
    compiler.loopEndIndex = prevLoopEndIndex;
}

static void continueStatement()
{
    Token token = compiler.previous;

    if (compiler.loopStartIndex == -1)
    {
        errorAt(&token, "This keyword can only be used inside loops");
        return;
    }

    emitBytes(OP_JUMP_BACKWARDS, (CURRENT_INDEX - compiler.loopStartIndex), &token);

    consume(TOKEN_SEMICOLON, "Expected ';'");
#undef CURRENT_INDEX
}

static void returnStatement()
{
    Token token = compiler.previous;

    if (!(compiler.type == TYPE_FUNCTION))
        errorAt(&token, "Can't return outside a function");

    if (!match(TOKEN_SEMICOLON))
    {
        expression(0);
        consume(TOKEN_SEMICOLON, "Expected ';'");
    }
    else
    {
        emitByte(OP_NIL, &token);
    }

    emitByte(OP_RETURN, &token);
}

static void expressionStatement()
{
    expression(0);

    compiler.canAssign = true;

    consume(TOKEN_SEMICOLON, "Expected ';'");
    emitByte(OP_POP, &compiler.previous);
}

static void statement()
{
    if (match(TOKEN_LEFT_BRACE))
        block();
    else if (match(TOKEN_IF))
        ifStatement();
    else if (match(TOKEN_WHILE))
        whileStatement();
    else if (match(TOKEN_CONTINUE))
        continueStatement();
    else if (match(TOKEN_RETURN))
        returnStatement();
    else
        expressionStatement();
}

static void defineVariable(Token *token, Token *name)
{
    if (compiler.scopeDepth == 0 && compiler.type == TYPE_SCRIPT)
    {
        emitByte(OP_DEFINE_GLOBAL, token);
        emitIdentifier(name->start, name->length, token);
    }
    else
    {
        if (compiler.currentLocal == UINT8_MAX)
        {
            errorAt(token, "Too many local variables are defiend");
        }

        for (int i = compiler.currentLocal - 1; i >= 0; i--)
        {
            Local *local = &compiler.locals[i];

            if (local->depth != compiler.scopeDepth)
                break;

            if (sameIdentifier(name, &local->name))
                warningAt(name, "There's a variable with the same name in the same scope");
        }

        Local local = {*name, compiler.scopeDepth, false};

        compiler.locals[compiler.currentLocal++] = local;
    }
}

static void varDeclaration()
{
    Token token = compiler.previous;

    consume(TOKEN_IDENTIFIER, "Expected the name of the variable after 'var'");

    Token name = compiler.previous;

    if (match(TOKEN_EQUAL))
    {
        expression(0);
    }
    else
    {
        emitByte(OP_NIL, &name);
    }

    consume(TOKEN_SEMICOLON, "Expected ';'");

    defineVariable(&token, &name);
}

static void funDeclaration()
{
    // TODO if there's an error inside funCompiler drill it up to the main compiler

    //>> create a new compiler and sync it
    Compiler enclosingCompiler = compiler;
    Compiler funCompiler;
    compiler = funCompiler;

    initCompiler(enclosingCompiler.scanner, TYPE_FUNCTION, &enclosingCompiler);
    //<<

    Token token = enclosingCompiler.previous;

    compiler.current = enclosingCompiler.current; // To allow the next line to work!

    consume(TOKEN_IDENTIFIER, "Expected function name");
    Token name = compiler.previous;
    compiler.function->name = allocateObjString(name.start, name.length);

    defineVariable(&name, &name);

    consume(TOKEN_LEFT_PAREN, "Expected '('");

    if (!match(TOKEN_RIGHT_PAREN))
    {

#define PARSE_PARAM                                         \
    consume(TOKEN_IDENTIFIER, "Expected a param");          \
    defineVariable(&compiler.previous, &compiler.previous); \
    compiler.function->arity++;

        compiler.inFunGrouping = true;

        PARSE_PARAM

        while (match(TOKEN_COMMA))
        {
            if (compiler.function->arity == 255)
            {
                errorAt(&compiler.previous, "Can't have more than 255 parameters");
            }

            PARSE_PARAM
        }

        consume(TOKEN_RIGHT_PAREN, "Expected ')'");
        compiler.inFunGrouping = false;
    }

    consume(TOKEN_LEFT_BRACE, "Expected '{'");

    while (!check(TOKEN_RIGHT_BRACE) && !atEnd())
    {
        declaration();
    }

    consume(TOKEN_RIGHT_BRACE, "Expected '}'");

    emitReturn(&compiler.previous);

    if (compiler.hadError)
        return;

#ifdef DEBUG_BYTECODE
    disassembleChunk(&compiler.function->chunk, compiler.function->name->chars);
#endif

    ObjFunction *function = compiler.function;
    UpValue *upValues = compiler.upValues;
    uint8_t currentUpValue = compiler.currentUpValue;

    enclosingCompiler.current = compiler.current;

    compiler = enclosingCompiler;

    emitByte(OP_CLOSURE, &token);

    emitConstant(OBJ((Obj *)function), &token);

    emitByte(currentUpValue, &token);

    for (int i = 0; i < currentUpValue; i++)
    {
        UpValue *upValue = &upValues[i];

        emitBytes((uint8_t)upValue->local, upValue->index, &token);
    }

    defineVariable(&token, &name);
}

static void declaration()
{
    if (match(TOKEN_VAR))
        varDeclaration();
    else if (match(TOKEN_FUN))
        funDeclaration();
    else if (match(TOKEN_SEMICOLON))
    {
        warningAt(&compiler.previous, "Trivial ';'");
    }
    else
        statement();

    compiler.canAssign = true;
}

ObjFunction *compile(Scanner *scanner)
{
    initCompiler(scanner, TYPE_SCRIPT, NULL);
    advance();

    while (!atEnd())
    {
        declaration();
        if (compiler.panicMode)
            synchronize();
    }

    emitReturn(&compiler.current);

#ifdef DEBUG_BYTECODE
    if (!compiler.hadError)
    {
        disassembleChunk(&compiler.function->chunk, "<script>");
    }
#endif

    return compiler.hadError
               ? NULL
               : compiler.function;
}