#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "chunk.h"
#include "value.h"

typedef enum {
    OBJ_STRING,
    OBJ_FUNCTION,
} ObjType;

typedef struct Obj {
    ObjType type;
    struct Obj* next;
} Obj;

typedef struct {
    Obj obj;
    int length;
    char* chars;
} ObjString;

#define IS_STRING(val) (val->type == VAL_OBJ && val->as.obj->type == OBJ_STRING)
#define AS_STRING(val) ((ObjString*) val->as.obj)
#define STRING(val) (Value) {VAL_OBJ, { .obj = (Obj*) val }}

typedef struct {
  Obj obj;
  int arity;
  Chunk chunk;
  ObjString* name;
} ObjFunction;

#define IS_FUNCTION(val) (val->type == VAL_OBJ && val->as.obj->type == OBJ_FUNCTION)
#define AS_FUNCTION(val) ((ObjFunction*) val->as.obj)
#define FUNCTION(val) (Value) {VAL_OBJ, { .obj = (Obj*) val }}

struct Vm;

Obj* allocateObj(struct Vm*, size_t, ObjType);

ObjString* allocateObjString(struct Vm*, char*);

ObjFunction* allocateObjFunction(struct Vm*);

void freeObj(Obj*);

#endif