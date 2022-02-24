#include "vm.h"
#include "error.h"
#include <string.h>

void initVm(Vm* vm) {
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
        puts(AS_STRING(value)->chars);
    } else if (IS_BOOL(value)) {
        puts(AS_BOOL(value)? "true": "false");
    } else if (IS_NIL(value)) {
        puts("nil");
    } else if (IS_NUMBER(value)) {
        printf("%.2lf", AS_NUMBER(value));
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

Result runChunk(Vm* vm, Chunk* chunk) {
    #define NEXT_BYTE *(++ip)
    #define NEXT_CONSTANT chunk->constants.values[NEXT_BYTE]
    #define NEXT_STRING AS_STRING((&NEXT_CONSTANT))
    #define NUMERIC_BINARY_OP(op)\
        {\
            Value b = pop(vm);\
            Value a = pop(vm);\
            if (IS_NUMBER((&a)) && IS_NUMBER((&b))) {\
                push(vm, NUMBER(AS_NUMBER((&a)) op AS_NUMBER((&b))));\
            } else {\
                reportError(ERROR_RUNTIME, &chunk->tokenArr.tokens[(int) (ip - chunk->code - (a.type != VAL_NUMBER? 4: 2))], "Both operands must be numbers");\
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
                reportError(ERROR_RUNTIME, &chunk->tokenArr.tokens[(int) (ip - chunk->code - (a.type != VAL_NUMBER? 4: 2))], "Both operands must be numbers");\
                return RESULT_RUNTIME_ERROR;\
            }\
        }

    uint8_t* ip = chunk->code;
    
    while (true) {
        if (*ip == OP_CONSTANT) 
            push(vm, NEXT_CONSTANT);
        
        else if (*ip == OP_NEGATE) {
            Value operand = *(vm->stackTop - 1);

            if (operand.type == VAL_NUMBER) {
                operand.as.number *= -1;
            } else {
                reportError(ERROR_RUNTIME, &chunk->tokenArr.tokens[(int) (ip - chunk->code - 2)], "Unary '-' operand must be a number");
                return RESULT_RUNTIME_ERROR;
            }
        }

        else if (*ip == OP_ADD) {
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
                reportError(ERROR_RUNTIME, &chunk->tokenArr.tokens[(int) (ip - chunk->code - 2)], "Concatinating strings with other types isn't supported yet");
                return RESULT_RUNTIME_ERROR;
            } else if (IS_STRING((&b))) {
                reportError(ERROR_RUNTIME, &chunk->tokenArr.tokens[(int) (ip - chunk->code - 4)], "Concatinating strings with other types isn't supported yet");
                return RESULT_RUNTIME_ERROR;
            } else { //<<
               reportError(ERROR_RUNTIME, &chunk->tokenArr.tokens[(int) (ip - chunk->code)], "Operands can be strings, a string mixed with another type, or numbers");
               return RESULT_RUNTIME_ERROR;
            }
        }

        else if (*ip == OP_SUBTRACT) NUMERIC_BINARY_OP(-)

        else if (*ip == OP_MULTIPLY) NUMERIC_BINARY_OP(*)

        else if (*ip == OP_DIVIDE) NUMERIC_BINARY_OP(/)

        else if (*ip == OP_OR) {
            Value b = pop(vm);
            Value a = pop(vm);

            if (isTruthy(&a)) {
                push(vm, a);
            }
            else {
                push(vm, b);
            }
        }

        else if (*ip == OP_AND) {
            Value b = pop(vm);
            Value a = pop(vm);

            if (!isTruthy(&a)) {
                push(vm, a);
            }
            else {
                push(vm, b);
            }
        }

        else if (*ip == OP_EQUAL) {
            Value b = pop(vm);
            Value a = pop(vm);


            push(vm, BOOL(equal(&a, &b)));
        }

        else if (*ip == OP_NOT_EQUAL) {
            Value b = pop(vm);
            Value a = pop(vm);

            push(vm, BOOL(!equal(&a, &b)));
        }

        else if (*ip == OP_GREATER) CMP_BINARY_OP(>)

        else if (*ip == OP_GREATER_OR_EQUAL) CMP_BINARY_OP(>=)

        else if (*ip == OP_LESS) CMP_BINARY_OP(<)

        else if (*ip == OP_LESS_OR_EQUAL) CMP_BINARY_OP(<=)

        else if (*ip == OP_BANG) {
            Value operand = pop(vm);
            uint8_t value = isTruthy(&operand);


            push(vm, BOOL(!value));
        }

        else if (*ip == OP_NIL) {
            push(vm, NIL);
        }

        else if (*ip == OP_GET_GLOBAL) {
            ObjString* name = NEXT_STRING;

            Value* value = hashMapGet(&vm->globals, name);

            if (value == NULL) {
                reportError(ERROR_RUNTIME, &chunk->tokenArr.tokens[(int) (ip - chunk->code)], "Undefined variable");
                return RESULT_RUNTIME_ERROR;
            }

            push(vm, *value);
        }

        else if (*ip == OP_DEFINE_GLOBAL) {
            ObjString* name = NEXT_STRING;
            Value value = pop(vm);

            hashMapInsert(&vm->globals, name, &value);
        }

        else if (*ip == OP_ASSIGN_GLOBAL) {
            ObjString* name = NEXT_STRING;

            if (hashMapInsert(&vm->globals, name, vm->stackTop - 1)) {
                reportError(ERROR_RUNTIME, &chunk->tokenArr.tokens[(int) (ip - chunk->code)], "Undefined variable");
                return RESULT_RUNTIME_ERROR;
            }
        }

        else if (*ip == OP_POP) {
            // It should just pop the value 🙄
            Value poped = pop(vm);

            printValue(&poped);

            putchar('\n');
        }

        else if (*ip == OP_RETURN) {
            break;
        }

        ip++;
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