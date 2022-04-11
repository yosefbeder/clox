#ifndef clox_value_h
#define clox_value_h

#include "common.h"

typedef enum
{
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJ,
    VAL_STRING,
} ValueType;

struct Obj *obj;
typedef struct
{
    ValueType type;
    union
    {
        uint8_t boolean;
        double number;
        struct Obj *obj;
        char string[8];
    } as;
} Value;

#define IS_BOOL(val) ((val).type == VAL_BOOL)
#define AS_BOOL(val) ((val).as.boolean)
#define BOOL(val)                    \
    (Value)                          \
    {                                \
        VAL_BOOL, { .boolean = val } \
    }

#define IS_NUMBER(val) ((val).type == VAL_NUMBER)
#define AS_NUMBER(val) ((val).as.number)
#define NUMBER(val)                   \
    (Value)                           \
    {                                 \
        VAL_NUMBER, { .number = val } \
    }

#define IS_NIL(val) ((val).type == VAL_NIL)
#define NIL ((Value){VAL_NIL, {.number = 0}})

#define IS_OBJ(val) ((val).type == VAL_OBJ)
#define AS_OBJ(val) ((val).as.obj)
#define OBJ(val)                       \
    (Value)                            \
    {                                  \
        VAL_OBJ, { .obj = (Obj *)val } \
    }

#define IS_STRING(val) ((val).type == VAL_STRING)
#define AS_STRING(val) ((val).as.string)

bool isTruthy(Value);

bool equal(Value, Value);

void printValue(Value);

#endif