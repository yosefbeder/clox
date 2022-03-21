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

typedef struct
{
    Obj obj;
    int length;
    char *chars;
} ObjString;

#define IS_OBJ_TYPE(val, typ) (AS_OBJ(val)->type == typ)

#define IS_STRING(val) (IS_OBJ(val) && IS_OBJ_TYPE(val, OBJ_STRING))
#define AS_STRING(val) ((ObjString *)AS_OBJ(val))

typedef struct
{
    Obj obj;
    uint8_t arity;
    Chunk chunk;
    ObjString *name;
} ObjFunction;

#define IS_FUNCTION(val) (IS_OBJ(val) && IS_OBJ_TYPE(val, OBJ_FUNCTION))
#define AS_FUNCTION(val) ((ObjFunction *)AS_OBJ(val))

struct Vm;
struct Compiler;

typedef bool (*NativeFun)(struct Vm *vm, Value *returnValue, Value *args);

typedef struct
{
    Obj obj;
    uint8_t arity;
    NativeFun function;
} ObjNative;

#define IS_NATIVE(val) (IS_OBJ(val) && IS_OBJ_TYPE(val, OBJ_NATIVE))
#define AS_NATIVE(val) ((ObjNative *)AS_OBJ(val))

typedef struct ObjUpValue
{
    Obj obj;
    Value *location;
    Value closed;
    struct ObjUpValue *next;
} ObjUpValue;

#define IS_UPVALUE(val) (IS_OBJ(val) && IS_OBJ_TYPE(val, OBJ_UPVALUE))
#define AS_UPVALUE(val) ((ObjUpValue *)AS_OBJ(val))

typedef struct
{
    Obj obj;
    ObjFunction *function;
    uint8_t upValuesCount;
    ObjUpValue **upValues;
} ObjClosure;

#define IS_CLOSURE(val) (IS_OBJ(val) && IS_OBJ_TYPE(val, OBJ_CLOSURE))
#define AS_CLOSURE(val) ((ObjClosure *)AS_OBJ(val))

ObjString *allocateObjString(struct Vm *, struct Compiler *, char *, int);

ObjFunction *allocateObjFunction(struct Vm *, struct Compiler *);

ObjNative *allocateObjNative(struct Vm *, struct Compiler *, uint8_t, NativeFun);

ObjUpValue *allocateObjUpValue(struct Vm *, struct Compiler *, Value *);

ObjClosure *allocateObjClosure(struct Vm *, struct Compiler *, ObjFunction *, uint8_t);

#endif