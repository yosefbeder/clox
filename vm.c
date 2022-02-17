#include "vm.h"

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

Result runChunk(Vm* vm, Chunk* chunk) {
    #define NEXT_BYTE *(++ip)
    #define NEXT_CONSTANT chunk->constants.values[NEXT_BYTE]
    #define BINARY_OP(op)\
        {\
            Value b = pop(vm);\
            Value a = pop(vm);\
            if (a.type == VAL_NUMBER && b.type == VAL_NUMBER) {\
                push(vm, (Value) {VAL_NUMBER, {.number = a.as.number op b.as.number}});\
            } else {\
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
                return RESULT_RUNTIME_ERROR;
            }
        }
        
        else if (*ip == OP_ADD) BINARY_OP(+)
        
        else if (*ip == OP_SUBTRACT) BINARY_OP(-)

        else if (*ip == OP_MULTIPLY) BINARY_OP(*)

        else if (*ip == OP_DIVIDE) BINARY_OP(/)

        else if (*ip == OP_RETURN) {
            printValue(pop(vm));
            putchar('\n');
            break;
        }

        ip++;
    }

    #undef BINARY_OP
    #undef NEXT_CONSTANT
    #undef NEXT_BYTE

    return RESULT_SUCCESS;
}