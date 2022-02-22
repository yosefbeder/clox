#include "object.h"
#include "value.h"
#include "vm.h"

Obj* allocateObj(struct Vm* vm, size_t size, ObjType type) {
    Obj* ptr = malloc(size);

    ptr->type = type;
    ptr->next = (struct Obj*) vm->objects;
    vm->objects = ptr;

    return ptr;
}

ObjString* allocateObjString(struct Vm* vm, char* chars, int length) {
    ObjString* ptr = (ObjString*) allocateObj(vm, sizeof(ObjString), OBJ_STRING);

    ptr->chars = chars;
    ptr->length = length;

    return ptr;
}

void freeObj(Obj* obj) {
    if (obj->type == OBJ_STRING) {
        free(((ObjString*) obj)->chars);
    }
}