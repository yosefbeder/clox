#include "value.h"
#include "memory.h"

void initValueArr(ValueArr* valueArr) {
    valueArr->count = 0;
    valueArr->capacity = 0;
    valueArr->values = NULL;
}

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

void freeValueArr(ValueArr* valueArr) {
    free(valueArr->values);

    initValueArr(valueArr);
}