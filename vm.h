#ifndef clox_vm_h
#define clox_vm_h

#include "common.h"
#include "chunk.h"
#include "value.h"
#include "object.h"
#include "hashmap.h"

#define STACK_MAX 256

typedef struct Vm {
    Value stack[STACK_MAX];
    Value* stackTop;
    Obj* objects;
    HashMap globals;
} Vm;

void initVm(Vm*);

Result runChunk(Vm*, Chunk*);

void freeVm(Vm*);

#endif