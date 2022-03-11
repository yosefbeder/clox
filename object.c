#include "object.h"
#include "value.h"
#include "vm.h"
#include <string.h>

Obj *allocateObj(struct Vm *vm, size_t size, ObjType type)
{
    Obj *ptr = malloc(size);

    ptr->type = type;
    ptr->next = (struct Obj *)vm->objects;
    vm->objects = ptr;

    return ptr;
}

char *allocateString(char *s, int length)
{
    char *chars = malloc(length + 1);

    strncpy(chars, s, length);

    chars[length] = '\0';

    return chars;
}

ObjString *allocateObjString(struct Vm *vm, char *s, int length)
{
    char *chars = allocateString(s, length);

    ObjString *ptr = (ObjString *)allocateObj(vm, sizeof(ObjString), OBJ_STRING);

    ptr->chars = chars;
    ptr->length = strlen(chars);

    return ptr;
}

ObjFunction *allocateObjFunction(struct Vm *vm)
{
    ObjFunction *ptr = (ObjFunction *)allocateObj(vm, sizeof(ObjFunction), OBJ_FUNCTION);

    ptr->arity = 0;
    ptr->name = NULL;

    initChunk(&ptr->chunk);

    return ptr;
}

ObjNative *allocateObjNative(struct Vm *vm, uint8_t arity, NativeFun function)
{
    ObjNative *ptr = (ObjNative *)allocateObj(vm, sizeof(ObjNative), OBJ_NATIVE);

    ptr->arity = arity;
    ptr->function = function;

    return ptr;
}

ObjClosure *allocateObjClosure(struct Vm *vm, ObjFunction *function, uint8_t upValuesCount)
{
    ObjClosure *ptr = (ObjClosure *)allocateObj(vm, sizeof(ObjClosure), OBJ_CLOSURE);

    ptr->function = function;
    ptr->upValuesCount = upValuesCount;
    ptr->upValues = malloc(upValuesCount * sizeof(ObjUpValue));

    return ptr;
}

ObjUpValue *allocateObjUpValue(struct Vm *vm, Value *value)
{
    ObjUpValue *ptr = (ObjUpValue *)allocateObj(vm, sizeof(ObjUpValue), OBJ_UPVALUE);

    ptr->location = value;

    return ptr;
}

void freeObj(Obj *obj)
{
    switch (obj->type)
    {
    case OBJ_STRING:
        free(((ObjString *)obj)->chars);
        break;
    case OBJ_FUNCTION:
        freeChunk(&((ObjFunction *)obj)->chunk);
        break;
    case OBJ_CLOSURE:
        free(((ObjClosure *)obj)->upValues);
        break;
    default:; // native functsions and upvalues don't "own" data that should be freed up
    }
}