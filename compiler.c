#include "compiler.h"
#include "reporter.h"
#include <string.h>
#include "object.h"

void errorAt(Compiler* compiler, Token* token, char msg[]) {
    if (compiler->panicMode) return; // We avoid throwing meaningless errors until we recover

    compiler->panicMode = true;
    compiler->hadError = true;
    
    report(token->type == TOKEN_ERROR? REPORT_SCAN_ERROR: REPORT_PARSE_ERROR, token, msg);
};

void warningAt(Compiler* compiler, Token* token, char msg[]) {
    if (compiler->panicMode) return; // We avoid throwing meaningless errors until we recover

    report(REPORT_WARNING, token, msg);
}

void emitByte(Compiler* compiler, uint8_t byte, Token* token) {
    writeChunk(&compiler->function->chunk, byte, token);
}

uint8_t makeConstant(Compiler* compiler, Value value) {
    uint8_t i = addConstant(&compiler->function->chunk, value);

    if (i > UINT8_MAX) {
        errorAt(compiler, &compiler->previous, "Too many constants in one chunk");

        return 0;
    }

    return i;
};

void emitIdentifier(Compiler* compiler, char* s, int length, Token* token) {
    char* chars = malloc(length + 1);

    strncpy(chars, s, length);

    chars[length] = '\0';

    ObjString* identifier = allocateObjString(compiler->vm, chars);

    uint8_t i = makeConstant(compiler, STRING(identifier));

    emitByte(compiler, i, token);
}

void emitNumber(Compiler* compiler, char* s, Token* token) {
    double value = strtod(s, NULL);
    uint8_t i = makeConstant(compiler, (Value) {VAL_NUMBER, { .number = value }});

    emitByte(compiler, OP_CONSTANT, token);
    emitByte(compiler, i, token);
}

void emitString(Compiler* compiler, char* s, int length, Token* token) {
    char* chars = malloc(length + 1);

    strncpy(chars, s, length);

    chars[length] = '\0';

    ObjString* objString = allocateObjString(compiler->vm, chars);

    uint8_t i = makeConstant(compiler, STRING(objString));

    emitByte(compiler, OP_CONSTANT, token);
    emitByte(compiler, i, token);
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

/*
    = -> [2, 1]
    ?: -> [4, 3]
    or -> [5, 6]
    and -> [7, 8]
    == != -> [9, 10]
    < > <= >= -> [11, 12]
    + - -> [13, 14]
    * / -> [15, 16]
    (unary) - -> [?, 17];
    . () -> [18, ?];
*/

void getPrefixBP(int bp[2], TokenType type) {
    switch (type) {
        case TOKEN_MINUS:
        case TOKEN_BANG:
            bp[1] = 17;
            break;
        default:
            break;
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
            break;
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
    compiler->groupingDepth = 0;
    compiler->stringDepth = 0;
    compiler->scopeDepth = 0;
    compiler->ternaryDepth = 0;
    compiler->loopStartIndex = -1;
    compiler->loopEndIndex = -1;

    advance(compiler);
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
            uint8_t i = makeConstant(compiler, (Value) {VAL_BOOL, { .boolean = 1 }});

            emitByte(compiler, OP_CONSTANT, &token);
            emitByte(compiler, i, &token);
            break;
        }
        case TOKEN_FALSE: {
            compiler->canAssign = false;
            uint8_t i = makeConstant(compiler, (Value) {VAL_BOOL, { .boolean = 0 }});

            emitByte(compiler, OP_CONSTANT, &token);
            emitByte(compiler, i, &token);
            break;
        }
        case TOKEN_NIL: {
            compiler->canAssign = false;
            uint8_t i = makeConstant(compiler, (Value) {VAL_NIL, { .number = 0 }});

            emitByte(compiler, OP_CONSTANT, &token);
            emitByte(compiler, i, &token);
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
            if (compiler->groupingDepth) {
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

        if (operator.type == TOKEN_EQUAL) errorAt(compiler, &operator, "Bad assignment target");

        if (operator.type == TOKEN_SEMICOLON) break;

        OpCode opCode;

        switch (operator.type) {
            case TOKEN_AND:
            case TOKEN_OR:
            case TOKEN_QUESTION_MARK:
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

static void declaration(Compiler*);

static void statement(Compiler* compiler) {
    if (match(compiler, TOKEN_LEFT_BRACE)) {
        compiler->scopeDepth++;

        while (!check(compiler, TOKEN_RIGHT_BRACE) & !atEnd(compiler)) {
            declaration(compiler);
        }

        consume(compiler, TOKEN_RIGHT_BRACE, "Expected '}'");
        compiler->scopeDepth--;

        for (int i = compiler->currentLocal - 1; i >= 0; i--) {
            if (compiler->locals[i].depth == compiler->scopeDepth) break;

            emitByte(compiler, OP_POP, &compiler->previous);
            compiler->currentLocal--;
        }
    } else if (match(compiler, TOKEN_IF)) {
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
    } else if (match(compiler, TOKEN_WHILE)) {
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
    } else if (match(compiler, TOKEN_CONTINUE)) {
        Token token = compiler->previous;

        if (compiler->loopStartIndex == -1) {
            errorAt(compiler, &token, "This keyword can only be used inside loops");
            return;
        }
        
        emitByte(compiler, OP_JUMP_BACKWARDS, &token);
        emitByte(compiler, (CURRENT_INDEX - compiler->loopStartIndex), &token);

        consume(compiler, TOKEN_SEMICOLON, "Expected ';'");

    } else if (match(compiler, TOKEN_BREAK)) {
        Token token = compiler->previous;

        if (compiler->loopStartIndex == -1) {
            errorAt(compiler, &token, "This keyword can only be used inside loops");
            return;
        }
        
        emitByte(compiler, OP_JUMP, &token);
        emitByte(compiler, (compiler->loopStartIndex - CURRENT_INDEX), &token);

        consume(compiler, TOKEN_SEMICOLON, "Expected ';'");

        #undef CURRENT_INDEX
    } else {
        expression(compiler, 0);

        compiler->canAssign  = true;

        consume(compiler, TOKEN_SEMICOLON, "Expected ';'");
        emitByte(compiler, OP_POP, &compiler->previous);
    }
}

static void declaration(Compiler* compiler) {
    if (match(compiler, TOKEN_VAR)) {
        Token var = compiler->previous;

        consume(compiler, TOKEN_IDENTIFIER, "Expected the name of the variable after 'var'");

        Token name = compiler->previous;

        if (match(compiler, TOKEN_EQUAL)) {
            expression(compiler, 0);
        } else {
            emitByte(compiler, OP_NIL, &name);
        }

        consume(compiler, TOKEN_SEMICOLON, "Expected ';'");

        if (compiler->scopeDepth == 0) {
            emitByte(compiler, OP_DEFINE_GLOBAL, &var);
            emitIdentifier(compiler, name.start, name.length, &var);
        } else {
            for (int i = compiler->currentLocal - 1; i >= 0; i--) {
                Local local = compiler->locals[i];

                if (local.depth != compiler->scopeDepth) break;

                if (sameIdentifier(&name, &local.name)) warningAt(compiler, &name, "There's a variable with the same name in the same scope");
            }

            Local local = {name, compiler->scopeDepth};

            compiler->locals[compiler->currentLocal] = local;

            emitByte(compiler, OP_DEFINE_LOCAL, &var);
            
            // MAX LOCAL VARIABLES

            compiler->currentLocal++;
        }
    } else statement(compiler);

    compiler->canAssign = true;
}

ObjFunction* compile(Compiler* compiler) {
    while (!atEnd(compiler)) {
        declaration(compiler);
        if (compiler->panicMode) synchronize(compiler);
    }

    return compiler->hadError? NULL: compiler->function;
}