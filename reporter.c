#include "reporter.h"

static char *reportTypeToString(ReportType type)
{
    switch (type)
    {
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
void report(ReportType type, Token *token, char msg[])
{
    puts("\n---");

    int pos[2];
    getTokenPos(pos, token);

    printf("%s(%d:%d): %s\n", reportTypeToString(type), pos[0], pos[1], msg);

    char *lineStart = token->start - (pos[1] - 1);
    int i, lineLength;

    i = lineLength = 0;

    while (1)
    {
        i++;
        lineLength++;
        if (lineStart[i] == '\0' || lineStart[i] == '\n')
            break;
    }

    printf("%.*s\n", lineLength, lineStart, pos[0]);
    printf("%*c", pos[1], '^');
    for (i = 0; i < token->length - 1; i++)
    {
        putchar('^');
    }

    putchar('\n');

    if (type == REPORT_RUNTIME_ERROR)
    {
        for (int i = vm.frameCount - 1; i >= 1; i--)
        {
            CallFrame *frame = &vm.frames[i];
            CallFrame *parentFrame = &vm.frames[i - 1];
            Token token = parentFrame->closure->function->chunk.tokenArr.tokens[(int)(parentFrame->ip - parentFrame->closure->function->chunk.code - 1)];

            if (frame->closure->function->name->chars)
            {
                int pos[2];

                getTokenPos(pos, &token);

                printf("in %s [%d:%d]\n", frame->closure->function->name->chars, pos[0], pos[1]);
            }
        }
    }

    puts("---\n");
}