#include "value.h"
#include "memory.h"
#include <string.h>

bool isTruthy(Value *value)
{
    switch (value->type)
    {
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

void printValue(Value *value)
{
    switch (value->type)
    {
    case VAL_BOOL:
        printf("%s", AS_BOOL(value) ? "true" : "false");
        break;
    case VAL_NIL:
        printf("nil");
        break;
    case VAL_NUMBER:
        printf("%.2lf", AS_NUMBER(value));
        break;
    case VAL_OBJ:
        switch (AS_OBJ(value)->type)
        {
        case OBJ_STRING:
            printf("%s", AS_STRING(value)->chars);
            break;
        case OBJ_FUNCTION:
        {
            ObjString *name = AS_FUNCTION(value)->name;

            if (name != NULL)
            {
                printf("<fun %s>", name->chars);
            }
            else
            {
                printf("<script>");
            }

            break;
        }
        case OBJ_NATIVE:
            printf("<native fun>");
            break;
        case OBJ_CLOSURE:
            printf("Closure -> ");
            printValue(&OBJ(AS_CLOSURE(value)->function));
            break;
        case OBJ_UPVALUE:
            printf("UpValue -> ");
            printValue(AS_UPVALUE(value)->location);
            break;
        }
    }
}

bool equal(Value *a, Value *b)
{
    if (a->type != b->type)
        return false;

    switch (a->type)
    {
    case VAL_BOOL:
        return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NIL:
        return true;
    case VAL_NUMBER:
        return AS_NUMBER(a) == AS_NUMBER(b);
    case VAL_OBJ:
        switch (AS_OBJ(a)->type)
        {
        // TODO add string interning
        case OBJ_STRING:
            return strcmp(AS_STRING(a)->chars, AS_STRING(b)->chars) == 0 ? true : false;
        default:
            return a == b;
        }
    }

    return false;
}
