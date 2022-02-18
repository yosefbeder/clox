#include "compiler.h"
#include "error.h"
#include <string.h>
#include "object.h"

void errorAt(Compiler* compiler, Token* token, char msg[]) {
    if (compiler->panicMode) return; // We avoid throwing meaningless errors until we recover

    compiler->panicMode = 1;
    compiler->hadError = 1;
    
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

void emitNumber(Compiler* compiler, char* s, Token* token) {
    double value = strtod(s, NULL);
    uint8_t i = makeConstant(compiler, (Value) {VAL_NUMBER, { .number = value }});

    writeChunk(compiler->chunk, OP_CONSTANT, *token);
    writeChunk(compiler->chunk, i, *token);
}

void emitString(Compiler* compiler, char* s, int length, Token* token) {
    char* chars = malloc((length + 1) * sizeof(char));

    strcpy(chars, s);

    chars[length] = '\0';

    ObjString* objString = allocateObjString(length, chars);

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
        case TOKEN_OR:
            bp[0] = 5;
            bp[1] = 6;
        case TOKEN_AND:
            bp[0] = 7;
            bp[1] = 8;
        case TOKEN_EQUAL_EQUAL:
        case TOKEN_BANG_EQUAL:
            bp[0] = 9;
            bp[1] = 10;
        case TOKEN_GREATER:
        case TOKEN_GREATER_EQUAL:
        case TOKEN_LESS:
        case TOKEN_LESS_EQUAL:
            bp[0] = 11;
            bp[1] = 12;
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

    while (1) {
        compiler->current = scanToken(compiler->scanner);

        if (compiler->current.type != TOKEN_ERROR) break;

        errorAt(compiler, &compiler->current, compiler->current.errorMsg);
    }
}

void initCompiler(Compiler* compiler, Scanner* scanner, Chunk* chunk) {
    compiler->scanner = scanner;
    compiler->chunk = chunk;
    compiler->hadError = 0;
    compiler->panicMode = 0;
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

int compile(Compiler* compiler, int minBP) {
    Token token = next(compiler);

    switch (token.type) {
        case TOKEN_LEFT_PAREN:
            compiler->groupingDepth++;
            compile(compiler, 0);
            consume(compiler, TOKEN_RIGHT_PAREN, "Expected ')' after the group");
            compiler->groupingDepth--;
            break;
        case TOKEN_NUMBER: {
            emitNumber(compiler, token.start, &token);
            break;
        }
        case TOKEN_STRING: {
            emitString(compiler, token.start + 1, token.length - 2, &token);
            break;
        }
        case TOKEN_TEMPLATE_HEAD: {
            compiler->stringDepth++;

            // emit the head
            emitString(compiler, token.start + 1, token.length - 3, &token);

            // emit the expression
            compile(compiler, 0);

            // add them
            writeChunk(compiler->chunk, OP_ADD, token);

            // emitting the middle
            while (peek(compiler).type == TOKEN_TEMPLATE_MIDDLE) {
                Token token = next(compiler);

                // emit the string
                emitString(compiler, token.start + 1, token.length - 3, &token);

                // emit the expression
                compile(compiler, 0);
                
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
            uint8_t i = makeConstant(compiler, (Value) {VAL_BOOL, { .boolean = 1 }});

            writeChunk(compiler->chunk, OP_CONSTANT, token);
            writeChunk(compiler->chunk, i, token);
            break;
        }
        case TOKEN_FALSE: {
            uint8_t i = makeConstant(compiler, (Value) {VAL_BOOL, { .boolean = 0 }});

            writeChunk(compiler->chunk, OP_CONSTANT, token);
            writeChunk(compiler->chunk, i, token);
            break;
        }
        case TOKEN_NIL: {
            uint8_t i = makeConstant(compiler, (Value) {VAL_NIL, { .number = 0 }});

            writeChunk(compiler->chunk, OP_CONSTANT, token);
            writeChunk(compiler->chunk, i, token);
            break;
        }
        case TOKEN_MINUS:
        case TOKEN_BANG: {
            int bp[2];
            getPrefixBP(bp, token.type);

            compile(compiler, bp[1]);

            OpCode opCode = token.type == TOKEN_MINUS? OP_NEGATE: OP_BANG;

            writeChunk(compiler->chunk, opCode, token);
            break;
        }
        default:
            errorAt(compiler, &token, "Expected an expression");
    }

    while (peek(compiler).type != TOKEN_EOF) {
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

        compile(compiler, bp[1]);

        writeChunk(compiler->chunk, opCode, operator);
    }

    return !compiler->hadError;
}