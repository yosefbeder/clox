#include "vm.h"

void initVm(Vm* vm) {
    vm->stackTop = vm->stack;
}

void push(Vm* vm, double value) {
    *vm->stackTop = value;
    vm->stackTop++;
}

double pop(Vm* vm) {
    vm->stackTop--;
    return *vm->stackTop;
}

void runChunk(Vm* vm, Chunk* chunk) {
    #define NEXT_BYTE *(++ip)
    #define NEXT_CONSTANT chunk->constants.values[NEXT_BYTE]
    #define BINARY_OP(op)\
        {\
            double b = pop(vm);\
            double a = pop(vm);\
            push(vm, a op b);\
        }


    uint8_t* ip = chunk->code;
    
    while (1) {
        if (*ip == OP_CONSTANT) 
            push(vm, NEXT_CONSTANT);
        
        else if (*ip == OP_NEGATE) 
            *(vm->stackTop - 1) *= -1;
        
        else if (*ip == OP_ADD) BINARY_OP(+)
        
        else if (*ip == OP_SUBTRACT) BINARY_OP(-)

        else if (*ip == OP_MULTIPLY) BINARY_OP(*)

        else if (*ip == OP_DIVIDE) BINARY_OP(/)

        else if (*ip == OP_RETURN) {
            printf("%lf", pop(vm));
            break;
        }

        ip++;
    }

    #undef BINARY_OP
    #undef NEXT_CONSTANT
    #undef NEXT_BYTE
}