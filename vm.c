#include "vm.h"
#include "error.h"

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

static void printValue(Value value) {
    switch (value.type) {
        case VAL_BOOL:
            if (value.as.boolean) {
                printf("true");
            } else {
                printf("false");
            }
            break;
        case VAL_NIL:
            printf("nil");
            break;
        case VAL_NUMBER:
            printf("%lf", value.as.number);
            break;
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

Result runChunk(Vm* vm, Chunk* chunk) {
    #define NEXT_BYTE *(++ip)
    #define NEXT_CONSTANT chunk->constants.values[NEXT_BYTE]
    #define NUMERIC_BINARY_OP(op)\
        {\
            Value b = pop(vm);\
            Value a = pop(vm);\
            if (a.type == VAL_NUMBER && b.type == VAL_NUMBER) {\
                push(vm, (Value) {VAL_NUMBER, { .number = a.as.number op b.as.number }});\
            } else {\
                reportError(ERROR_RUNTIME, &chunk->tokenArr.tokens[(int) (ip - chunk->code - (a.type != VAL_NUMBER? 4: 2))], "Both operands should be numbers");\
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
                reportError(ERROR_RUNTIME, &chunk->tokenArr.tokens[(int) (ip - chunk->code - (a.type != VAL_NUMBER? 4: 2))], "Both operands should be numbers");\
                return RESULT_RUNTIME_ERROR;\
            }\
        }


    uint8_t* ip = chunk->code;
    
    while (1) {
        if (*ip == OP_CONSTANT) 
            push(vm, NEXT_CONSTANT);
        
        else if (*ip == OP_NEGATE) {
            if ((vm->stackTop - 1)->type == VAL_NUMBER) {
                (vm->stackTop - 1)->as.number *= -1;
            } else {
                reportError(ERROR_RUNTIME, &chunk->tokenArr.tokens[(int) (ip - chunk->code - 2)], "Unary '-' operand should be a number");
                return RESULT_RUNTIME_ERROR;
            }
        }
        
        else if (*ip == OP_ADD) NUMERIC_BINARY_OP(+)
        
        else if (*ip == OP_SUBTRACT) NUMERIC_BINARY_OP(-)

        else if (*ip == OP_MULTIPLY) NUMERIC_BINARY_OP(*)

        else if (*ip == OP_DIVIDE) NUMERIC_BINARY_OP(/)

        else if (*ip == OP_OR) {
            Value b = pop(vm);
            Value a = pop(vm);

            if (isTruthy(&a)) push(vm, a);
            else push(vm, b);
        }

        else if (*ip == OP_AND) {
            Value b = pop(vm);
            Value a = pop(vm);

            if (!isTruthy(&a)) push(vm, a);
            else push(vm, b);
        }

        else if (*ip == OP_EQUAL) {
            Value b = pop(vm);
            Value a = pop(vm);

            push(vm, (Value) {VAL_BOOL, { .boolean = equal(a, b) }});
        }

        else if (*ip == OP_NOT_EQUAL) {
            Value b = pop(vm);
            Value a = pop(vm);

            push(vm, (Value) {VAL_BOOL, { .boolean = !equal(a, b) }});   
        }

        else if (*ip == OP_GREATER) CMP_BINARY_OP(>)

        else if (*ip == OP_GREATER_OR_EQUAL) CMP_BINARY_OP(>=)

        else if (*ip == OP_LESS) CMP_BINARY_OP(<)

        else if (*ip == OP_LESS_OR_EQUAL) CMP_BINARY_OP(<=)

        else if (*ip == OP_BANG) {
            uint8_t value = isTruthy((vm->stackTop - 1));

            (vm->stackTop - 1)->type = VAL_BOOL;
            (vm->stackTop - 1)->as.boolean = !value;
        }

        else if (*ip == OP_RETURN) {
            printValue(pop(vm));
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