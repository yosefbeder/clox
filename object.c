#include "object.h"
#include "value.h"
#include "memory.h"
#include <string.h>

static void postAllocation(Obj *ptr)
{
#ifdef DEBUG_GC
    printf("Allocated %p (", ptr);
    printValue(&OBJ(ptr));
    printf(")\n");
#endif
}

static Obj *allocateObj(struct Vm *vm, struct Compiler *compiler, size_t size, ObjType type)
{
    Obj *ptr = reallocate(vm, compiler, NULL, 0, size);

    ptr->type = type;
    ptr->marked = false;
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

ObjString *allocateObjString(struct Vm *vm, struct Compiler *compiler, char *s, int length)
{
    char *chars = allocateString(s, length);

    ObjString *ptr = (ObjString *)allocateObj(vm, compiler, sizeof(ObjString), OBJ_STRING);

    ptr->chars = chars;
    ptr->length = strlen(chars);

    postAllocation((Obj *)ptr);

    return ptr;
}

ObjFunction *allocateObjFunction(struct Vm *vm, struct Compiler *compiler)
{
    ObjFunction *ptr = (ObjFunction *)allocateObj(vm, compiler, sizeof(ObjFunction), OBJ_FUNCTION);

    ptr->arity = 0;
    ptr->name = NULL;

    initChunk(&ptr->chunk);

    postAllocation((Obj *)ptr);

    return ptr;
}

ObjNative *allocateObjNative(struct Vm *vm, struct Compiler *compiler, uint8_t arity, NativeFun function)
{
    ObjNative *ptr = (ObjNative *)allocateObj(vm, compiler, sizeof(ObjNative), OBJ_NATIVE);

    ptr->arity = arity;
    ptr->function = function;

    postAllocation((Obj *)ptr);

    return ptr;
}

ObjClosure *allocateObjClosure(struct Vm *vm, struct Compiler *compiler, ObjFunction *function, uint8_t upValuesCount)
{
    ObjClosure *ptr = (ObjClosure *)allocateObj(vm, compiler, sizeof(ObjClosure), OBJ_CLOSURE);

    ptr->function = function;
    ptr->upValuesCount = upValuesCount;
    ptr->upValues = malloc(upValuesCount * sizeof(ObjUpValue *));

    for (int i = 0; i < upValuesCount; i++)
    {
        ptr->upValues[i] = NULL;
    }

    postAllocation((Obj *)ptr);

    return ptr;
}

ObjUpValue *allocateObjUpValue(struct Vm *vm, struct Compiler *compiler, Value *value)
{
    ObjUpValue *ptr = (ObjUpValue *)allocateObj(vm, compiler, sizeof(ObjUpValue), OBJ_UPVALUE);

    ptr->location = value;
    ptr->next = NULL;
    ptr->closed = NIL;

    postAllocation((Obj *)ptr);

    return ptr;
}
