#ifndef clox_value_h
#define clox_value_h

#include "common.h"

typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_STRING,
} ValueType;

typedef struct {
    ValueType type;
    union
    {
        uint8_t boolean;
        double number;
        char* string;
    } as;
} Value;

typedef struct {
    int count;
    int capacity;
    Value *values;
} ValueArr;

void toString(char[], Value*);

void initValueArr(ValueArr *);

void writeValueArr(ValueArr *, Value);

int isTruthy(Value*);

void freeValueArr(ValueArr *);

#endif