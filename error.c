#include "error.h"

void reportError(ErrorType type, Token* token, char msg[]) {
    puts("\n---");

    int pos[2];
    getTokenPos(pos, token);

    printf("%s(%d:%d): %s\n", type == ERROR_SCAN? "ScanError": type == ERROR_PARSE? "ParseError": "RuntimeError", pos[0], pos[1], msg);

    char* lineStart = token->start - (pos[1] - 1);
    int i, lineLength;

    i = lineLength = 0;

    while (1) {
        i++;
        lineLength++; 
        if (lineStart[i] == '\0' || lineStart[i] == '\n') break;
    }

    printf("%.*s\n", lineLength, lineStart, pos[0]);
    printf("%*c", pos[1], '^');
    char c = '^';
    for (i = 0; i < token->length - 1; i ++) {
        putchar(c);
    }
    puts("\n---\n");
}