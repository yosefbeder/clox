#include "value.h"
#include "memory.h"
#include <string.h>

bool isTruthy(Value value)
{
    switch (value.type)
    {
    case VAL_BOOL:
        return value.as.boolean;
    case VAL_NIL:
        return false;
    case VAL_NUMBER:
        return value.as.number;
    case VAL_OBJ:
        return true;
    }
}

#define TAB_SIZE 4
void printValue(Value value)
{
    switch (value.type)
    {
    case VAL_BOOL:
        printf("%s", AS_BOOL(value) ? "true" : "false");
        break;
    case VAL_NIL:
        printf("nil");
        break;
    case VAL_NUMBER:
        printf("%g", AS_NUMBER(value));
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
                printf("<anonymous fun>");
            }

            break;
        }
        case OBJ_NATIVE:
            printf("<native fun>");
            break;
        case OBJ_CLOSURE:
#ifdef DEBUG_WRAPPERS
            printf("Closure -> ");
#endif
            printValue(OBJ(AS_CLOSURE(value)->function));
            break;
        case OBJ_UPVALUE:
#ifdef DEBUG_WRAPPERS
            printf("UpValue -> ");
#endif
            printValue(*AS_UPVALUE(value)->location);
            break;
        case OBJ_CLASS:
        {
            ObjClass *klass = AS_CLASS(value);

            printf("<class %s>", klass->name->chars);
            break;
        }
        case OBJ_INSTANCE:
        {
            ObjInstance *instance = AS_INSTANCE(value);
            HashMap fields = instance->fields;

            printf("<instanceof %s> {%s", instance->klass->name->chars, instance->fields.count > 0 ? "\n" : "");
            for (int i = 0; i < fields.capacity; i++)
            {
                Entry *entry = &fields.entries[i];

                if (entry->key == NULL || entry->isTombstone)
                    continue;

                for (int i = 0; i < TAB_SIZE; i++)
                {
                    putchar(' ');
                }

                printValue(OBJ(entry->key));
                printf(": ");
                printValue(entry->value);
                printf(",\n");
            }
            putchar('}');
            break;
        }
        case OBJ_BOUND_METHOD:
#ifdef DEBUG_WRAPPERS
            printf("BoundMethod -> ");
#endif
            printValue(OBJ(AS_BOUND_METHOD(value)->method->function));
            break;
        }
    default:;
    }
}

bool equal(Value a, Value b)
{
    if (a.type != b.type)
        return false;

    switch (a.type)
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
            return AS_OBJ(a) == AS_OBJ(b);
        }
    }

    return false;
}
