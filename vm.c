#include "vm.h"
#include "reporter.h"
#include <string.h>
#include <time.h>

static void runtimeError(Vm* vm, char msg[]) {
    CallFrame* frame = &vm->frames[vm->frameCount - 1];

    report(REPORT_RUNTIME_ERROR, &frame->function->chunk.tokenArr.tokens[(int)(frame->ip - frame->function->chunk.code - 1)], msg, vm);
}

void printValue(Value* value) {
    if (IS_STRING(value)) {
        printf("%s", AS_STRING(value)->chars);
    } else if (IS_BOOL(value)) {
        printf("%s", AS_BOOL(value)? "true": "false");
    } else if (IS_NIL(value)) {
        printf("%s", "nil");
    } else if (IS_NUMBER(value)) {
        printf("%.2lf", AS_NUMBER(value));
    } else if (IS_FUNCTION(value)) {
        printf("<fn %s>", AS_FUNCTION(value)->name->chars);
    }
}

static bool equal(Value* a, Value* b) {
    if (a->type != b->type) return false;

    if (IS_STRING(a)) {
        return strcmp(AS_STRING(a)->chars, AS_STRING(b)->chars) == 0? true: false;
    }

    switch (a->type) {
        case VAL_BOOL:
            return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NIL:
            return true;
        case VAL_NUMBER:
            return AS_NUMBER(a) == AS_NUMBER(b);
    }

    return false;
}

//> NATIVE FUNCTIONS

bool nativeClock(Vm* vm, Value* returnValue, Value* args) {
    *returnValue = NUMBER((double)clock() / CLOCKS_PER_SEC);

    return true;
}

bool nativePrint(Vm* vm, Value* returnValue, Value* args) {
    Value* arg = &args[1];

    printValue(arg);
    putchar('\n');

    *returnValue = NIL;

    return true;
}

bool nativeInt(Vm* vm, Value* returnValue, Value* args) {
    Value* arg = &args[1];

    if (!IS_STRING(arg)) {
        runtimeError(vm, "The argument should be a string");
        return false;
    }

    ObjString* string = AS_STRING(arg);

    *returnValue = NUMBER(strtod(string->chars, NULL));
    return true;
}

bool nativeString(Vm* vm, Value* returnValue, Value* args) {
    Value *arg = &args[1];

    if (!IS_NUMBER(arg)) {
        runtimeError(vm, "The argument should be a number");
        return false;
    }

    char buffer[16];
    sprintf(buffer, "%.2lf", AS_NUMBER(arg));

    *returnValue = STRING(allocateObjString(vm, buffer, 16));

    return true;
}

//<

void initVm(Vm* vm) {
    vm->frameCount = 0;
    vm->stackTop = vm->stack;
    vm->objects = NULL;

    HashMap globals;
    initHashMap(&globals);

    hashMapInsert(&globals, allocateObjString(vm, "clock", 5), &NATIVE(allocateObjNative(vm, 0, nativeClock)));
    hashMapInsert(&globals, allocateObjString(vm, "print", 5), &NATIVE(allocateObjNative(vm, 1, nativePrint)));
    hashMapInsert(&globals, allocateObjString(vm, "int", 3), &NATIVE(allocateObjNative(vm, 1, nativeInt)));
    hashMapInsert(&globals, allocateObjString(vm, "string", 6), &NATIVE(allocateObjNative(vm, 1, nativeString)));

    vm->globals = globals;
}

static void push(Vm* vm, Value value) {
    *vm->stackTop = value;
    vm->stackTop++;
}

static Value pop(Vm* vm) {
    vm->stackTop--;
    return *vm->stackTop;
}

static uint8_t next(Vm* vm) {
    return *vm->frames[vm->frameCount - 1].ip++;
}

static Value nextAsConstant(Vm* vm) {
    return vm->frames[vm->frameCount - 1].function->chunk.constants.values[next(vm)];
}

static ObjString* nextAsString(Vm* vm) {
    Value constant = nextAsConstant(vm);

    return AS_STRING((&constant));
}

static uint8_t* peek(Vm* vm) {
    return vm->frames[vm->frameCount - 1].ip;
}

bool call(Vm* vm, Obj* obj, int argsCount) {
    switch (obj->type) {
        case OBJ_FUNCTION: {
            ObjFunction* function = (ObjFunction*) obj;

            if (function->arity != argsCount) {
                char msg[160];
                sprintf(msg, "Expected %d argument%s but got %d", function->arity, function->arity == 1 ? "" : "s", argsCount);

                runtimeError(vm, msg);
                return false;
            }

            if (vm->frameCount == FRAMES_MAX) {
                runtimeError(vm, "Stack overflow");
                return false;
            }

            if (vm->frameCount == 0) {
                push(vm, FUNCTION(obj));
            }

            CallFrame* frame = &vm->frames[vm->frameCount++];

            frame->function = function;
            frame->ip = function->chunk.code;
            frame->slots = vm->stackTop - frame->function->arity - 1;

            return true;
        }
        case OBJ_NATIVE: {
            ObjNative* function = (ObjNative*) obj;

            if (function->arity != argsCount) {
                char msg[160];
                sprintf(msg, "Expected %d argument%s but got %d", function->arity, function->arity == 1 ? "" : "s", argsCount);

                runtimeError(vm, msg);
                return false;
            }

            Value returnValue;
            if (!function->function(vm, &returnValue, vm->stackTop - function->arity - 1)) {
                return false;
            }

            vm->stackTop -= function->arity + 1;

            push(vm, returnValue);

            return true;
        }
    }
}

Result run(Vm* vm) {
    CallFrame* frame = &vm->frames[vm->frameCount - 1];

    #define NUMERIC_BINARY_OP(op)\
        {\
            Value b = pop(vm);\
            Value a = pop(vm);\
            if (IS_NUMBER((&a)) && IS_NUMBER((&b))) {\
                push(vm, NUMBER(AS_NUMBER((&a)) op AS_NUMBER((&b))));\
            } else {\
                runtimeError(vm, "Both operands must be numbers");\
                return RESULT_RUNTIME_ERROR;\
            }\
        }
    #define CMP_BINARY_OP(op)\
        {\
            Value b = pop(vm);\
            Value a = pop(vm);\
            if (IS_NUMBER((&a)) && IS_NUMBER((&b))) {\
                push(vm, BOOL(AS_NUMBER((&a)) op AS_NUMBER((&b))));\
            } else {\
                runtimeError(vm, "Both operands must be numbers");\
                return RESULT_RUNTIME_ERROR;\
            }\
        }
    
    while (true) {
        switch (next(vm)) {
            case OP_CONSTANT:
                push(vm, nextAsConstant(vm));
                break;
            case OP_NEGATE: {
                Value operand = pop(vm);

                if (operand.type == VAL_NUMBER) {
                    push(vm, NUMBER(AS_NUMBER((&operand)) * -1));
                }
                else {
                    runtimeError(vm, "Unary '-' operand must be a number");
                    return RESULT_RUNTIME_ERROR;
                }

                break;
            }
            case OP_ADD: {
                Value b = pop(vm);
                Value a = pop(vm);

                if (IS_NUMBER((&a)) && IS_NUMBER((&b))) {
                    push(vm, NUMBER(AS_NUMBER((&a)) + AS_NUMBER((&b))));
                }
                else if (IS_STRING((&a)) && IS_STRING((&b))) {
                    int length = AS_STRING((&a))->length + AS_STRING((&b))->length;
                    char* result = malloc(length + 1);

                    result[0] = '\0';

                    strcat(result, AS_STRING((&a))->chars);
                    strcat(result, AS_STRING((&b))->chars);

                    push(vm, STRING(allocateObjString(vm, result, length)));
                    free(result);
                } else if (IS_STRING((&a)))
                { //>>IMPLEMENT
                    runtimeError(vm, "Concatinating strings with other types isn't supported yet");
                    return RESULT_RUNTIME_ERROR;
                } else if (IS_STRING((&b))) {
                    runtimeError(vm, "Concatinating strings with other types isn't supported yet");
                    return RESULT_RUNTIME_ERROR;
                } else { //<<
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

            case OP_EQUAL: {
                Value b = pop(vm);
                Value a = pop(vm);

                push(vm, BOOL(equal(&a, &b)));
                break;
            }

            case OP_NOT_EQUAL: {
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

            case OP_BANG: {
                Value operand = pop(vm);
                uint8_t value = isTruthy(&operand);

                push(vm, BOOL(!value));
                break;
            }

            case OP_NIL:
                push(vm, NIL);
                break;

            case OP_GET_GLOBAL: {
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

            case OP_DEFINE_GLOBAL: {
                ObjString *name = nextAsString(vm);
                Value value = pop(vm);

                hashMapInsert(&vm->globals, name, &value);
                break;
            }

            case OP_ASSIGN_GLOBAL: {
                ObjString *name = nextAsString(vm);

                if (hashMapInsert(&vm->globals, name, vm->stackTop - 1)) {
                    runtimeError(vm, "Undefined variable");
                    return RESULT_RUNTIME_ERROR;
                }

                break;
            }

            case OP_DEFINE_LOCAL:
                break;

            case OP_GET_LOCAL: {
                push(vm, frame->slots[next(vm)]);
                break;
            }

            case OP_ASSIGN_LOCAL:
                frame->slots[next(vm)] = *(vm->stackTop - 1);
                break;

            case OP_JUMP_IF_FALSE: {
                if (!isTruthy((vm->stackTop - 1)))
                    frame->ip += next(vm) - 1;
                else
                    next(vm);

                break;
            }

            case OP_JUMP_IF_TRUE: {
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
                frame->ip -= next(vm) + 1;
                break;

            case OP_POP: {
                pop(vm);
                break;
            }

            case OP_RETURN: {
                vm->frameCount--;

                // stores its return value
                Value returnValue = pop(vm);

                // pops its locals including the function itself from the stack
                vm->stackTop = frame->slots;

                // pushes the return value
                push(vm, returnValue);

                // updates the current frame
                frame = &vm->frames[vm->frameCount - 1];

                if (vm->frameCount == 0) {
                    return RESULT_SUCCESS;
                }

                break;
            }

            case OP_CALL: {
                // gets the callee and calls it
                uint8_t argsCount = next(vm);
                ObjFunction* callee = AS_FUNCTION((vm->stackTop - argsCount - 1));

                if (!call(vm, (Obj*) callee, argsCount)) {
                    return RESULT_RUNTIME_ERROR;
                }

                // update the current frame
                frame = &vm->frames[vm->frameCount - 1];
                break;
            }

            default: ;
        }
    }

    #undef CMP_BINARY_OP
    #undef NUMERIC_BINARY_OP

}

void freeVm(Vm* vm) {
    Obj* curObj = vm->objects;

    while (curObj) {
        freeObj(curObj);

        Obj* temp = curObj;
        curObj = (Obj*) curObj->next;

        free(temp);
    }
}