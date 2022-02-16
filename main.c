#include "common.h"
#include "scanner.h"
#include "debug.h"
#include "compiler.h"
#include "vm.h"

#define LINE_LIMIT 1024

Result run(char*);

void runRepl();

void runFile(char[]);

int main(int argc, char* argv[]) {
    if (argc == 1) {
        runRepl();
    } else if (argc == 2) {
        runFile(argv[1]);
    } else {
        return 64;
    }

    return 0;
}

Result run(char* source) {
    Chunk chunk;
    initChunk(&chunk);

    Scanner scanner;
    initScanner(&scanner, source);

    Compiler compiler;
    initCompiler(&compiler, &scanner, &chunk);

    if (!compile(&compiler, 0)) {
        freeChunk(&chunk);
        return RESULT_COMPILE_ERROR;
    }

    writeChunk(&chunk, OP_RETURN, compiler.current);

    disassembleChunk(&chunk);

    Vm vm;
    initVm(&vm);

    Result result = runChunk(&vm, &chunk);

    freeChunk(&chunk);

    return RESULT_SUCCESS;
}

int nextLine(char line[], int limit) {
    char c;
    int i = 0;

    while (i < limit - 1 && (c = getchar()) != EOF && c != '\n')
        line[i++] = c;


    line[i] = '\0';

    return i;
}

void runRepl() {
    char line[LINE_LIMIT];

    while (nextLine(line, LINE_LIMIT)) {
        run(line);
        line[0] = '\0';
    }
}

char* readFile(char path[]) {
    FILE* ptr = fopen(path, "r");

    if (ptr == NULL) {
        exit(65);
    }

    fseek(ptr, 0, SEEK_END);
    int length = ftell(ptr);
    fseek(ptr, 0, SEEK_SET);

    char* buffer = malloc(length + 1);
    int readbytes = fread(buffer, sizeof(char), length, ptr);
    buffer[readbytes] = '\0';

    fclose(ptr);

    return buffer;
}

void runFile(char path[]) {
    char* buffer = readFile(path);

    run(buffer);

    free(buffer);
}
