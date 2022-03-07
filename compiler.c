#include "compiler.h"
#include "reporter.h"
#include <string.h>
#include "object.h"
#include "debug.h"

void errorAt(Compiler* compiler, Token* token, char msg[]) {
    if (compiler->panicMode) return; // We avoid throwing meaningless errors until we recover

    compiler->panicMode = true;
    compiler->hadError = true;
    
    report(token->type == TOKEN_ERROR? REPORT_SCAN_ERROR: REPORT_PARSE_ERROR, token, msg, NULL);
};

void warningAt(Compiler* compiler, Token* token, char msg[]) {
    if (compiler->panicMode) return; // We avoid throwing meaningless errors until we recover

    report(REPORT_WARNING, token, msg, NULL);
}

void emitByte(Compiler* compiler, uint8_t byte, Token* token) {
    writeChunk(&compiler->function->chunk, byte, token);
}

void emitConstant(Compiler* compiler, Value value, Token* token) {
    uint8_t i = addConstant(&compiler->function->chunk, value);

    if (i > UINT8_MAX) {
        errorAt(compiler, &compiler->previous, "Too many constants in one chunk");

        return;
    }

    emitByte(compiler, i, token);
};

void emitNumber(Compiler* compiler, char* s, Token* token) {
    double value = strtod(s, NULL);

    emitByte(compiler, OP_CONSTANT, token);
    emitConstant(compiler, NUMBER(value), token);
}

void emitIdentifier(Compiler* compiler, char* s, int length, Token* token) {
    ObjString* identifier = allocateObjString(compiler->vm, s, length);

    emitConstant(compiler, STRING(identifier), token);
}

void emitString(Compiler* compiler, char* s, int length, Token* token) {
    ObjString* objString = allocateObjString(compiler->vm, s, length);

    emitByte(compiler, OP_CONSTANT, token);
    emitConstant(compiler, STRING(objString), token);
}

int emitJump(Compiler* compiler, OpCode type, Token* token) {
    emitByte(compiler, type, token);
    emitByte(compiler, 1, token);

    return compiler->function->chunk.count - 1;
}

void patchJump(Compiler* compiler, int index) {
    int value = compiler->function->chunk.count - index;

    if (value > UINT8_MAX) {
        errorAt(compiler, &compiler->function->chunk.tokenArr.tokens[index], "Too many code to jump over!");
    }

    compiler->function->chunk.code[index]  = value;
}

void emitReturn(Compiler* compiler, Token* token) {
    emitByte(compiler, OP_NIL, token);
    emitByte(compiler, OP_RETURN, token);
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

void getPrefixBP(int bp[2], TokenType type) {
    switch (type) {
        case TOKEN_MINUS:
        case TOKEN_BANG:
            bp[1] = 17;
            break;
        default: ;
    }
}

void getInfixBP(int bp[2], TokenType type) {
    switch (type) {
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
        default: ;
    }
}

void advance(Compiler* compiler) {
    compiler->previous = compiler->current;

    while (true) {
        compiler->current = scanToken(compiler->scanner);

        if (compiler->current.type != TOKEN_ERROR) break;

        errorAt(compiler, &compiler->current, compiler->current.errorMsg);
    }
}

void initCompiler(Compiler* compiler, Scanner* scanner, Vm* vm, FunctionType type) {
    compiler->function = allocateObjFunction(vm);
    compiler->type = type;

    compiler->scanner = scanner;
    compiler->vm = vm;
    compiler->currentLocal = 0;
    compiler->hadError = false;
    compiler->panicMode = false;
    compiler->canAssign = true;
    compiler->inFunGrouping = false;
    compiler->groupingDepth = 0;
    compiler->stringDepth = 0;
    compiler->scopeDepth = 0;
    compiler->ternaryDepth = 0;
    compiler->loopStartIndex = -1;
    compiler->loopEndIndex = -1;
}

static void consume(Compiler* compiler, TokenType type, char msg[]) {
    if (compiler->current.type == type) {
        advance(compiler);
        return;
    }

    errorAt(compiler, &compiler->current, msg);
}

static Token peek(Compiler* compiler) {
    return compiler->current;
}

static Token next(Compiler* compiler) {
    advance(compiler);

    return compiler->previous;
}

static bool check(Compiler* compiler, TokenType type) {
    return peek(compiler).type == type;
}

static bool atEnd(Compiler* compiler) {
    return check(compiler, TOKEN_EOF);
}

static bool match(Compiler* compiler, TokenType type) {
    if (check(compiler, type)) {
        advance(compiler);
        return true;
    }

    return false;
}

// compares two identifier tokens
static bool sameIdentifier(Token* token1, Token* token2) {
    return token1->length == token2->length & strncmp(token1->start, token2->start, token1->length) == 0;
}

static int resolveLocal(Compiler* compiler, Token* token) {
    for (int i = compiler->currentLocal - 1; i >= 0; i--) {
        Local local = compiler->locals[i];

        if (sameIdentifier(token, &local.name)) {
            return i;
        }
    }

    return -1;
}

static void expression(Compiler* compiler, int minBP) {
    Token token = next(compiler);

    switch (token.type) {
        case TOKEN_IDENTIFIER: {
            int local = resolveLocal(compiler, &token);

            if (match(compiler, TOKEN_EQUAL)) {
                Token equal = compiler->previous;

                if (compiler->canAssign) {
                    // parse the target
                    int bp[2];
                    getInfixBP(bp, equal.type);

                    expression(compiler, bp[1]);
                    emitByte(compiler, local == -1? OP_ASSIGN_GLOBAL: OP_ASSIGN_LOCAL, &token); // point to equal instead
                } else {
                    errorAt(compiler, &equal, "Bad assignment target");
                }
            } else {
                compiler->canAssign = false;
                emitByte(compiler, local == -1? OP_GET_GLOBAL: OP_GET_LOCAL, &token);
            }

            if (local == -1) {
                emitIdentifier(compiler, token.start, token.length, &token);
            } else {
                emitByte(compiler, local, &token);
            }

            break;
        }
        case TOKEN_LEFT_PAREN:
            compiler->canAssign = true;
            compiler->groupingDepth++;
            expression(compiler, 0);
            consume(compiler, TOKEN_RIGHT_PAREN, "Expected ')' after the group");
            compiler->groupingDepth--;
            break;
        case TOKEN_NUMBER:
        compiler->canAssign = false;
            emitNumber(compiler, token.start, &token);
            break;
        case TOKEN_STRING: 
        compiler->canAssign = false;
            emitString(compiler, token.start + 1, token.length - 2, &token);
            break;
        case TOKEN_TEMPLATE_HEAD: {
            compiler->canAssign = false;
            compiler->stringDepth++;

            // emit the head
            emitString(compiler, token.start + 1, token.length - 3, &token);

            // emit the expression
            expression(compiler, 0);

            // add them
            emitByte(compiler, OP_ADD, &token);

            // emitting the middle
            while (check(compiler, TOKEN_TEMPLATE_MIDDLE)) {
                Token token = next(compiler);

                // emit the string
                emitString(compiler, token.start + 1, token.length - 3, &token);

                // emit the expression
                expression(compiler, 0);
                
                // add them both
                emitByte(compiler, OP_ADD, &token);

                // add this to the head
                emitByte(compiler, OP_ADD, &token);
            }

            // emitting the tail
            Token token = peek(compiler);

            consume(compiler, TOKEN_TEMPLATE_TAIL, "Expected a template terminator");
            emitString(compiler, compiler->previous.start + 1, compiler->previous.length - 2, &compiler->previous);

            // add it to the head
            emitByte(compiler, OP_ADD, &compiler->previous);
        
            compiler->stringDepth--;
            break;
        }
        case TOKEN_TRUE: {
            compiler->canAssign = false;

            emitByte(compiler, OP_CONSTANT, &token);
            emitConstant(compiler, BOOL(1), &token);
            break;
        }
        case TOKEN_FALSE: {
            compiler->canAssign = false;

            emitByte(compiler, OP_CONSTANT, &token);
            emitConstant(compiler, BOOL(0), &token);
            break;
        }
        case TOKEN_NIL: {
            compiler->canAssign = false;

            emitByte(compiler, OP_CONSTANT, &token);
            emitConstant(compiler, NIL, &token);
            break;
        }
        case TOKEN_MINUS:
        case TOKEN_BANG: {
            compiler->canAssign = false;
            int bp[2];
            getPrefixBP(bp, token.type);

            expression(compiler, bp[1]);

            OpCode opCode = token.type == TOKEN_MINUS? OP_NEGATE: OP_BANG;

            emitByte(compiler, opCode, &token);
            break;
        }
        default:
            errorAt(compiler, &token, "Expected an expression");
    }

    while (!check(compiler, TOKEN_EOF)) {
        compiler->canAssign = false;
        Token operator = peek(compiler);

        if (operator.type == TOKEN_TEMPLATE_TAIL) {
            if (compiler->stringDepth) {
                break;
            } else {
                errorAt(compiler, &operator, "This tail doesn't terminate a template");
            }
        }

        if (operator.type == TOKEN_TEMPLATE_MIDDLE) {
            if (compiler->stringDepth) {
                break;
            } else {
                errorAt(compiler, &operator, "This template middle doesn't belong to any template");
            }
        }

        if (operator.type == TOKEN_RIGHT_PAREN) {
            if (compiler->groupingDepth || compiler->inFunGrouping) {
                break;
            } else {
                errorAt(compiler, &operator, "This parenthese doesn't terminate a group");
            }
        }

        if (operator.type == TOKEN_COLON) {
            if (compiler->ternaryDepth) {
                break;
            } else {
                errorAt(compiler, &operator, "This colon is trivial");
            }
        }

        if (operator.type == TOKEN_COMMA) {
            if (compiler->inFunGrouping) {
                break;
            } else {
                errorAt(compiler, &operator, "Unexpected ','");
            }
        }

        if (operator.type == TOKEN_EQUAL) errorAt(compiler, &operator, "Bad assignment target");

        if (operator.type == TOKEN_SEMICOLON) break;

        OpCode opCode;

        switch (operator.type) {
            case TOKEN_AND:
            case TOKEN_OR:
            case TOKEN_QUESTION_MARK:
            case TOKEN_LEFT_PAREN:
                opCode = -1;
                break;
            case TOKEN_EQUAL_EQUAL: opCode = OP_EQUAL; break;
            case TOKEN_BANG_EQUAL: opCode = OP_NOT_EQUAL; break;
            case TOKEN_GREATER: opCode = OP_GREATER; break;
            case TOKEN_GREATER_EQUAL: opCode = OP_GREATER_OR_EQUAL; break;
            case TOKEN_LESS: opCode = OP_LESS; break;
            case TOKEN_LESS_EQUAL: opCode = OP_LESS_OR_EQUAL; break;
            case TOKEN_PLUS: opCode = OP_ADD; break;
            case TOKEN_MINUS: opCode = OP_SUBTRACT; break;
            case TOKEN_STAR: opCode = OP_MULTIPLY; break;
            case TOKEN_SLASH: opCode = OP_DIVIDE; break;
            default: 
                errorAt(compiler, &operator, "Unexpected token");
        }

        int bp[2];
        getInfixBP(bp, operator.type);

        if (minBP > bp[0]) break;

        next(compiler);

        if (opCode == -1) {
            if (operator.type == TOKEN_AND) {
                int index = emitJump(compiler, OP_JUMP_IF_FALSE, &operator);
                emitByte(compiler, OP_POP, &operator);
                expression(compiler, bp[1]);
                patchJump(compiler, index);
            }
            
            if (operator.type == TOKEN_OR) {
                int index = emitJump(compiler, OP_JUMP_IF_TRUE, &operator);
                emitByte(compiler, OP_POP, &operator);
                expression(compiler, bp[1]);
                patchJump(compiler, index);
            }

            if (operator.type == TOKEN_QUESTION_MARK) {
                compiler->ternaryDepth++;
                int elseJumpIndex = emitJump(compiler, OP_JUMP_IF_FALSE, &operator);
                expression(compiler, 0);
                int ifJumpIndex = emitJump(compiler, OP_JUMP, &operator);
                patchJump(compiler, elseJumpIndex);
                consume(compiler, TOKEN_COLON, "Expected a colon that separates the two expressions");
                expression(compiler, bp[1]);
                patchJump(compiler, ifJumpIndex);
                compiler->ternaryDepth--;
            }

            if (operator.type == TOKEN_LEFT_PAREN) {
                compiler->inFunGrouping = true;
                int argsCount = 0;

                if (!match(compiler, TOKEN_RIGHT_PAREN)) {
                    expression(compiler, 0);
                    argsCount++;

                    while (match(compiler, TOKEN_COMMA)) {
                        if (argsCount == 255) {
                            errorAt(compiler, &compiler->previous, "Can't have more than 255 arguments");
                        }

                        expression(compiler, 0);
                        argsCount++;
                    }

                    consume(compiler, TOKEN_RIGHT_PAREN, "Expected ')'");
                }

                compiler->inFunGrouping = false;

                emitByte(compiler, OP_CALL, &operator);
                emitByte(compiler, argsCount, &operator);
            }
        } else {
            expression(compiler, bp[1]);
            emitByte(compiler, opCode, &operator);
        }
    }
}

static void synchronize(Compiler* compiler) {
    compiler->panicMode = false;

    while (!atEnd(compiler)) {
        if (compiler->previous.type == TOKEN_SEMICOLON) return;

        switch (compiler->current.type) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_RETURN:
            case TOKEN_CONTINUE:
            case TOKEN_BREAK:
                return;
        }

        advance(compiler);
    }
}

static void statement(Compiler*);

static void declaration(Compiler*);

static void block(Compiler* compiler) {
    compiler->scopeDepth++;

    while (!check(compiler, TOKEN_RIGHT_BRACE) && !atEnd(compiler)) {
        declaration(compiler);
    }

    consume(compiler, TOKEN_RIGHT_BRACE, "Expected '}'");
    compiler->scopeDepth--;

    for (int i = compiler->currentLocal - 1; i >= 0; i--) {
        if (compiler->locals[i].depth == compiler->scopeDepth) break;

        emitByte(compiler, OP_POP, &compiler->previous);
        compiler->currentLocal--;
    }
}

static void ifStatement(Compiler* compiler) {
    Token token = compiler->previous;

    consume(compiler, TOKEN_LEFT_PAREN, "Expected '('");
    compiler->groupingDepth++;
    expression(compiler, 0);
    compiler->canAssign = true;
    consume(compiler, TOKEN_RIGHT_PAREN, "Expected ')'");
    compiler->groupingDepth--;

    int elseJumpIndex = emitJump(compiler, OP_JUMP_IF_FALSE, &token);

    statement(compiler);

    int ifJumpIndex = emitJump(compiler, OP_JUMP, &token);

    patchJump(compiler, elseJumpIndex);

    if (match(compiler, TOKEN_ELSE)) {
        statement(compiler);
        patchJump(compiler, ifJumpIndex);
    }
}

static void whileStatement(Compiler* compiler) {
    #define CURRENT_INDEX compiler->function->chunk.count

    Token token = compiler->previous;

    consume(compiler, TOKEN_LEFT_PAREN, "Expected '('");
    compiler->groupingDepth++;

    int prevLoopStartIndex = compiler->loopStartIndex;
    compiler->loopStartIndex = CURRENT_INDEX;

    expression(compiler, 0);
    compiler->canAssign = true;
    consume(compiler, TOKEN_RIGHT_PAREN, "Expected ')'");
    compiler->groupingDepth--;

    int prevLoopEndIndex = compiler->loopEndIndex;
    compiler->loopEndIndex = emitJump(compiler, OP_JUMP_IF_FALSE, &token);

    statement(compiler);
    emitByte(compiler, OP_JUMP_BACKWARDS, &token);
    emitByte(compiler, CURRENT_INDEX - compiler->loopStartIndex, &token);

    patchJump(compiler, compiler->loopEndIndex);

    compiler->loopStartIndex = prevLoopStartIndex;
    compiler->loopEndIndex = prevLoopEndIndex;
}

static void continueStatement(Compiler* compiler) {
    Token token = compiler->previous;

    if (compiler->loopStartIndex == -1) {
        errorAt(compiler, &token, "This keyword can only be used inside loops");
        return;
    }
    
    emitByte(compiler, OP_JUMP_BACKWARDS, &token);
    emitByte(compiler, (CURRENT_INDEX - compiler->loopStartIndex), &token);

    consume(compiler, TOKEN_SEMICOLON, "Expected ';'");
}

static void breakStatement(Compiler* compiler) {
    Token token = compiler->previous;

    if (compiler->loopStartIndex == -1) {
        errorAt(compiler, &token, "This keyword can only be used inside loops");
        return;
    }
    
    emitByte(compiler, OP_JUMP, &token);
    emitByte(compiler, (compiler->loopStartIndex - CURRENT_INDEX), &token);

    consume(compiler, TOKEN_SEMICOLON, "Expected ';'");

    #undef CURRENT_INDEX  
}

static void returnStatement(Compiler* compiler) {
    Token token = compiler->previous;

    if (!compiler->type == TYPE_FUNCTION) errorAt(compiler, &token, "Can't return outside a function");

    if (!match(compiler, TOKEN_SEMICOLON)) {
        expression(compiler, 0);
        consume(compiler, TOKEN_SEMICOLON, "Expected ';'");
    } else {
        emitByte(compiler, OP_NIL, &token);
    }

    emitByte(compiler, OP_RETURN, &token);
}

static void expressionStatement(Compiler* compiler) {
    expression(compiler, 0);

    compiler->canAssign  = true;

    consume(compiler, TOKEN_SEMICOLON, "Expected ';'");
    emitByte(compiler, OP_POP, &compiler->previous);
}

static void statement(Compiler* compiler) {
    if (match(compiler, TOKEN_LEFT_BRACE)) block(compiler);
    else if (match(compiler, TOKEN_IF)) ifStatement(compiler);
    else if (match(compiler, TOKEN_WHILE)) whileStatement(compiler);
    else if (match(compiler, TOKEN_CONTINUE)) continueStatement(compiler);
    else if (match(compiler, TOKEN_BREAK)) breakStatement(compiler);
    else if (match(compiler, TOKEN_RETURN)) returnStatement(compiler);
    else expressionStatement(compiler);
}

static void defineVariable(Compiler* compiler, Token* token, Token* name) {
    if (compiler->scopeDepth == 0 && compiler->type == TYPE_SCRIPT) {
        emitByte(compiler, OP_DEFINE_GLOBAL, token);
        emitIdentifier(compiler, name->start, name->length, token);
    } else {
        if (compiler->currentLocal == UINT8_MAX) {
            errorAt(compiler, token, "Too many local variables are defiend");
        }

        for (int i = compiler->currentLocal - 1; i >= 0; i--) {
            Local local = compiler->locals[i];

            if (local.depth != compiler->scopeDepth) break;

            if (sameIdentifier(name, &local.name)) warningAt(compiler, name, "There's a variable with the same name in the same scope");
        }

        Local local = {*name, compiler->scopeDepth};

        compiler->locals[compiler->currentLocal++] = local;

        emitByte(compiler, OP_DEFINE_LOCAL, token);
    }
}

static void varDeclaration(Compiler* compiler) {
    Token token = compiler->previous;

    consume(compiler, TOKEN_IDENTIFIER, "Expected the name of the variable after 'var'");

    Token name = compiler->previous;

    if (match(compiler, TOKEN_EQUAL)) {
        expression(compiler, 0);
    } else {
        emitByte(compiler, OP_NIL, &name);
    }

    consume(compiler, TOKEN_SEMICOLON, "Expected ';'");

    defineVariable(compiler, &token, &name);
}

static void funDeclaration(Compiler* compiler) {
    #define SYNC_COMPILERS(compiler1, compiler2)\
            compiler1->previous = compiler2->previous;\
            compiler1->current = compiler2->current;

    //>> create a new compiler and sync it
    Compiler funCompiler;
    initCompiler(&funCompiler, compiler->scanner, compiler->vm, TYPE_FUNCTION);

    SYNC_COMPILERS((&funCompiler), compiler)
    //<<

    Token token = funCompiler.previous;

    consume(&funCompiler, TOKEN_IDENTIFIER, "Expected function name");
    Token name = funCompiler.previous;
    funCompiler.function->name = allocateObjString(funCompiler.vm, name.start, name.length);

    defineVariable(&funCompiler, &name, &name);

    consume(&funCompiler, TOKEN_LEFT_PAREN, "Expected '('");


    if (!match(&funCompiler, TOKEN_RIGHT_PAREN)) {
        #define PARSE_PARAM\
            consume(&funCompiler, TOKEN_IDENTIFIER, "Expected a param");\
            defineVariable(&funCompiler, &funCompiler.previous, &funCompiler.previous);\
            funCompiler.function->arity++;


        funCompiler.inFunGrouping = true;

        PARSE_PARAM

        while (match(&funCompiler, TOKEN_COMMA)) {
            if (funCompiler.function->arity == 255) {
                errorAt(compiler, &funCompiler.previous, "Can't have more than 255 parameters");
            }

            PARSE_PARAM
        }

        consume(&funCompiler, TOKEN_RIGHT_PAREN, "Expected ')'");
        funCompiler.inFunGrouping = false;
    }

    consume(&funCompiler, TOKEN_LEFT_BRACE, "Expected '{'");

    while (!check(&funCompiler, TOKEN_RIGHT_BRACE) && !atEnd(&funCompiler)) {
        declaration(&funCompiler);
    }

    consume(&funCompiler, TOKEN_RIGHT_BRACE, "Expected '}'");

    emitReturn(&funCompiler, &funCompiler.previous);

    emitByte(compiler, OP_CONSTANT, &token);
    emitConstant(compiler, FUNCTION(funCompiler.function), &token);
    defineVariable(compiler, &token, &name);

    disassembleChunk(&funCompiler.function->chunk, funCompiler.function->name->chars);

    //>> sync them back
    SYNC_COMPILERS(compiler, (&funCompiler))
    //<<

    #undef SYNC_COMPILERS
}

static void declaration(Compiler* compiler) {
    if (match(compiler, TOKEN_VAR)) varDeclaration(compiler);
    else if (match(compiler, TOKEN_FUN)) funDeclaration(compiler);
    else if (match(compiler, TOKEN_SEMICOLON)) {
        warningAt(compiler, &compiler->previous, "Trivial ';'");
    } else statement(compiler);

    compiler->canAssign = true;
}

ObjFunction* compile(Compiler* compiler, Scanner* scanner, Vm* vm) {
    initCompiler(compiler, scanner, vm, TYPE_SCRIPT);
    advance(compiler);

    while (!atEnd(compiler)) {
        declaration(compiler);
        if (compiler->panicMode) synchronize(compiler);
    }

    emitReturn(compiler, &compiler->current);

    return compiler->hadError? NULL: compiler->function;
}