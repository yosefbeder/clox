#include "vm.h"
#include "reporter.h"
#include "memory.h"
#include <string.h>
#include <time.h>

#ifdef DEBUG_BYTECODE
#include "debug.h"
#endif

static void runtimeError(Vm *vm, char msg[])
{
    CallFrame *frame = &vm->frames[vm->frameCount - 1];

    report(REPORT_RUNTIME_ERROR, &frame->closure->function->chunk.tokenArr.tokens[(int)(frame->ip - frame->closure->function->chunk.code - 1)], msg, vm);
}

//> NATIVE FUNCTIONS
bool nativeClock(Vm *vm, Value *returnValue, Value *args)
{
    *returnValue = NUMBER((double)clock() / CLOCKS_PER_SEC);

    return true;
}

bool nativePrint(Vm *vm, Value *returnValue, Value *args)
{
    Value *arg = &args[1];

    printValue(arg);
    putchar('\n');

    *returnValue = NIL;

    return true;
}

bool nativeInt(Vm *vm, Value *returnValue, Value *args)
{
    Value *arg = &args[1];

    if (!IS_STRING(arg))
    {
        runtimeError(vm, "The argument should be a string");
        return false;
    }

    ObjString *string = AS_STRING(arg);

    *returnValue = NUMBER(strtod(string->chars, NULL));
    return true;
}

bool nativeString(Vm *vm, Value *returnValue, Value *args)
{
    Value *arg = &args[1];

    if (!IS_NUMBER(arg))
    {
        runtimeError(vm, "The argument should be a number");
        return false;
    }

    char buffer[16];
    sprintf(buffer, "%.2lf", AS_NUMBER(arg));

    *returnValue = OBJ((Obj *)allocateObjString(vm, vm->compiler, buffer, 16));

    return true;
}
//<

static void push(Vm *vm, Value value)
{
    *vm->stackTop = value;
    vm->stackTop++;
}

static Value pop(Vm *vm)
{
    vm->stackTop--;
    return *vm->stackTop;
}

static void defineNative(Vm *vm, char *name, NativeFun fun, uint8_t argsCount)
{
    push(vm, OBJ((Obj *)allocateObjString(vm, vm->compiler, name, strlen(name))));
    push(vm, OBJ((Obj *)allocateObjNative(vm, vm->compiler, argsCount, fun)));
    hashMapInsert(&vm->globals, AS_STRING((vm->stackTop - 2)), vm->stackTop - 1);
    pop(vm);
    pop(vm);
}

void initVm(Vm *vm, struct Compiler *compiler)
{
    vm->frameCount = 0;
    vm->stackTop = vm->stack;
    vm->objects = NULL;
    vm->openUpValues = NULL;
    vm->compiler = compiler;

    vm->gray = NULL;
    vm->grayCapacity = 0;
    vm->grayCount = 0;

    initHashMap(&vm->globals);

    defineNative(vm, "clock", nativeClock, 0);
    defineNative(vm, "print", nativePrint, 1);
    defineNative(vm, "int", nativeInt, 1);
    defineNative(vm, "string", nativeString, 1);
}

static uint8_t next(Vm *vm)
{
    return *vm->frames[vm->frameCount - 1].ip++;
}

static Value nextAsConstant(Vm *vm)
{
    return vm->frames[vm->frameCount - 1].closure->function->chunk.constants.values[next(vm)];
}

static ObjString *nextAsString(Vm *vm)
{
    Value constant = nextAsConstant(vm);

    return AS_STRING((&constant));
}

static uint8_t *peek(Vm *vm)
{
    return vm->frames[vm->frameCount - 1].ip;
}

static void closeUpValue(Vm *vm, Value *slot)
{
    while (vm->openUpValues != NULL && vm->openUpValues->location >= slot)
    {
        ObjUpValue *upValue = vm->openUpValues;
        upValue->closed = *upValue->location;
        upValue->location = &upValue->closed;

        vm->openUpValues = upValue->next;
    }
}

bool call(Vm *vm, Value *value, int argsCount)
{
    if (!IS_NATIVE(value) && !IS_CLOSURE(value))
    {
        runtimeError(vm, "Functions and classes are the only types that can be called");
        return false;
    }

    Obj *obj = AS_OBJ(value);

    switch (obj->type)
    {
    case OBJ_CLOSURE:
    {
        ObjClosure *closure = (ObjClosure *)obj;

        if (closure->function->arity != argsCount)
        {
            char msg[160];
            sprintf(msg, "Expected %d argument%s but got %d", closure->function->arity, closure->function->arity == 1 ? "" : "s", argsCount);

            runtimeError(vm, msg);
            return false;
        }

        if (vm->frameCount == FRAMES_MAX)
        {
            runtimeError(vm, "Stack overflow");
            return false;
        }

        if (vm->frameCount == 0)
        {
            push(vm, OBJ(obj));
        }

        CallFrame *frame = &vm->frames[vm->frameCount++];

        frame->closure = closure;
        frame->ip = closure->function->chunk.code;
        frame->slots = vm->stackTop - frame->closure->function->arity - 1;

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

        return true;
    }
    case OBJ_NATIVE:
    {
        ObjNative *function = (ObjNative *)obj;

        if (function->arity != argsCount)
        {
            char msg[160];
            sprintf(msg, "Expected %d argument%s but got %d", function->arity, function->arity == 1 ? "" : "s", argsCount);

            runtimeError(vm, msg);
            return false;
        }

        Value returnValue;
        if (!function->function(vm, &returnValue, vm->stackTop - function->arity - 1))
        {
            return false;
        }

        vm->stackTop -= function->arity + 1;

        push(vm, returnValue);

        return true;
    }
    }
}

Result run(Vm *vm)
{
    CallFrame *frame = &vm->frames[vm->frameCount - 1];

#define NUMERIC_BINARY_OP(op)                                  \
    {                                                          \
        Value b = pop(vm);                                     \
        Value a = pop(vm);                                     \
        if (IS_NUMBER(&a) && IS_NUMBER(&b))                    \
        {                                                      \
            push(vm, NUMBER(AS_NUMBER(&a) op AS_NUMBER(&b)));  \
        }                                                      \
        else                                                   \
        {                                                      \
            runtimeError(vm, "Both operands must be numbers"); \
            return RESULT_RUNTIME_ERROR;                       \
        }                                                      \
    }
#define CMP_BINARY_OP(op)                                      \
    {                                                          \
        Value b = pop(vm);                                     \
        Value a = pop(vm);                                     \
        if (IS_NUMBER(&a) && IS_NUMBER(&b))                    \
        {                                                      \
            push(vm, BOOL(AS_NUMBER(&a) op AS_NUMBER(&b)));    \
        }                                                      \
        else                                                   \
        {                                                      \
            runtimeError(vm, "Both operands must be numbers"); \
            return RESULT_RUNTIME_ERROR;                       \
        }                                                      \
    }

    while (true)
    {
#ifdef DEBUG_BYTECODE
        disassembleInstruction(&frame->closure->function->chunk, frame->ip - frame->closure->function->chunk.code);
#endif

        switch (next(vm))
        {
        case OP_CONSTANT:
            push(vm, nextAsConstant(vm));
            break;
        case OP_NEGATE:
        {
            Value operand = pop(vm);

            if (operand.type == VAL_NUMBER)
            {
                push(vm, NUMBER(AS_NUMBER(&operand) * -1));
            }
            else
            {
                runtimeError(vm, "Unary '-' operand must be a number");
                return RESULT_RUNTIME_ERROR;
            }

            break;
        }
        case OP_ADD:
        {
            Value b = pop(vm);
            Value a = pop(vm);

            if (IS_NUMBER(&a) && IS_NUMBER(&b))
            {
                push(vm, NUMBER(AS_NUMBER(&a) + AS_NUMBER(&b)));
            }
            else if (IS_STRING(&a) && IS_STRING(&b))
            {
                int length = AS_STRING(&a)->length + AS_STRING(&b)->length;
                char *result = malloc(length + 1);

                result[0] = '\0';

                strcat(result, AS_STRING(&a)->chars);
                strcat(result, AS_STRING(&b)->chars);

                push(vm, OBJ((Obj *)allocateObjString(vm, vm->compiler, result, length)));
                free(result);
            }
            else if (IS_STRING(&a))
            { //>>IMPLEMENT
                runtimeError(vm, "Concatinating strings with other types isn't supported yet");
                return RESULT_RUNTIME_ERROR;
            }
            else if (IS_STRING(&b))
            {
                runtimeError(vm, "Concatinating strings with other types isn't supported yet");
                return RESULT_RUNTIME_ERROR;
            }
            else
            { //<<
                runtimeError(vm, "Operands can be strings, a string mixed with another type, or numbers");
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
            Value b = pop(vm);
            Value a = pop(vm);

            push(vm, BOOL(equal(&a, &b)));
            break;
        }

        case OP_NOT_EQUAL:
        {
            Value b = pop(vm);
            Value a = pop(vm);

            push(vm, BOOL(!equal(&a, &b)));
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
        {
            Value operand = pop(vm);
            uint8_t value = isTruthy(&operand);

            push(vm, BOOL(!value));
            break;
        }

        case OP_NIL:
            push(vm, NIL);
            break;

        case OP_GET_GLOBAL:
        {
            ObjString *name = nextAsString(vm);

            Value *value = hashMapGet(&vm->globals, name);

            if (value == NULL)
            {
                runtimeError(vm, "Undefined variable");
                return RESULT_RUNTIME_ERROR;
            }

            push(vm, *value);
            break;
        }

        case OP_DEFINE_GLOBAL:
        {
            ObjString *name = nextAsString(vm);
            Value value = pop(vm);

            hashMapInsert(&vm->globals, name, &value);
            break;
        }

        case OP_ASSIGN_GLOBAL:
        {
            ObjString *name = nextAsString(vm);

            if (hashMapInsert(&vm->globals, name, vm->stackTop - 1))
            {
                runtimeError(vm, "Undefined variable");
                return RESULT_RUNTIME_ERROR;
            }

            break;
        }

        case OP_GET_LOCAL:
        {
            push(vm, frame->slots[next(vm)]);
            break;
        }

        case OP_ASSIGN_LOCAL:
            frame->slots[next(vm)] = *(vm->stackTop - 1);
            break;

        case OP_JUMP_IF_FALSE:
        {
            if (!isTruthy((vm->stackTop - 1)))
                frame->ip += next(vm) - 1;
            else
                next(vm);

            break;
        }

        case OP_JUMP_IF_TRUE:
        {
            if (isTruthy((vm->stackTop - 1)))
                frame->ip += next(vm) - 1;
            else
                next(vm);

            break;
        }

        case OP_JUMP:
            frame->ip += next(vm) - 1;
            break;

        case OP_JUMP_BACKWARDS:
            frame->ip -= next(vm) + 2; // +2 because frame->ip now equal OP_JUMP's one + 2 (because of calling next)
            break;

        case OP_POP:
        {
            pop(vm);
            break;
        }

        case OP_RETURN:
        {
            vm->frameCount--;

            // stores its return value
            Value returnValue = pop(vm);

            // pops its locals and put them if necessary on the heap
            closeUpValue(vm, frame->slots);
            vm->stackTop = frame->slots;

            // pushes the return value
            push(vm, returnValue);

            // updates the current frame
            frame = &vm->frames[vm->frameCount - 1];

            if (vm->frameCount == 0)
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
            uint8_t argsCount = next(vm);
            Value *callee = vm->stackTop - argsCount - 1;

            if (!call(vm, callee, argsCount))
            {
                return RESULT_RUNTIME_ERROR;
            }

            frame = &vm->frames[vm->frameCount - 1];

            break;
        }

        // should push an ObjClosure to the stack
        // after filling its upvalues
        case OP_CLOSURE:
        {
            Value constant = nextAsConstant(vm);
            ObjFunction *function = AS_FUNCTION(&constant);
            uint8_t upValuesCount = next(vm);

            ObjClosure *closure = allocateObjClosure(vm, vm->compiler, function, upValuesCount);
            push(vm, OBJ((Obj *)closure));

            for (int i = 0; i < upValuesCount; i++)
            {
                bool local = next(vm);
                uint8_t index = next(vm);

                if (local)
                {
                    Value *slot = frame->slots + index;
                    ObjUpValue *prev = NULL;
                    ObjUpValue *upValue = vm->openUpValues;

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

                    ObjUpValue *createdUpValue = allocateObjUpValue(vm, vm->compiler, slot);

                    createdUpValue->next = upValue;

                    if (prev == NULL) // we exited straight away
                    {
                        vm->openUpValues = createdUpValue;
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
            uint8_t index = next(vm);

            push(vm, *frame->closure->upValues[index]->location);
            break;
        }

        case OP_ASSIGN_UPVALUE:
        {
            uint8_t index = next(vm);

            *frame->closure->upValues[index]->location = *(vm->stackTop - 1);
            break;
        }

        case OP_CLOSE_UPVALUE:
        {
            closeUpValue(vm, vm->stackTop - 1);
            pop(vm);
            break;
        }

        default:;
        }
    }

#undef CMP_BINARY_OP
#undef NUMERIC_BINARY_OP
}

void freeVm(Vm *vm)
{
}