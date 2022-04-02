#include "vm.h"
#include "reporter.h"
#include "memory.h"
#include <string.h>
#include <time.h>

#ifdef DEBUG_BYTECODE
#include "debug.h"
#endif

Vm vm;

static void runtimeError(char msg[])
{
    CallFrame *frame = &vm.frames[vm.frameCount - 1];

    report(REPORT_RUNTIME_ERROR, &frame->closure->function->chunk.tokenArr.tokens[(int)(frame->ip - frame->closure->function->chunk.code - 1)], msg);
}

//> NATIVE FUNCTIONS
bool nativeClock(Value *returnValue, Value *args)
{
    *returnValue = NUMBER((double)clock());

    return true;
}

bool nativePrint(Value *returnValue, Value *args)
{
    Value *arg = &args[1];

    printValue(*arg);
    putchar('\n');

    *returnValue = NIL;

    return true;
}

bool nativeInt(Value *returnValue, Value *args)
{
    Value *arg = &args[1];

    if (!IS_STRING(*arg))
    {
        runtimeError("The argument should be a string");
        return false;
    }

    ObjString *string = AS_STRING(*arg);

    *returnValue = NUMBER(strtod(string->chars, NULL));
    return true;
}

bool nativeString(Value *returnValue, Value *args)
{
    Value *arg = &args[1];

    if (!IS_NUMBER(*arg))
    {
        runtimeError("The argument should be a number");
        return false;
    }

    char buffer[16];
    sprintf(buffer, "%g", AS_NUMBER(*arg));

    *returnValue = OBJ((Obj *)allocateObjString(buffer, 16));

    return true;
}
//<

void push(Value value)
{
    *vm.stackTop = value;
    vm.stackTop++;
}

Value pop()
{
    vm.stackTop--;
    return *vm.stackTop;
}

static Value get(int distance)
{
    return vm.stackTop[-1 - distance];
}

static void defineNative(char *name, NativeFun fun, uint8_t argsCount)
{
    push(OBJ((Obj *)allocateObjString(name, strlen(name))));
    push(OBJ((Obj *)allocateObjNative(argsCount, fun)));
    hashMapInsert(&vm.globals, AS_STRING(get(1)), get(0));
    pop();
    pop();
}

void initVm()
{
    vm.frameCount = 0;
    vm.stackTop = vm.stack;
    vm.objects = NULL;
    vm.openUpValues = NULL;

    vm.gray = NULL;
    vm.grayCapacity = 0;
    vm.grayCount = 0;
    vm.allocatedBytes = 0;
    vm.nextVm = 1024 * 1024;

    initHashMap(&vm.globals);

    defineNative("clock", (NativeFun)nativeClock, 0);
    defineNative("print", (NativeFun)nativePrint, 1);
    defineNative("int", (NativeFun)nativeInt, 1);
    defineNative("string", (NativeFun)nativeString, 1);
}

static uint8_t next()
{
    return *vm.frames[vm.frameCount - 1].ip++;
}

static Value nextAsConstant()
{
    return vm.frames[vm.frameCount - 1].closure->function->chunk.constants.values[next()];
}

static ObjString *nextAsString()
{
    Value constant = nextAsConstant();

    return AS_STRING(constant);
}

static uint8_t peek()
{
    return *vm.frames[vm.frameCount - 1].ip;
}

static void closeUpValue(Value *slot)
{
    while (vm.openUpValues != NULL && vm.openUpValues->location >= slot)
    {
        ObjUpValue *upValue = vm.openUpValues;
        upValue->closed = *upValue->location;
        upValue->location = &upValue->closed;

        vm.openUpValues = upValue->next;
    }
}

bool call(Value value, int argsCount)
{
    switch (value.type)
    {
    case VAL_OBJ:
    {
        Obj *obj = AS_OBJ(value);

        switch (obj->type)
        {
        case OBJ_BOUND_METHOD:
        {
            ObjBoundMethod *boundMethod = (ObjBoundMethod *)obj;

            vm.stackTop[-argsCount - 1] = OBJ(boundMethod->instance);

            return call(OBJ(boundMethod->method), argsCount);
        }
        case OBJ_CLOSURE:
        {
            ObjClosure *closure = (ObjClosure *)obj;

            if (closure->function->arity != argsCount)
            {
                char msg[160];
                sprintf(msg, "Expected %d argument%s but got %d", closure->function->arity, closure->function->arity == 1 ? "" : "s", argsCount);

                runtimeError(msg);
                return false;
            }

            if (vm.frameCount == FRAMES_MAX)
            {
                runtimeError("Stack overflow");
                return false;
            }

            CallFrame *frame = &vm.frames[vm.frameCount++];

            frame->closure = closure;
            frame->ip = closure->function->chunk.code;
            frame->slots = vm.stackTop - frame->closure->function->arity - 1;

#ifdef DEBUG_BYTECODE
            ObjString *name = frame->closure->function->name;

            if (name != NULL)
            {
                printf("Excuting %s's chunk\n", name->chars);
            }
            else
            {
                printf("Executing anonymous function's chunk\n");
            }
#endif

            return true;
        }
        case OBJ_NATIVE:
        {
            ObjNative *function = (ObjNative *)obj;

            if (function->arity != argsCount)
            {
                char msg[160];
                sprintf(msg, "Expected %d argument%s but got %d", function->arity, function->arity == 1 ? "" : "s", argsCount);

                runtimeError(msg);
                return false;
            }

            Value returnValue;
            if (!function->function(&returnValue, vm.stackTop - function->arity - 1))
            {
                return false;
            }

            vm.stackTop -= function->arity + 1;

            push(returnValue);

            return true;
        }
        case OBJ_CLASS:
        {
            ObjClass *klass = (ObjClass *)obj;

            ObjInstance *instance = allocateObjInstance(klass);

            if (klass->initializer != NULL)
            {
                vm.stackTop[-argsCount - 1] = OBJ(instance);
                return call(OBJ(klass->initializer), argsCount);
            }
            else
            {
                if (argsCount != 0)
                {
                    char msg[160];

                    sprintf(msg, "Expected 0 arguments but got %d", argsCount);

                    runtimeError(msg);
                    return false;
                }

                push(OBJ(instance));
                return true;
            }
        }
        }
    }
    }

    runtimeError("Functions and classes are the only types that can be called");
    return false;
}

ObjString *concat(ObjString *s1, ObjString *s2)
{
    size_t length = s1->length + s2->length;

    char *temp = malloc(length);

    temp[0] = '\0';

    strcpy(temp, s1->chars);
    strcat(temp, s2->chars);

    ObjString *string = allocateObjString(temp, length);

    free(temp);

    return string;
}

Result run()
{
    CallFrame *frame = &vm.frames[vm.frameCount - 1];

#define NUMERIC_BINARY_OP(op)                              \
    {                                                      \
        Value b = pop();                                   \
        Value a = pop();                                   \
        if (IS_NUMBER(a) && IS_NUMBER(b))                  \
        {                                                  \
            push(NUMBER(AS_NUMBER(a) op AS_NUMBER(b)));    \
        }                                                  \
        else                                               \
        {                                                  \
            runtimeError("Both operands must be numbers"); \
            return RESULT_RUNTIME_ERROR;                   \
        }                                                  \
    }
#define CMP_BINARY_OP(op)                                  \
    {                                                      \
        Value b = pop();                                   \
        Value a = pop();                                   \
        if (IS_NUMBER(a) && IS_NUMBER(b))                  \
        {                                                  \
            push(BOOL(AS_NUMBER(a) op AS_NUMBER(b)));      \
        }                                                  \
        else                                               \
        {                                                  \
            runtimeError("Both operands must be numbers"); \
            return RESULT_RUNTIME_ERROR;                   \
        }                                                  \
    }

    while (true)
    {
#ifdef DEBUG_BYTECODE
        disassembleInstruction(&frame->closure->function->chunk, frame->ip - frame->closure->function->chunk.code);
#endif

        switch (next())
        {
        case OP_CONSTANT:
            push(nextAsConstant());
            break;
        case OP_NEGATE:
        {
            Value operand = pop();

            if (operand.type == VAL_NUMBER)
            {
                push(NUMBER(AS_NUMBER(operand) * -1));
            }
            else
            {
                runtimeError("Unary '-' operand must be a number");
                return RESULT_RUNTIME_ERROR;
            }

            break;
        }
        case OP_ADD:
        {
            Value b = pop();
            Value a = pop();

            if (IS_NUMBER(a) && IS_NUMBER(b))
            {
                push(NUMBER(AS_NUMBER(a) + AS_NUMBER(b)));
            }
            else if (IS_STRING(a) && IS_STRING(b))
            {
                push(OBJ(concat(AS_STRING(a), AS_STRING(b))));
            }
            else if (IS_STRING(a))
            { //>>IMPLEMENT
                runtimeError("Concatinating strings with other types isn't supported yet");
                return RESULT_RUNTIME_ERROR;
            }
            else if (IS_STRING(b))
            {
                runtimeError("Concatinating strings with other types isn't supported yet");
                return RESULT_RUNTIME_ERROR;
            }
            else
            { //<<
                runtimeError("Operands can be strings, a string mixed with another type, or numbers");
                return RESULT_RUNTIME_ERROR;
            }

            break;
        }

        case OP_SUBTRACT:
            NUMERIC_BINARY_OP(-)
            break;

        case OP_MULTIPLY:
            NUMERIC_BINARY_OP(*)
            break;

        case OP_DIVIDE:
            NUMERIC_BINARY_OP(/)
            break;

        case OP_EQUAL:
        {
            Value b = pop();
            Value a = pop();

            push(BOOL(equal(a, b)));
            break;
        }

        case OP_NOT_EQUAL:
        {
            Value b = pop();
            Value a = pop();

            push(BOOL(!equal(a, b)));
            break;
        }

        case OP_GREATER:
            CMP_BINARY_OP(>)
            break;

        case OP_GREATER_OR_EQUAL:
            CMP_BINARY_OP(>=)
            break;

        case OP_LESS:
            CMP_BINARY_OP(<)
            break;

        case OP_LESS_OR_EQUAL:
            CMP_BINARY_OP(<=)
            break;

        case OP_BANG:
            push(BOOL(!isTruthy(pop())));
            break;

        case OP_NIL:
            push(NIL);
            break;

        case OP_GET_GLOBAL:
        {
            ObjString *name = nextAsString();

            Value *value = hashMapGet(&vm.globals, name);

            if (value == NULL)
            {
                runtimeError("Undefined variable");
                return RESULT_RUNTIME_ERROR;
            }

            push(*value);
            break;
        }

        case OP_DEFINE_GLOBAL:
            hashMapInsert(&vm.globals, nextAsString(), pop());
            break;

        case OP_SET_GLOBAL:
            if (hashMapInsert(&vm.globals, nextAsString(), get(0)))
            {
                runtimeError("Undefined variable");
                return RESULT_RUNTIME_ERROR;
            }
            break;

        case OP_GET_LOCAL:
            push(frame->slots[next()]);
            break;

        case OP_SET_LOCAL:
            frame->slots[next()] = get(0);
            break;

        case OP_JUMP_IF_FALSE:
        {
            if (!isTruthy(get(0)))
                frame->ip += next() - 1;
            else
                next();

            break;
        }

        case OP_JUMP_IF_TRUE:
        {
            if (isTruthy(get(0)))
                frame->ip += next() - 1;
            else
                next();

            break;
        }

        case OP_JUMP:
            frame->ip += next() - 1;
            break;

        case OP_JUMP_BACKWARDS:
            frame->ip -= next() + 2; // +2 because frame->ip now equal OP_JUMP's one + 2 (because of calling next)
            break;

        case OP_POP:
            pop();
            break;

        case OP_RETURN:
        {
            vm.frameCount--;

            // stores its return value
            Value returnValue = pop();

            // pops its locals and put them if necessary on the heap
            closeUpValue(frame->slots);
            vm.stackTop = frame->slots;

            // pushes the return value
            push(returnValue);

            // updates the current frame
            frame = &vm.frames[vm.frameCount - 1];

            if (vm.frameCount == 0)
            {
                return RESULT_SUCCESS;
            }

#ifdef DEBUG_BYTECODE
            ObjString *name = frame->closure->function->name;

            if (name != NULL)
            {
                printf("Excuting %s's chunk\n", name->chars);
            }
            else
            {
                printf("Executing <script>'s chunk\n");
            }
#endif

            break;
        }

        case OP_CALL:
        {
            // gets the callee and calls it
            uint8_t argsCount = next();
            Value callee = get(argsCount);

            if (!call(callee, argsCount))
            {
                return RESULT_RUNTIME_ERROR;
            }

            frame = &vm.frames[vm.frameCount - 1];

            break;
        }

        // should push an ObjClosure to the stack
        // after filling its upvalues
        case OP_CLOSURE:
        {
            ObjFunction *function = AS_FUNCTION(nextAsConstant());
            uint8_t upValuesCount = next();

            ObjClosure *closure = allocateObjClosure(function, upValuesCount);
            push(OBJ((Obj *)closure));

            for (int i = 0; i < upValuesCount; i++)
            {
                bool local = next();
                uint8_t index = next();

                if (local)
                {
                    Value *slot = frame->slots + index;
                    ObjUpValue *prev = NULL;
                    ObjUpValue *upValue = vm.openUpValues;

                    while (upValue != NULL && upValue->location > slot)
                    {
                        prev = upValue;
                        upValue = upValue->next;
                    }

                    if (upValue != NULL && upValue->location == slot)
                    {
                        closure->upValues[i] = upValue;
                        continue;
                    }

                    ObjUpValue *createdUpValue = allocateObjUpValue(slot);

                    createdUpValue->next = upValue;

                    if (prev == NULL) // we exited straight away
                    {
                        vm.openUpValues = createdUpValue;
                    }
                    else
                    {
                        prev->next = createdUpValue;
                    }

                    closure->upValues[i] = createdUpValue;
                }
                else
                {
                    closure->upValues[i] = frame->closure->upValues[index];
                }
            }

            break;
        }

        case OP_GET_UPVALUE:
        {
            uint8_t index = next();

            push(*frame->closure->upValues[index]->location);
            break;
        }

        case OP_SET_UPVALUE:
        {
            uint8_t index = next();

            *frame->closure->upValues[index]->location = get(0);
            break;
        }

        case OP_CLOSE_UPVALUE:
            closeUpValue(vm.stackTop - 1);
            pop();
            break;

        case OP_CLASS:
        {
            ObjString *name = nextAsString();
            ObjClass *klass = allocateObjClass(name);

            hashMapInsert(&vm.globals, name, OBJ((Obj *)klass));
            push(OBJ(klass));
            break;
        }

        case OP_GET_PROPERTY:
        {
            Value obj = get(0);
            ObjString *key = nextAsString();
            Value value;

            switch (obj.type)
            {
            case VAL_OBJ:
            {
                switch (AS_OBJ(obj)->type)
                {
                case OBJ_INSTANCE:
                {
                    ObjInstance *instance = AS_INSTANCE(obj);
                    Value *ptr;

                    if ((ptr = hashMapGet(&instance->fields, key)) != NULL)
                    {
                        value = *ptr;
                        goto pushValue;
                    }

                    if ((ptr = hashMapGet(&instance->klass->methods, key)) != NULL)
                    {
                        ObjBoundMethod *boundMethod = allocateObjBoundMethod(instance, AS_CLOSURE(*ptr));
                        value = OBJ(boundMethod);
                        goto pushValue;
                    }

                    runtimeError("Undefined property");
                    return RESULT_RUNTIME_ERROR;
                }
                // TODO add String native class
                case OBJ_STRING:
                {
                    ObjString *string = AS_STRING(obj);
                    char length[] = "length";

                    if (key->length == strlen(length) && strcmp(key->chars, length) == 0)
                    {
                        value = NUMBER((double)string->length);
                        goto pushValue;
                    }
                    else
                    {
                        runtimeError("String class isn't yet implemented");
                        return RESULT_RUNTIME_ERROR;
                    }
                }
                case OBJ_CLASS:
                {
                    ObjClass *klass = AS_CLASS(obj);
                    Value *ptr;

                    if ((ptr = hashMapGet(&klass->fields, key)) == NULL)
                    {
                        if (hashMapGet(&klass->methods, key))
                        {
                            runtimeError("This field is a method and it can only be accessed from an instance");
                            return RESULT_RUNTIME_ERROR;
                        }

                        runtimeError("Undefined field");
                        return RESULT_RUNTIME_ERROR;
                    }

                    value = *ptr;
                    goto pushValue;
                }
                default:;
                }
            }
            default:
            {
                runtimeError("Getters can only be used with strings, instances, and classes");
                return RESULT_RUNTIME_ERROR;
            }
            }

        pushValue:
        {
            pop();
            push(value);
            break;
        }
        }

        case OP_SET_FIELD:
        {
            Value value = pop();
            Value obj = get(0);
            ObjString *key = nextAsString();

            switch (obj.type)
            {
            case VAL_OBJ:
            {
                switch (AS_OBJ(obj)->type)
                {
                case OBJ_INSTANCE:
                {
                    ObjInstance *instance = AS_INSTANCE(obj);

                    hashMapInsert(&instance->fields, key, value);
                    break;
                }
                default:
                    goto invalidObj;
                }
                break;
            }
            default:
                goto invalidObj;
            }
            break;

        invalidObj:
            runtimeError("Setters can only be used with instances and classes");
            return RESULT_RUNTIME_ERROR;
        }

        case OP_METHOD:
        {
            ObjClass *klass = AS_CLASS(get(1));
            ObjString *name = nextAsString();

            hashMapInsert(&klass->methods, name, pop());

            break;
        }

        case OP_INITIALIZER:
        {
            ObjClass *klass = AS_CLASS(get(1));
            klass->initializer = AS_CLOSURE(pop());
            break;
        }

        case OP_INVOKE:
        {
            // TODO make 'this' be of type 'Value'
            ObjString *key = nextAsString();
            uint8_t argsCount = next();
            ObjInstance *instance = AS_INSTANCE(get(argsCount));

            Value *value;

            if ((value = hashMapGet(&instance->fields, key)) != NULL)
            {
                if (!call(*value, argsCount))
                    return RESULT_RUNTIME_ERROR;
                else
                    goto updateFrame;
            }

            if ((value = hashMapGet(&instance->klass->methods, key)) != NULL)
            {
                if (!call(*value, argsCount))
                    return RESULT_RUNTIME_ERROR;
                else
                    goto updateFrame;
            }

            runtimeError("Undefined property");
            return RESULT_RUNTIME_ERROR;

        updateFrame:
            frame = &vm.frames[vm.frameCount - 1];
            break;
        }

        case OP_INHERIT:
        {
            ObjClass *klass = AS_CLASS(get(1));
            Value superclass = pop();

            if (!IS_CLASS(superclass))
            {
                runtimeError("Superclass must be a class");
                return RESULT_RUNTIME_ERROR;
            }

            hashMapInsertAll(&klass->methods, &AS_CLASS(superclass)->methods);
            klass->superclass = AS_CLASS(superclass);
            break;
        }

        default:;
        }
    }

#undef CMP_BINARY_OP
#undef NUMERIC_BINARY_OP
}

void freeVm()
{
}