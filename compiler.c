#include "compiler.h"
#include "error.h"
#include <string.h>
#include "object.h"

void errorAt(Compiler* compiler, Token* token, char msg[]) {
    if (compiler->panicMode) return; // We avoid throwing meaningless errors until we recover

    compiler->panicMode = true;
    compiler->hadError = true;
    
    reportError(token->type == TOKEN_ERROR? ERROR_SCAN: ERROR_PARSE, token, msg);
};

uint8_t makeConstant(Compiler* compiler, Value value) {
    uint8_t i = addConstant(compiler->chunk, value);

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

    writeChunk(compiler->chunk, i, *token);
}

void emitNumber(Compiler* compiler, char* s, Token* token) {
    double value = strtod(s, NULL);
    uint8_t i = makeConstant(compiler, (Value) {VAL_NUMBER, { .number = value }});

    writeChunk(compiler->chunk, OP_CONSTANT, *token);
    writeChunk(compiler->chunk, i, *token);
}

void emitString(Compiler* compiler, char* s, int length, Token* token) {
    char* chars = malloc(length + 1);

    strncpy(chars, s, length);

    chars[length] = '\0';

    ObjString* objString = allocateObjString(compiler->vm, chars);

    uint8_t i = makeConstant(compiler, STRING(objString));

    writeChunk(compiler->chunk, OP_CONSTANT, *token);
    writeChunk(compiler->chunk, i, *token);
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
        default:
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

void initCompiler(Compiler* compiler, Scanner* scanner, Chunk* chunk, Vm* vm) {
    compiler->scanner = scanner;
    compiler->chunk = chunk;
    compiler->vm = vm;
    compiler->hadError = false;
    compiler->panicMode = false;
    compiler->canAssign = true;
    compiler->groupingDepth = 0;
    compiler->stringDepth = 0;

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

static bool atEnd(Compiler* compiler) {
    return peek(compiler).type == TOKEN_EOF;
}

static bool match(Compiler* compiler, TokenType type) {
    if (peek(compiler).type == type) {
        advance(compiler);
        return true;
    }

    return false;
}

static void expression(Compiler* compiler, int minBP) {
    Token token = next(compiler);

    switch (token.type) {
        case TOKEN_IDENTIFIER:
            if (peek(compiler).type == TOKEN_EQUAL) {
                Token equal = next(compiler);

                if (compiler->canAssign) {
                    // parse the target
                    int bp[2];
                    getInfixBP(bp, equal.type);

                    expression(compiler, bp[1]);
                    writeChunk(compiler->chunk, OP_ASSIGN_GLOBAL, token);
                } else {
                    errorAt(compiler, &equal, "Bad assignment target");
                }
            } else {
                compiler->canAssign = false;
                writeChunk(compiler->chunk, OP_GET_GLOBAL, token);
            }
            emitIdentifier(compiler, token.start, token.length, &token);
            break;
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
            writeChunk(compiler->chunk, OP_ADD, token);

            // emitting the middle
            while (peek(compiler).type == TOKEN_TEMPLATE_MIDDLE) {
                Token token = next(compiler);

                // emit the string
                emitString(compiler, token.start + 1, token.length - 3, &token);

                // emit the expression
                expression(compiler, 0);
                
                // add them both
                writeChunk(compiler->chunk, OP_ADD, token);

                // add this to the head
                writeChunk(compiler->chunk, OP_ADD, token);
            }

            // emitting the tail
            Token token = peek(compiler);

            consume(compiler, TOKEN_TEMPLATE_TAIL, "Expected a template terminator");
            emitString(compiler, compiler->previous.start + 1, compiler->previous.length - 2, &compiler->previous);

            // add it to the head
            writeChunk(compiler->chunk, OP_ADD, compiler->previous);
        
            compiler->stringDepth--;
            break;
        }
        case TOKEN_TRUE: {
            compiler->canAssign = false;
            uint8_t i = makeConstant(compiler, (Value) {VAL_BOOL, { .boolean = 1 }});

            writeChunk(compiler->chunk, OP_CONSTANT, token);
            writeChunk(compiler->chunk, i, token);
            break;
        }
        case TOKEN_FALSE: {
            compiler->canAssign = false;
            uint8_t i = makeConstant(compiler, (Value) {VAL_BOOL, { .boolean = 0 }});

            writeChunk(compiler->chunk, OP_CONSTANT, token);
            writeChunk(compiler->chunk, i, token);
            break;
        }
        case TOKEN_NIL: {
            compiler->canAssign = false;
            uint8_t i = makeConstant(compiler, (Value) {VAL_NIL, { .number = 0 }});

            writeChunk(compiler->chunk, OP_CONSTANT, token);
            writeChunk(compiler->chunk, i, token);
            break;
        }
        case TOKEN_MINUS:
        case TOKEN_BANG: {
            compiler->canAssign = false;
            int bp[2];
            getPrefixBP(bp, token.type);

            expression(compiler, bp[1]);

            OpCode opCode = token.type == TOKEN_MINUS? OP_NEGATE: OP_BANG;

            writeChunk(compiler->chunk, opCode, token);
            break;
        }
        default:
            errorAt(compiler, &token, "Expected an expression");
    }

    while (peek(compiler).type != TOKEN_EOF) {
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

        if (operator.type == TOKEN_EQUAL) errorAt(compiler, &operator, "Bad assignment target");

        if (operator.type == TOKEN_SEMICOLON) break;

        OpCode opCode;

        switch (operator.type) {
            case TOKEN_OR: opCode = OP_OR; break;
            case TOKEN_AND: opCode = OP_AND; break;
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
            default: {
                errorAt(compiler, &operator, "Unexpected token");
            }
        }

        int bp[2];
        getInfixBP(bp, operator.type);

        if (minBP > bp[0]) break;

        next(compiler);

        expression(compiler, bp[1]);

        writeChunk(compiler->chunk, opCode, operator);
    }
}

static void statement(Compiler* compiler) {
    expression(compiler, 0);

    compiler->canAssign  = true;

    consume(compiler, TOKEN_SEMICOLON, "Expected ';' after the statement");
    writeChunk(compiler->chunk, OP_POP, compiler->previous);
}

static void declaration(Compiler* compiler) {
    if (match(compiler, TOKEN_VAR)) {
        Token var = compiler->previous;

        consume(compiler, TOKEN_IDENTIFIER, "Expected the name of the variable after 'var'");

        Token name = compiler->previous;

        if (match(compiler, TOKEN_EQUAL)) {
            expression(compiler, 0);
        } else {
            writeChunk(compiler->chunk, OP_NIL, name);
        }

        consume(compiler, TOKEN_SEMICOLON, "Expected ';' after the declaration");

        writeChunk(compiler->chunk, OP_DEFINE_GLOBAL, var);
        emitIdentifier(compiler, name.start, name.length, &var);
    } else statement(compiler);
}

bool compile(Compiler* compiler) {
    while (!atEnd(compiler)) {
        declaration(compiler);
    }

    return !compiler->hadError;
}