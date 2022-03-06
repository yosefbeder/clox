#include "reporter.h"

static char* reportTypeToString(ReportType type) {
    switch (type) {
        case REPORT_SCAN_ERROR:
            return "ScanError";
        case REPORT_PARSE_ERROR:
            return "ParseError";
        case REPORT_RUNTIME_ERROR:
            return "RuntimeError";
        case REPORT_WARNING:
            return "Warnning";
    }
}

// vm pointers is passed only for runtime errors
void report(ReportType type, Token *token, char msg[], struct Vm* vm)
{
    puts("\n---");

    if (type == REPORT_RUNTIME_ERROR) {
       int i = vm->frameCount - 1;

        while (i >= 1) {
            CallFrame *frame = &vm->frames[i];

            if (frame->function->name->chars) {
                int pos[2];
                getTokenPos(pos, &frame->function->chunk.tokenArr.tokens[(int)(frame->ip - frame->function->chunk.code - 1)]);

                printf("at %s(%d:%d)\n", frame->function->name->chars, pos[0], pos[1]);
            }

            i--;
        }

        putchar('\n');
    }

    int pos[2];
    getTokenPos(pos, token);

    printf("%s(%d:%d): %s\n", reportTypeToString(type), pos[0], pos[1], msg);

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