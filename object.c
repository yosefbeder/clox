#include "object.h"
#include "value.h"

Obj* allocateObj(size_t size, ObjType type) {
    Obj* ptr = malloc(size);

    ptr->type = type;

    return ptr;
}

ObjString* allocateObjString(char* chars, int length) {
    ObjString* ptr = (ObjString*) allocateObj(sizeof(ObjString), OBJ_STRING);

    ptr->chars = chars;
    ptr->length = length;

    return ptr;
}

void freeObj(Obj* obj) {
    if (obj->type == OBJ_STRING) {
        free(((ObjString*) obj)->chars);
    }
}