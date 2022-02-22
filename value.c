#include "value.h"
#include "memory.h"
#include <string.h>

void primitiveAsString(char s[], Value* value) {
    switch (value->type) {
        case VAL_BOOL:
            strcpy(s, AS_BOOL(value)? "true": "false");
            break;
        case VAL_NIL:
            strcpy(s, "nil");
            break;
        case VAL_NUMBER:
            sprintf(s, "%.2lf", AS_NUMBER(value));
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

bool isTruthy(Value* value) {
    switch(value->type) {
        case VAL_BOOL:
            return value->as.boolean;
        case VAL_NIL:
            return false;
        case VAL_NUMBER:
            return value->as.number;
        case VAL_OBJ:
            return true;
    }
}

void freeValueArr(ValueArr* valueArr) {
    free(valueArr->values);

    initValueArr(valueArr);
}