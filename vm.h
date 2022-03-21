#ifndef clox_vm_h
#define clox_vm_h

#include "common.h"
#include "chunk.h"
#include "value.h"
#include "object.h"
#include "hashmap.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * (UINT8_MAX + 1))

typedef struct
{
    ObjClosure *closure;
    uint8_t *ip;
    Value *slots;
} CallFrame;

typedef struct Vm
{
    struct Compiler *compiler;

    CallFrame frames[FRAMES_MAX];
    int frameCount;

    Value stack[STACK_MAX];
    Value *stackTop;
    Obj *objects;
    ObjUpValue *openUpValues;
    HashMap globals;

    // these three fields are only used in the garbage-collector
    int grayCount;
    int grayCapacity;
    Obj **gray;
} Vm;

void printValue(Value *);

void initVm(Vm *, struct Compiler *);

bool call(Vm *, Value *, int);

Result run(Vm *);

void freeVm(Vm *);

#endif