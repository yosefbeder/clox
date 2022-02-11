#include "line.h"
#include "memory.h"

void initLineArr(LineArr* lineArr) {
    lineArr->capacity = 0;
    lineArr->count = 0;
    lineArr->lines = NULL;
}

void writeLineArr(LineArr* lineArr, int line) {
    // check the capacity
    if (lineArr->count == lineArr->capacity) {
        lineArr->capacity = GROW_CAPACITY(lineArr->capacity);
        lineArr->lines = realloc(lineArr->lines, lineArr->capacity * sizeof(double));

        if (lineArr->lines == NULL) {
            exit(1);
        }
    }

    /*
        Inserting logic
            * Check whether the last int is equal to the passed one.
                * If so increment the one before it.
                * Else push 1, line
    */
    if (lineArr->count == 0) {
        lineArr->lines[lineArr->count++] = 1;
        lineArr->lines[lineArr->count++] = line;

        return;
    }

    if (lineArr->lines[lineArr->count - 1] == line) {
        lineArr->lines[lineArr->count - 2]++;
    } else {
        lineArr->lines[lineArr->count++] = 1;
        lineArr->lines[lineArr->count++] = line;
    }
}

int getLine(LineArr* lineArr, int offset) {
    int i, j;

    for(i = j = 0; i < lineArr->count; i+=2) {
        j += lineArr->lines[i];

        if(j > offset) return lineArr->lines[i + 1];
    }

    return 0;
}

void freeLineArr(LineArr* lineArr) {
    free(lineArr->lines);

    initLineArr(lineArr);
}