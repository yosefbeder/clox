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
    switch (value->type) {
        case VAL_STRING:
            printf("%s", value->as.string);
            break;
        default: {
            char str[16];
            toString(str, value);

            printf("%s", str);
        }
    }
}

static int equal(Value a, Value b) {
    if (a.type != b.type) return 0;

    switch (a.type) {
        case VAL_BOOL:
            return a.as.boolean == b.as.boolean;
        case VAL_NIL:
            return 1;
        case VAL_NUMBER:
            return a.as.number == b.as.number;
    }
}

static char* concat(char s1[], char s2[]) {
    char* new = malloc((strlen(s1) + strlen(s2) + 1) * sizeof(char));
    strcpy(new, s1);
    strcat(new, s2);
    return new;
}

Result runChunk(Vm* vm, Chunk* chunk) {
    #define NEXT_BYTE *(++ip)
    #define NEXT_CONSTANT chunk->constants.values[NEXT_BYTE]
    #define FREE_STRING(value)\
        if (value.type == VAL_STRING) {\
            free(value.as.string);\
        }
    #define NUMERIC_BINARY_OP(op)\
        {\
            Value b = pop(vm);\
            Value a = pop(vm);\
            if (a.type == VAL_NUMBER && b.type == VAL_NUMBER) {\
                push(vm, (Value) {VAL_NUMBER, { .number = a.as.number op b.as.number }});\
            } else {\
                FREE_STRING(a)\
                FREE_STRING(b)\
                reportError(ERROR_RUNTIME, &chunk->tokenArr.tokens[(int) (ip - chunk->code - (a.type != VAL_NUMBER? 4: 2))], "Both operands must be numbers");\
                return RESULT_RUNTIME_ERROR;\
            }\
        }
    #define CMP_BINARY_OP(op)\
        {\
            Value b = pop(vm);\
            Value a = pop(vm);\
            if (a.type == VAL_NUMBER && b.type == VAL_NUMBER) {\
                push(vm, (Value) {VAL_BOOL, { .boolean = a.as.number op b.as.number }});\
            } else {\
                FREE_STRING(a)\
                FREE_STRING(b)\
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
                FREE_STRING(operand)

                reportError(ERROR_RUNTIME, &chunk->tokenArr.tokens[(int) (ip - chunk->code - 2)], "Unary '-' operand must be a number");
                return RESULT_RUNTIME_ERROR;
            }
        }
        
        else if (*ip == OP_ADD) {
            Value b = pop(vm);
            Value a = pop(vm);

            if (a.type == VAL_NUMBER && b.type == VAL_NUMBER) {
                push(vm, (Value) {VAL_NUMBER, { .number = a.as.number + b.as.number }});
            } else if (a.type == VAL_STRING && b.type == VAL_STRING) {
                push(vm, (Value) {VAL_STRING, { .string = concat(a.as.string, b.as.string) }});

                free(a.as.string);
                free(b.as.string);
            } else if (a.type == VAL_STRING) {
                char bAsString[16];
                toString(bAsString, &b);

                push(vm, (Value) {VAL_STRING, { .string = concat(a.as.string, bAsString) }});

                free(a.as.string);
            } else if (b.type == VAL_STRING) {
                char aAsString[16];
                toString(aAsString, &a);

                push(vm, (Value) {VAL_STRING, { .string = concat(aAsString, b.as.string) }});

                free(b.as.string);
            } else {
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
                FREE_STRING(b);
                push(vm, a);
            }
            else {
                FREE_STRING(a)
                push(vm, b);
            }
        }

        else if (*ip == OP_AND) {
            Value b = pop(vm);
            Value a = pop(vm);

            if (!isTruthy(&a)) {
                FREE_STRING(b)
                push(vm, a);
            }
            else {
                FREE_STRING(a)
                push(vm, b);
            }
        }

        else if (*ip == OP_EQUAL) {
            Value b = pop(vm);
            Value a = pop(vm);


            push(vm, (Value) {VAL_BOOL, { .boolean = equal(a, b) }});
            FREE_STRING(a)
            FREE_STRING(b)
        }

        else if (*ip == OP_NOT_EQUAL) {
            Value b = pop(vm);
            Value a = pop(vm);

            push(vm, (Value) {VAL_BOOL, { .boolean = !equal(a, b) }});
            FREE_STRING(a)
            FREE_STRING(b)
        }

        else if (*ip == OP_GREATER) CMP_BINARY_OP(>)

        else if (*ip == OP_GREATER_OR_EQUAL) CMP_BINARY_OP(>=)

        else if (*ip == OP_LESS) CMP_BINARY_OP(<)

        else if (*ip == OP_LESS_OR_EQUAL) CMP_BINARY_OP(<=)

        else if (*ip == OP_BANG) {
            Value operand = pop(vm);
            uint8_t value = isTruthy(&operand);

            FREE_STRING(operand)

            push(vm, (Value) {VAL_BOOL, { .boolean = !value }});
        }

        else if (*ip == OP_RETURN) {
            Value poped = pop(vm);

            printValue(&poped);

            FREE_STRING(poped)

            putchar('\n');
            break;
        }

        ip++;
    }

    #undef CMP_BINARY_OP
    #undef NUMERIC_BINARY_OP
    #undef FREE_STRING
    #undef NEXT_CONSTANT
    #undef NEXT_BYTE

    return RESULT_SUCCESS;
}