#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "chunk.h"
#include "value.h"

typedef enum
{
    OBJ_STRING,
    OBJ_FUNCTION,
    OBJ_NATIVE,
    OBJ_CLOSURE,
    OBJ_UPVALUE,
} ObjType;

typedef struct Obj
{
    ObjType type;
    bool marked;
    struct Obj *next;
} Obj;

#define IS_OBJ(val) (val->type == VAL_OBJ)
#define AS_OBJ(val) ((Obj *)val->as.obj)
#define OBJ(val)                       \
    (Value)                            \
    {                                  \
        VAL_OBJ, { .obj = (Obj *)val } \
    }

typedef struct
{
    Obj obj;
    int length;
    char *chars;
} ObjString;

#define IS_STRING(val) (val->type == VAL_OBJ && val->as.obj->type == OBJ_STRING)
#define AS_STRING(val) ((ObjString *)val->as.obj)
#define STRING(val)                    \
    (Value)                            \
    {                                  \
        VAL_OBJ, { .obj = (Obj *)val } \
    }

typedef struct
{
    Obj obj;
    uint8_t arity;
    Chunk chunk;
    ObjString *name;
} ObjFunction;

#define IS_FUNCTION(val) (val->type == VAL_OBJ && val->as.obj->type == OBJ_FUNCTION)
#define AS_FUNCTION(val) ((ObjFunction *)val->as.obj)
#define FUNCTION(val)                  \
    (Value)                            \
    {                                  \
        VAL_OBJ, { .obj = (Obj *)val } \
    }

struct Vm;
struct Compiler;

typedef bool (*NativeFun)(struct Vm *vm, Value *returnValue, Value *args);

typedef struct
{
    Obj obj;
    uint8_t arity;
    NativeFun function;
} ObjNative;

#define IS_NATIVE(val) (val->type == VAL_OBJ && val->as.obj->type == OBJ_NATIVE)
#define AS_NATIVE(val) ((ObjNative *)val->as.obj)
#define NATIVE(val)                    \
    (Value)                            \
    {                                  \
        VAL_OBJ, { .obj = (Obj *)val } \
    }

typedef struct ObjUpValue
{
    Obj obj;
    Value *location;
    Value closed;
    struct ObjUpValue *next;
} ObjUpValue;

#define IS_UPVALUE(val) (val->type == VAL_OBJ && val->as.obj->type == OBJ_UPVALUE)
#define AS_UPVALUE(val) ((ObjUpValue *)val->as.obj)
#define UPVALUE(val)                   \
    (Value)                            \
    {                                  \
        VAL_OBJ, { .obj = (Obj *)val } \
    }

typedef struct
{
    Obj obj;
    ObjFunction *function;
    uint8_t upValuesCount;
    ObjUpValue **upValues;
} ObjClosure;

#define IS_CLOSURE(val) (val->type == VAL_OBJ && val->as.obj->type == OBJ_CLOSURE)
#define AS_CLOSURE(val) ((ObjClosure *)val->as.obj)
#define CLOSURE(val)                   \
    (Value)                            \
    {                                  \
        VAL_OBJ, { .obj = (Obj *)val } \
    }

ObjString *allocateObjString(struct Vm *, struct Compiler *, char *, int);

ObjFunction *allocateObjFunction(struct Vm *, struct Compiler *);

ObjNative *allocateObjNative(struct Vm *, struct Compiler *, uint8_t, NativeFun);

ObjUpValue *allocateObjUpValue(struct Vm *, struct Compiler *, Value *);

ObjClosure *allocateObjClosure(struct Vm *, struct Compiler *, ObjFunction *, uint8_t);

#endif