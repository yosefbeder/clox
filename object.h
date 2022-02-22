#ifndef clox_object_h
#define clox_object_h

#include "common.h"

typedef enum {
    OBJ_STRING
} ObjType;

struct Obj;
typedef struct {
    ObjType type;
    struct Obj* next;
} Obj;

typedef struct {
    Obj obj;
    int length;
    char* chars;
} ObjString;

struct Vm;

Obj* allocateObj(struct Vm*, size_t, ObjType);

ObjString* allocateObjString(struct Vm*, char*, int);

void freeObj(Obj*);

#endif