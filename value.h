#ifndef clox_value_h
#define clox_value_h

#include "common.h"

typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJ,
} ValueType;

struct Obj;
typedef struct {
    ValueType type;
    union
    {
        uint8_t boolean;
        double number;
        struct Obj* obj;
    } as;
} Value;

#define IS_BOOL(val) (val->type == VAL_BOOL)
#define AS_BOOL(val) val->as.boolean
#define BOOL(val) (Value) {VAL_BOOL, { .boolean = val }}

#define IS_NUMBER(val) (val->type == VAL_NUMBER)
#define AS_NUMBER(val) val->as.number
#define NUMBER(val) (Value) {VAL_NUMBER, { .number = val }}

#define IS_NIL(val) (val->type == VAL_NIL)
#define NIL ((Value) {VAL_NIL, { .number = 0 }})

typedef struct {
    int count;
    int capacity;
    Value *values;
} ValueArr;

void primitiveAsString(char[], Value*);

void initValueArr(ValueArr *);

void writeValueArr(ValueArr *, Value);

bool isTruthy(Value*);

void freeValueArr(ValueArr *);

#endif