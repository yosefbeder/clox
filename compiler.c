#include "compiler.h"

void initCompiler(Compiler* compiler, Scanner* scanner, Chunk* chunk) {
    compiler->scanner = scanner;
    compiler->chunk = chunk;
    compiler->hadError = 0;
    compiler->panicMode = 0;
}

void errorAt(Compiler* compiler, Token* token, char msg[]) {
    if (compiler->panicMode) return; // We avoid throwing meaningless errors until we recover

    compiler->panicMode = 1;
    compiler->hadError = 1;
    
    if (token->type == TOKEN_ERROR) {
        printf("ScanError: %s", msg);
    } else {

        int pos[2];
        getTokenPos(compiler->scanner, token, pos);

        printf("ParseError: %s\n", msg);

        char* lineStart = token->start - (pos[1] - 1);
        int i, lineLength;

        i = lineLength = 0;

        while (1) {
            i++;
            lineLength++; 
            if (lineStart[i] == '\0' || lineStart[i] == '\n') break;
        }

        printf("%-3d | %.*s\n", pos[0], lineLength, lineStart);
        printf("%*c\n", pos[1] + 6, '^');
    }
};

void advance(Compiler* compiler) {
    compiler->previous = compiler->current;

    while (1) {
        compiler->current = scanToken(compiler->scanner);

        if (compiler->current.type != TOKEN_ERROR) break;

        errorAt(compiler, &compiler->current, compiler->current.start);
    }
}

void consume(Compiler* compiler, TokenType type, char msg[]) {
    if (compiler->current.type == type) {
        advance(compiler);
        return;
    }

    errorAt(compiler, &compiler->current, msg);
}

void emitByte(Compiler* compiler, uint8_t byte) {
    writeChunk(compiler->chunk, byte, compiler->previous);
}

void emitConstant(Compiler* compiler, double value) {
    uint8_t i = addConstant(compiler->chunk, value);

    if (i > UINT8_MAX) {
        errorAt(compiler, &compiler->previous, "Too many constants in one chunk");
        return;
    }

    emitByte(compiler, OP_CONSTANT);
    emitByte(compiler, i);
}