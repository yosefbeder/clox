#ifndef clox_vm_h
#define clox_vm_h

#include "common.h"
#include "chunk.h"
#include "value.h"
#define STACK_MAX 256

typedef struct {
    Value stack[STACK_MAX];
    Value* stackTop;
} Vm;

void initVm(Vm*);

Result runChunk(Vm*, Chunk*);

#endif