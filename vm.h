#ifndef clox_vm_h
#define clox_vm_h

#include "common.h"
#include "chunk.h"
#include "value.h"
#include "object.h"

#define STACK_MAX 256

typedef struct Vm {
    Value stack[STACK_MAX];
    Value* stackTop;
    Obj* objects;
} Vm;

void initVm(Vm*);

Result runChunk(Vm*, Chunk*);

void freeVm(Vm*);

#endif