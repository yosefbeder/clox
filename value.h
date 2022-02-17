#ifndef clox_value_h
#define clox_value_h

#include "common.h"

typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
} ValueType;

typedef struct {
    ValueType type;
    union
    {
        uint8_t boolean;
        double number;
    } as;
} Value;

typedef struct {
    int count;
    int capacity;
    Value *values;
} ValueArr;

void initValueArr(ValueArr *);

void writeValueArr(ValueArr *, Value);

int isTruthy(Value*);

void freeValueArr(ValueArr *);

#endif