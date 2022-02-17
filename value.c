#include "value.h"
#include "memory.h"
#include <string.h>

void toString(char str[], Value* value) {
    switch (value->type) {
        case VAL_BOOL:
            strcpy(str, value->as.boolean? "true": "false");
            break;
        case VAL_NIL:
            strcpy(str, "nil");
            break;
        case VAL_NUMBER: {
            sprintf(str, "%lf", value->as.number);
            break;
        }
        // this one shouldn't be used
        case VAL_STRING:
            strcpy(str, value->as.string);
            break;
    }
}

void initValueArr(ValueArr* valueArr) {
    valueArr->count = 0;
    valueArr->capacity = 0;
    valueArr->values = NULL;
}

//TODO take a pointer instead
void writeValueArr(ValueArr* valueArr, Value value) {
    // check the capacity
    if (valueArr->count == valueArr->capacity) {
        valueArr->capacity = GROW_CAPACITY(valueArr->capacity);
        valueArr->values = realloc(valueArr->values, valueArr->capacity * sizeof(Value));

        if (valueArr->values == NULL) {
            exit(1);
        }
    }

    // insert
    valueArr->values[valueArr->count++] = value;
}

int isTruthy(Value* value) {
    switch(value->type) {
        case VAL_BOOL:
            return value->as.boolean;
        case VAL_NIL:
            return 0;
        case VAL_NUMBER:
            return value->as.number;
        case VAL_STRING:
            return 1;
    }
}

void freeValueArr(ValueArr* valueArr) {
    free(valueArr->values);

    initValueArr(valueArr);
}