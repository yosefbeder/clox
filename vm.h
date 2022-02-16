#ifndef clox_vm_h
#define clox_vm_h

#include "common.h"
#include "chunk.h"
#define STACK_MAX 256

typedef struct {
    double stack[STACK_MAX];
    double* stackTop;
} Vm;

void initVm(Vm*);

Result runChunk(Vm*, Chunk*);

#endif