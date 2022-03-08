#ifndef clox_vm_h
#define clox_vm_h

#include "common.h"
#include "chunk.h"
#include "value.h"
#include "object.h"
#include "hashmap.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * (UINT8_MAX + 1))

typedef struct {
    ObjFunction* function;
    uint8_t* ip;
    Value* slots;
} CallFrame;

typedef struct Vm {
    CallFrame frames[FRAMES_MAX];
    int frameCount;

    Value stack[STACK_MAX];
    Value* stackTop;
    Obj* objects;
    HashMap globals;
} Vm;

void printValue(Value*);

void initVm(Vm*);

bool call(Vm*, Obj*, int);

Result run(Vm*);

void freeVm(Vm*);

#endif