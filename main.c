#include "common.h"
#include "scanner.h"
#include "debug.h"
#include "compiler.h"
#include "vm.h"

#define LINE_LIMIT 1024

void runRepl();

void runFile(char[]);

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        runRepl();
    }
    else if (argc == 2)
    {
        runFile(argv[1]);
    }
    else
    {
        return 64;
    }

    return 0;
}

int nextLine(char line[], int limit)
{
    char c;
    int i = 0;

    while (i < limit - 1 && (c = getchar()) != EOF && c != '\n')
        line[i++] = c;

    line[i] = '\0';

    return i;
}

void runRepl()
{
    char line[LINE_LIMIT];
    initVm();

    while (nextLine(line, LINE_LIMIT))
    {
        Scanner scanner;
        initScanner(&scanner, line);

        ObjFunction *script = compile(&scanner);

        if (script == NULL)
            continue;

        ObjClosure *closure = allocateObjClosure(script, 0);

        push(OBJ(closure));

        call(OBJ(closure), 0);

        run();

        line[0] = '\0';
    }

    freeVm();
}

char *readFile(char path[])
{
    FILE *ptr = fopen(path, "r");

    if (ptr == NULL)
    {
        printf("There's no file with such path");
        exit(74);
    }

    fseek(ptr, 0, SEEK_END);
    int length = ftell(ptr);
    fseek(ptr, 0, SEEK_SET);

    char *buffer = malloc(length + 1);
    int readbytes = fread(buffer, sizeof(char), length, ptr);
    buffer[readbytes] = '\0';

    fclose(ptr);

    return buffer;
}

void runFile(char path[])
{
    char *buffer = readFile(path);

    initVm();

    Scanner scanner;
    initScanner(&scanner, buffer);

    ObjFunction *script = compile(&scanner);

    if (script == NULL)
        exit(65);

    ObjClosure *closure = allocateObjClosure(script, 0);

    push(OBJ(closure));

    call(OBJ(closure), 0);

    run();

    free(buffer);
    freeVm();
}
