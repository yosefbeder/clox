#ifndef clox_object_h
#define clox_object_h

#include "common.h"

typedef enum {
    OBJ_STRING
} ObjType;

typedef struct {
    ObjType type;
} Obj;

typedef struct {
    Obj obj;
    int length;
    char* chars;
} ObjString;

Obj* allocateObj(size_t, ObjType);

ObjString* allocateObjString(int length, char* chars);

void freeObj(Obj*);

#endif