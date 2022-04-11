#include "memory.h"
#include "chunk.h"

void *reallocate(void *pointer, size_t oldSize, size_t newSize)
{
    vm.allocatedBytes -= oldSize;
    vm.allocatedBytes += newSize;

    if (newSize > oldSize)
    {
#ifdef STRESS_TEST_GC
        collectGarbage();
#else
        if (vm.allocatedBytes > vm.nextVm)
        {
            collectGarbage();
            vm.nextVm *= GC_GROW_FACTOR;
        }
#endif
    }

    if (newSize == 0)
    {
#ifdef DEBUG_GC
        printf("Freed %llu bytes (%p)\n", oldSize, pointer);
#endif
        free(pointer);
        return NULL;
    }

    void *result = realloc(pointer, newSize);

    if (result == NULL)
    {
        printf("Falied failed allocating memory\n");
        exit(71);
    }

#ifdef DEBUG_GC
    printf("Allocated %llu bytes (%p)\n", newSize, result);
#endif

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
    printValue(OBJ(obj));
    printf(")\n");
#endif
}

static void markValue(Value value)
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

        markValue(entry->key);

        markValue(entry->value);
    }
}

static void markArr(Value *arr, size_t length)
{
    for (int i = 0; i < length; i++)
    {
        Value value = arr[i];

        markValue(value);
    }
}

static void markCompilerRoots()
{
    Compiler *curCompiler = &compiler;

    while (curCompiler != NULL)
    {
        ObjFunction *function = curCompiler->function;

        if (function != NULL)
        {
            markObj((Obj *)function);
            markArr(function->chunk.constants.values, function->chunk.constants.count);
        }

        curCompiler = curCompiler->enclosing;
    }
}

static void markVmRoots()
{
    markHashMap(&vm.globals);

    markArr(vm.stack, vm.stackTop - vm.stack);

    for (int i = 0; i < vm.frameCount; i++)
        markObj((Obj *)vm.frames[i].closure);

    for (ObjUpValue *upValue = vm.openUpValues; upValue != NULL; upValue = upValue->next)
        markObj((Obj *)upValue);
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

        markValue(function->name);

        for (int i = 0; i < function->chunk.constants.count; i++)
        {
            Value value = function->chunk.constants.values[i];
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

        markValue(*upValue->location);

        break;
    }
    case OBJ_CLASS:
    {
        ObjClass *klass = (ObjClass *)obj;

        markValue(klass->name);
        markObj((Obj *)klass->superclass);
        markObj((Obj *)klass->initializer);
        markHashMap(&klass->methods);

        break;
    }
    case OBJ_INSTANCE:
    {
        ObjInstance *instance = (ObjInstance *)obj;

        markObj((Obj *)instance->klass);
        markHashMap(&instance->fields);

        break;
    }
    case OBJ_BOUND_METHOD:
    {
        ObjBoundMethod *boundMethod = (ObjBoundMethod *)obj;

        markObj((Obj *)boundMethod->method);
        markObj((Obj *)boundMethod->instance);

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

static void freeTokenArr(TokenArr *tokenArr)
{
    FREE_ARRAY(Token, tokenArr->tokens, tokenArr->capacity);

    initTokenArr(tokenArr);
}

static void freeValueArr(ValueArr *valueArr)
{
    FREE_ARRAY(Value, valueArr->values, valueArr->capacity);

    initValueArr(valueArr);
}

static void freeChunk(Chunk *chunk)
{
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    freeValueArr(&chunk->constants);
    freeTokenArr(&chunk->tokenArr);

    initChunk(chunk);
}

static void freeHashMap(HashMap *hashMap)
{
    FREE_ARRAY(Entry, hashMap->entries, hashMap->capacity);

    initHashMap(hashMap);
}

static void freeObj(Obj *obj)
{
    if (obj == NULL)
        return;

#ifdef DEBUG_GC
    printf("Freed %p (", obj);
    printValue(OBJ(obj));
    printf(")\n");
#endif

    switch (obj->type)
    {
    case OBJ_STRING:
    {
        ObjString *string = (ObjString *)obj;
        FREE_ARRAY(char, string->chars, string->length);
        break;
    }

    case OBJ_FUNCTION:
        freeChunk(&((ObjFunction *)obj)->chunk);
        break;
    case OBJ_CLOSURE:
    {
        ObjClosure *closure = (ObjClosure *)obj;
        FREE_ARRAY(ObjUpValue *, closure->upValues, closure->upValuesCount);
        break;
    }
    case OBJ_CLASS:
    {
        ObjClass *klass = (ObjClass *)obj;
        freeHashMap(&klass->methods);
        break;
    }
    case OBJ_INSTANCE:
    {
        ObjInstance *instance = (ObjInstance *)obj;
        freeHashMap(&instance->fields);
        break;
    }
    default:;
    }
}

// Removes any entry that's not marked (its key)
static void removeWhiteInternedStrings()
{
    for (int i = 0; i < vm.strings.capacity; i++)
    {
        Entry *entry = &vm.strings.entries[i];

        if (IS_NIL(entry->key) || entry->isTombstone)
            continue;

        if (!AS_OBJ(entry->key)->marked)
        {
#ifdef DEBUG_STRINGS_INTERNING
            printf("'%s' got removed from interned strings\n", AS_STRING_OBJ(entry->key)->chars);
#endif
            hashMapRemove(&vm.strings, entry->key);
        }
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
                prev->next = cur->next;
            else
                vm.objects = cur->next;

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
    printf("---\nCollecting garbage... 🚛 (%llu bytes allocated by the GC)\n---\n", vm.allocatedBytes);
#endif

    markVmRoots();

    markCompilerRoots();

    traceReferences();

    removeWhiteInternedStrings();

    sweep();

#ifdef DEBUG_GC
    printf("---\nEverything unused object is cleand up now 👍 (%llu bytes allocated by the GC)\n---\n", vm.allocatedBytes);
#endif
}
