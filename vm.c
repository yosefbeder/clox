#include "vm.h"
#include "error.h"
#include <string.h>

void initVm(Vm* vm) {
    vm->stackTop = vm->stack;
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

static int equal(Value* a, Value* b) {
    if (a->type != b->type) return 0;

    if (IS_STRING(a)) {
        return strcmp(AS_STRING(a)->chars, AS_STRING(b)->chars) == 0? 1: 0;
    }

    switch (a->type) {
        case VAL_BOOL:
            return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NIL:
            return 1;
        case VAL_NUMBER:
            return AS_NUMBER(a) == AS_NUMBER(b);
    }
}

static ObjString* concat(ObjString* s1, ObjString* s2) {
    size_t length = s1->length + s2->length + 1;

    //>> REVIEW
    char* chars = malloc(length);
    strcpy(chars, s1->chars);
    strcat(chars, s2->chars);
    //<<

    return allocateObjString(length, chars);
}

Result runChunk(Vm* vm, Chunk* chunk) {
    #define NEXT_BYTE *(++ip)
    #define NEXT_CONSTANT chunk->constants.values[NEXT_BYTE]
    #define NUMERIC_BINARY_OP(op)\
        {\
            Value b = pop(vm);\
            Value a = pop(vm);\
            if (IS_NUMBER((&a)) && IS_NUMBER((&b))) {\
                push(vm, (Value) {VAL_NUMBER, { .number = AS_NUMBER((&a)) op AS_NUMBER((&b)) }});\
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
                push(vm, (Value) {VAL_BOOL, { .boolean = AS_NUMBER((&a)) op AS_NUMBER((&b)) }});\
            } else {\
                reportError(ERROR_RUNTIME, &chunk->tokenArr.tokens[(int) (ip - chunk->code - (a.type != VAL_NUMBER? 4: 2))], "Both operands must be numbers");\
                return RESULT_RUNTIME_ERROR;\
            }\
        }

    uint8_t* ip = chunk->code;
    
    while (1) {
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
                push(vm, (Value) {VAL_NUMBER, { .number = AS_NUMBER((&a)) + AS_NUMBER((&b)) }});
            } else if (IS_STRING((&a)) && IS_STRING((&b))) {
                push(vm, STRING(concat(AS_STRING((&a)), AS_STRING((&b)))));
            } else if (IS_STRING((&a))) { //>>OPTIMIZE
                char* bAsString = malloc(16);

                primitiveAsString(bAsString, &b);

                push(vm, STRING(concat(AS_STRING((&a)), allocateObjString(strlen(bAsString), bAsString))));
            } else if (IS_STRING((&b))) {
                char* aAsString = malloc(16);

                primitiveAsString(aAsString, &a);

                push(vm, STRING(concat(allocateObjString(strlen(aAsString), aAsString), AS_STRING((&b)))));
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


            push(vm, (Value) {VAL_BOOL, { .boolean = equal(&a, &b) }});
        }

        else if (*ip == OP_NOT_EQUAL) {
            Value b = pop(vm);
            Value a = pop(vm);

            push(vm, (Value) {VAL_BOOL, { .boolean = !equal(&a, &b) }});
        }

        else if (*ip == OP_GREATER) CMP_BINARY_OP(>)

        else if (*ip == OP_GREATER_OR_EQUAL) CMP_BINARY_OP(>=)

        else if (*ip == OP_LESS) CMP_BINARY_OP(<)

        else if (*ip == OP_LESS_OR_EQUAL) CMP_BINARY_OP(<=)

        else if (*ip == OP_BANG) {
            Value operand = pop(vm);
            uint8_t value = isTruthy(&operand);


            push(vm, (Value) {VAL_BOOL, { .boolean = !value }});
        }

        else if (*ip == OP_RETURN) {
            Value poped = pop(vm);

            printValue(&poped);

            putchar('\n');
            break;
        }

        ip++;
    }

    #undef CMP_BINARY_OP
    #undef NUMERIC_BINARY_OP
    #undef NEXT_CONSTANT
    #undef NEXT_BYTE

    return RESULT_SUCCESS;
}