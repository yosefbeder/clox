#include "vm.h"
#include "reporter.h"
#include <string.h>

void initVm(Vm* vm) {
    vm->frameCount = 0;
    vm->stackTop = vm->stack;
    vm->objects = NULL;

    HashMap globals;
    initHashMap(&globals);

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

static void printValue(Value* value) {
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
}

Result run(Vm* vm) {
    CallFrame* frame = &vm->frames[vm->frameCount - 1];

    #define NEXT_BYTE *(++frame->ip)
    #define NEXT_CONSTANT frame->function->chunk.constants.values[NEXT_BYTE]
    #define NEXT_STRING AS_STRING((&NEXT_CONSTANT))
    #define NUMERIC_BINARY_OP(op)\
        {\
            Value b = pop(vm);\
            Value a = pop(vm);\
            if (IS_NUMBER((&a)) && IS_NUMBER((&b))) {\
                push(vm, NUMBER(AS_NUMBER((&a)) op AS_NUMBER((&b))));\
            } else {\
                report(REPORT_RUNTIME_ERROR, &frame->function->chunk.tokenArr.tokens[(int) (frame->ip - frame->function->chunk.code - (a.type != VAL_NUMBER? 4: 2))], "Both operands must be numbers");\
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
                report(REPORT_RUNTIME_ERROR, &frame->function->chunk.tokenArr.tokens[(int) (frame->ip - frame->function->chunk.code - (a.type != VAL_NUMBER? 4: 2))], "Both operands must be numbers");\
                return RESULT_RUNTIME_ERROR;\
            }\
        }
    
    while (true) {
        if (*frame->ip == OP_CONSTANT) 
            push(vm, NEXT_CONSTANT);
        
        else if (*frame->ip == OP_NEGATE) {
            Value operand = pop(vm);

            if (operand.type == VAL_NUMBER) {
                push(vm, NUMBER(AS_NUMBER((&operand)) * -1));
            } else {
                report(REPORT_RUNTIME_ERROR, &frame->function->chunk.tokenArr.tokens[(int) (frame->ip - frame->function->chunk.code - 2)], "Unary '-' operand must be a number");
                return RESULT_RUNTIME_ERROR;
            }
        }

        else if (*frame->ip == OP_ADD) {
            Value b = pop(vm);
            Value a = pop(vm);

            if (IS_NUMBER((&a)) && IS_NUMBER((&b))) {
                push(vm, NUMBER(AS_NUMBER((&a)) + AS_NUMBER((&b))));
            } else if (IS_STRING((&a)) && IS_STRING((&b))) {
                int length = AS_STRING((&a))->length + AS_STRING((&b))->length;
                char* result = malloc(length + 1);

                result[0] = '\0';

                strcat(result, AS_STRING((&a))->chars);
                strcat(result, AS_STRING((&b))->chars);

                push(vm, STRING(allocateObjString(vm, result)));
            } else if (IS_STRING((&a))) { //>>IMPLEMENT
                report(REPORT_RUNTIME_ERROR, &frame->function->chunk.tokenArr.tokens[(int) (frame->ip - frame->function->chunk.code - 2)], "Concatinating strings with other types isn't supported yet");
                return RESULT_RUNTIME_ERROR;
            } else if (IS_STRING((&b))) {
                report(REPORT_RUNTIME_ERROR, &frame->function->chunk.tokenArr.tokens[(int) (frame->ip - frame->function->chunk.code - 4)], "Concatinating strings with other types isn't supported yet");
                return RESULT_RUNTIME_ERROR;
            } else { //<<
               report(REPORT_RUNTIME_ERROR, &frame->function->chunk.tokenArr.tokens[(int) (frame->ip - frame->function->chunk.code)], "Operands can be strings, a string mixed with another type, or numbers");
               return RESULT_RUNTIME_ERROR;
            }
        }

        else if (*frame->ip == OP_SUBTRACT) NUMERIC_BINARY_OP(-)

        else if (*frame->ip == OP_MULTIPLY) NUMERIC_BINARY_OP(*)

        else if (*frame->ip == OP_DIVIDE) NUMERIC_BINARY_OP(/)

        else if (*frame->ip == OP_EQUAL) {
            Value b = pop(vm);
            Value a = pop(vm);


            push(vm, BOOL(equal(&a, &b)));
        }

        else if (*frame->ip == OP_NOT_EQUAL) {
            Value b = pop(vm);
            Value a = pop(vm);

            push(vm, BOOL(!equal(&a, &b)));
        }

        else if (*frame->ip == OP_GREATER) CMP_BINARY_OP(>)

        else if (*frame->ip == OP_GREATER_OR_EQUAL) CMP_BINARY_OP(>=)

        else if (*frame->ip == OP_LESS) CMP_BINARY_OP(<)

        else if (*frame->ip == OP_LESS_OR_EQUAL) CMP_BINARY_OP(<=)

        else if (*frame->ip == OP_BANG) {
            Value operand = pop(vm);
            uint8_t value = isTruthy(&operand);


            push(vm, BOOL(!value));
        }

        else if (*frame->ip == OP_NIL) {
            push(vm, NIL);
        }

        else if (*frame->ip == OP_GET_GLOBAL) {
            ObjString* name = NEXT_STRING;

            Value* value = hashMapGet(&vm->globals, name);

            if (value == NULL) {
                report(REPORT_RUNTIME_ERROR, &frame->function->chunk.tokenArr.tokens[(int) (frame->ip - frame->function->chunk.code)], "Undefined variable");
                return RESULT_RUNTIME_ERROR;
            }

            push(vm, *value);
        }

        else if (*frame->ip == OP_DEFINE_GLOBAL) {
            ObjString* name = NEXT_STRING;
            Value value = pop(vm);

            hashMapInsert(&vm->globals, name, &value);
        }

        else if (*frame->ip == OP_ASSIGN_GLOBAL) {
            ObjString* name = NEXT_STRING;

            if (hashMapInsert(&vm->globals, name, vm->stackTop - 1)) {
                report(REPORT_RUNTIME_ERROR, &frame->function->chunk.tokenArr.tokens[(int) (frame->ip - frame->function->chunk.code)], "Undefined variable");
                return RESULT_RUNTIME_ERROR;
            }
        }

        else if (*frame->ip == OP_GET_LOCAL) {
            push(vm, frame->slots[NEXT_BYTE]);
        }

        else if (*frame->ip == OP_ASSIGN_LOCAL) {
            frame->slots[NEXT_BYTE] = *(vm->stackTop - 1);
        }

        else if (*frame->ip == OP_JUMP_IF_FALSE) {
            if (isTruthy((vm->stackTop - 1))) {
                NEXT_BYTE;
            } else {
                frame->ip += NEXT_BYTE;
                continue;
            }
        }

        else if (*frame->ip == OP_JUMP_IF_TRUE) {
            if (isTruthy((vm->stackTop - 1))) {
                frame->ip += NEXT_BYTE;
                continue;
            } else {
                NEXT_BYTE;
            }
        }

        else if (*frame->ip == OP_JUMP) {
            frame->ip += NEXT_BYTE;
            continue;
        }

        else if (*frame->ip == OP_JUMP_BACKWARDS) {
            frame->ip -= NEXT_BYTE;
            continue;
        }

        else if (*frame->ip == OP_POP) {
            // It should just pop the value 🙄
            Value poped = pop(vm);

            printValue(&poped);

            putchar('\n');
        }

        else if (*frame->ip == OP_RETURN) {
            vm->frameCount--;
            break;
        }

        frame->ip++;
    }

    #undef CMP_BINARY_OP
    #undef NUMERIC_BINARY_OP
    #undef NEXT_STRING
    #undef NEXT_CONSTANT
    #undef NEXT_BYTE

    return RESULT_SUCCESS;
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