#ifndef clox_line_h
#define clox_line_h

#include "common.h"

typedef struct {
    int capacity;
    int count;
    int* lines;
} LineArr;

void initLineArr(LineArr*);

void writeLineArr(LineArr*, int);

int getLine(LineArr*, int);

void freeLineArr(LineArr*);

#endif