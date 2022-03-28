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
    size_t allocatedBytes;
    size_t nextVm;
} Vm;

void initVm();

bool call(Value, int);

Result run();

void freeVm();

void push(Value value);

Value pop();

extern Vm vm;

#endif