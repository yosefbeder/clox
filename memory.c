#include "memory.h"

static void pushGray(struct Vm *, Obj *);

static void markObj(struct Vm *vm, Obj *obj)
{
    if (obj == NULL || obj->marked)
        return;

    obj->marked = true;
    pushGray(vm, obj);
#ifdef GC_DEBUG_MODE
    printf("Marked %p (", obj);
    printValue(&OBJ(obj));
    printf(")\n");
#endif
}

static void markValue(struct Vm *vm, Value *value)
{
    if (IS_OBJ(value))
        markObj(vm, AS_OBJ(value));
}

static void markHashMap(struct Vm *vm, HashMap *hashMap)
{
    if (hashMap->count == 0)
        return;

    for (int i = 0; i < hashMap->capacity; i++)
    {
        Entry *entry = &hashMap->entries[i];

        if (entry->isTombstone)
            continue;

        markObj(vm, (Obj *)entry->key);

        markValue(vm, &entry->value);
    }
}

static void markCompilerRoots(struct Vm *vm, struct Compiler *compiler)
{
    while (compiler != NULL)
    {
        markObj(vm, (Obj *)compiler->function);
        compiler = compiler->enclosing;
    }
}

static void markVmRoots(struct Vm *vm)
{
    markHashMap(vm, &vm->globals);

    for (Value *slot = vm->stack; slot != vm->stackTop; slot++)
    {
        markValue(vm, slot);
    }

    for (int i = 0; i < vm->frameCount; i++)
    {
        markObj(vm, (Obj *)vm->frames[i].closure);
    }

    for (ObjUpValue *upValue = vm->openUpValues; upValue != NULL; upValue = upValue->next)
    {
        markObj(vm, (Obj *)upValue);
    }
}

static void pushGray(struct Vm *vm, Obj *obj)
{
    if (vm->grayCount == vm->grayCapacity)
    {
        vm->grayCapacity = GROW_CAPACITY(vm->grayCapacity);
        vm->gray = realloc(vm->gray, vm->grayCapacity * sizeof(Obj **));
    }

    vm->gray[vm->grayCount++] = obj;
}

// pushes all of the objects an object reference to `vm->gray`
static void blankenObj(struct Vm *vm, Obj *obj)
{
    switch (obj->type)
    {
    case OBJ_FUNCTION:
    {
        ObjFunction *function = (ObjFunction *)obj;

        markObj(vm, (Obj *)function->name);

        for (int i = 0; i < function->chunk.constants.count; i++)
        {
            Value *value = &function->chunk.constants.values[i];
            markValue(vm, value);
        }

        break;
    }
    case OBJ_CLOSURE:
    {
        ObjClosure *closure = (ObjClosure *)obj;

        markObj(vm, (Obj *)closure->function);

        for (int i = 0; i < closure->upValuesCount; i++)
        {
            ObjUpValue *upValue = closure->upValues[i];

            markObj(vm, (Obj *)upValue);
        }

        break;
    }
    case OBJ_UPVALUE:
    {
        ObjUpValue *upValue = (ObjUpValue *)obj;

        markValue(vm, upValue->location);

        break;
    }
    default:;
    }
}

static void traceReferences(struct Vm *vm)
{
    while (vm->grayCount > 0)
    {
        Obj *obj = vm->gray[--vm->grayCount];

        blankenObj(vm, obj);
    }
}

static void freeObj(Obj *obj)
{
#ifdef GC_DEBUG_MODE
    printf("Freed %p (", obj);
    printValue(&OBJ(obj));
    printf(")\n");
#endif

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

static void sweep(struct Vm *vm)
{
    Obj *prev = NULL;
    Obj *cur = vm->objects;

    while (cur != NULL)
    {
        if (!cur->marked)
        {
            if (prev != NULL)
            {
                prev->next = cur->next;
            }
            else
            {
                vm->objects = cur->next;
            }

            Obj *garbage = cur;
            cur = cur->next;

            freeObj(garbage);
            free(garbage);
        }

        cur->marked = false;

        prev = cur;
        cur = cur->next;
    }
}

void collectGarbage(struct Vm *vm, struct Compiler *compiler)
{
#ifdef GC_DEBUG_MODE
    printf("---\nCollecting garbage... 🚛🗑️\n---\n");
#endif

    markVmRoots(vm);

    markCompilerRoots(vm, compiler);

    traceReferences(vm);

    sweep(vm);

#ifdef GC_DEBUG_MODE
    printf("---\nEverything got cleaned up 👍\n---\n");
#endif
}
