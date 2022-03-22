#include "memory.h"

void *reallocate(void *pointer, size_t oldSize, size_t newSize)
{
    if (newSize > oldSize)
    {
#ifdef STRESS_TEST_GC
        collectGarbage();
#endif
    }

    if (newSize == 0)
    {
        free(pointer);
        return NULL;
    }

    void *result = realloc(pointer, newSize);

    if (result == NULL)
    {
        printf("Falied failed allocating memory");
        exit(71);
    }

    return result;
}

static void pushGray();

static void markObj(Obj *obj)
{
    if (obj == NULL || obj->marked)
        return;

    obj->marked = true;
    pushGray(obj);
#ifdef DEBUG_GC
    printf("Marked %p (", obj);
    printValue(&OBJ(obj));
    printf(")\n");
#endif
}

static void markValue(Value *value)
{
    if (IS_OBJ(value))
        markObj(AS_OBJ(value));
}

static void markHashMap(HashMap *hashMap)
{
    if (hashMap->count == 0)
        return;

    for (int i = 0; i < hashMap->capacity; i++)
    {
        Entry *entry = &hashMap->entries[i];

        if (entry->isTombstone)
            continue;

        markObj((Obj *)entry->key);

        markValue(&entry->value);
    }
}

static void markCompilerRoots()
{
    Compiler *curCompiler = &compiler;

    while (curCompiler != NULL)
    {
        markObj((Obj *)curCompiler->function);
        curCompiler = curCompiler->enclosing;
    }
}

static void markVmRoots()
{
    markHashMap(&vm.globals);

    for (Value *slot = vm.stack; slot != vm.stackTop; slot++)
    {
        markValue(slot);
    }

    for (int i = 0; i < vm.frameCount; i++)
    {
        markObj((Obj *)vm.frames[i].closure);
    }

    for (ObjUpValue *upValue = vm.openUpValues; upValue != NULL; upValue = upValue->next)
    {
        markObj((Obj *)upValue);
    }
}

static void pushGray(Obj *obj)
{
    if (vm.grayCount == vm.grayCapacity)
    {
        vm.grayCapacity = GROW_CAPACITY(vm.grayCapacity);
        vm.gray = realloc(vm.gray, vm.grayCapacity * sizeof(Obj **));
    }

    vm.gray[vm.grayCount++] = obj;
}

// pushes all of the objects an object reference to `vm.gray`
static void blankenObj(Obj *obj)
{
    switch (obj->type)
    {
    case OBJ_FUNCTION:
    {
        ObjFunction *function = (ObjFunction *)obj;

        markObj((Obj *)function->name);

        for (int i = 0; i < function->chunk.constants.count; i++)
        {
            Value *value = &function->chunk.constants.values[i];
            markValue(value);
        }

        break;
    }
    case OBJ_CLOSURE:
    {
        ObjClosure *closure = (ObjClosure *)obj;

        markObj((Obj *)closure->function);

        for (int i = 0; i < closure->upValuesCount; i++)
        {
            ObjUpValue *upValue = closure->upValues[i];

            markObj((Obj *)upValue);
        }

        break;
    }
    case OBJ_UPVALUE:
    {
        ObjUpValue *upValue = (ObjUpValue *)obj;

        markValue(upValue->location);

        break;
    }
    default:;
    }
}

static void traceReferences()
{
    while (vm.grayCount > 0)
    {
        Obj *obj = vm.gray[--vm.grayCount];

        blankenObj(obj);
    }
}

static void freeObj(Obj *obj)
{
#ifdef DEBUG_GC
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

static void sweep()
{
    Obj *prev = NULL;
    Obj *cur = vm.objects;

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
                vm.objects = cur->next;
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

void collectGarbage()
{
#ifdef DEBUG_GC
    printf("---\nCollecting garbage... 🚛🗑️\n---\n");
#endif

    markVmRoots();

    markCompilerRoots();

    traceReferences();

    sweep();

#ifdef DEBUG_GC
    printf("---\nEverything got cleaned up 👍\n---\n");
#endif
}
