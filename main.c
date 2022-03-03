#include "common.h"
#include "scanner.h"
#include "debug.h"
#include "compiler.h"
#include "vm.h"

#define LINE_LIMIT 1024

Result runSource(Vm*, char*);

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

Result runSource(Vm* vm, char* source) {
    Scanner scanner;
    initScanner(&scanner, source);

    Compiler compiler;
    initCompiler(&compiler, &scanner, vm, TYPE_SCRIPT);

    ObjFunction* script = compile(&compiler);

    if (script == NULL) {
        return RESULT_COMPILE_ERROR;
    }

    writeChunk(&script->chunk, OP_RETURN, &compiler.current);

    disassembleChunk(&script->chunk);

    vm->frames[vm->frameCount++] = (CallFrame) {script, script->chunk.code, vm->stack};

    return run(vm);
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
    Vm vm;
    initVm(&vm);
    char line[LINE_LIMIT];

    while (nextLine(line, LINE_LIMIT)) {
        runSource(&vm, line);
        line[0] = '\0';
    }

    freeVm(&vm);
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
    Vm vm;
    initVm(&vm);
    char* buffer = readFile(path);

    runSource(&vm, buffer);

    free(buffer);
    freeVm(&vm);
}
