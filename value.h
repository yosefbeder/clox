#ifndef clox_value_h
#define clox_value_h

#include "common.h"

typedef struct {
    int count;
    int capacity;
    double* values;
} ValueArr;

void initValueArr(ValueArr*);

void writeValueArr(ValueArr*, double);

void freeValueArr(ValueArr*);

#endif