#include "object.h"
#include "value.h"
#include "memory.h"
#include <string.h>

static Obj *allocateObj(size_t size, ObjType type)
{
    Obj *ptr = reallocate(NULL, 0, size);

    ptr->type = type;
    ptr->marked = false;
    ptr->next = (struct Obj *)vm.objects;
    vm.objects = ptr;

    return ptr;
}

char *allocateString(char *s, int length)
{
    char *chars = ALLOCATE(char, length + 1);

    strncpy(chars, s, length);

    chars[length] = '\0';

    return chars;
}

ObjString *allocateObjString(char *s, int length)
{
    char *chars = allocateString(s, length);

    ObjString *ptr = (ObjString *)allocateObj(sizeof(ObjString), OBJ_STRING);

    ptr->chars = chars;
    ptr->length = strlen(chars);

#ifdef DEBUG_GC
    printValue(&OBJ(ptr));
    putchar('\n');
#endif

    return ptr;
}

ObjFunction *allocateObjFunction(struct Vm *vm, struct Compiler *compiler)
{
    ObjFunction *ptr = (ObjFunction *)allocateObj(sizeof(ObjFunction), OBJ_FUNCTION);

    ptr->arity = 0;
    ptr->name = NULL;

    initChunk(&ptr->chunk);

#ifdef DEBUG_GC
    printValue(&OBJ(ptr));
    putchar('\n');
#endif

    return ptr;
}

ObjNative *allocateObjNative(uint8_t arity, NativeFun function)
{
    ObjNative *ptr = (ObjNative *)allocateObj(sizeof(ObjNative), OBJ_NATIVE);

    ptr->arity = arity;
    ptr->function = function;

#ifdef DEBUG_GC
    printValue(&OBJ(ptr));
    putchar('\n');
#endif

    return ptr;
}

ObjUpValue *allocateObjUpValue(Value *value)
{
    ObjUpValue *ptr = (ObjUpValue *)allocateObj(sizeof(ObjUpValue), OBJ_UPVALUE);

    ptr->location = value;
    ptr->next = NULL;
    ptr->closed = NIL;

#ifdef DEBUG_GC
    printValue(&OBJ(ptr));
    putchar('\n');
#endif

    return ptr;
}

ObjClosure *allocateObjClosure(ObjFunction *function, uint8_t upValuesCount)
{
    ObjClosure *ptr = (ObjClosure *)allocateObj(sizeof(ObjClosure), OBJ_CLOSURE);

    ptr->function = function;
    ptr->upValuesCount = upValuesCount;
    ptr->upValues = ALLOCATE(ObjUpValue *, upValuesCount);

    for (int i = 0; i < upValuesCount; i++)
    {
        ptr->upValues[i] = NULL;
    }

#ifdef DEBUG_GC
    printValue(&OBJ(ptr));
    putchar('\n');
#endif

    return ptr;
}

ObjClass *allocateObjClass(ObjString *name)
{
    ObjClass *ptr = (ObjClass *)allocateObj(sizeof(ObjClass), OBJ_CLASS);

    ptr->superclass = NULL;
    ptr->name = name;
    initHashMap(&ptr->fields);
    initHashMap(&ptr->methods);

#ifdef DEBUG_GC
    printValue(&OBJ(ptr));
    putchar('\n');
#endif

    return ptr;
}

ObjInstance *allocateObjInstance(ObjClass *klass)
{
    ObjInstance *ptr = (ObjInstance *)allocateObj(sizeof(ObjInstance), OBJ_INSTANCE);

    ptr->klass = klass;
    initHashMap(&ptr->fields);

#ifdef DEBUG_GC
    printValue(&OBJ(ptr));
    putchar('\n');
#endif

    return ptr;
}
