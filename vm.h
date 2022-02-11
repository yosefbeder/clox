#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#define STACK_MAX 256

typedef struct {
    double stack[STACK_MAX];
    double* stackTop;
} Vm;

void initVm(Vm*);

void runChunk(Vm*, Chunk*);

#endif