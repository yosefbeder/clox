#include "object.h"
#include "value.h"
#include "vm.h"
#include <string.h>

Obj* allocateObj(struct Vm* vm, size_t size, ObjType type) {
    Obj* ptr = malloc(size);

    ptr->type = type;
    ptr->next = (struct Obj*) vm->objects;
    vm->objects = ptr;

    return ptr;
}

ObjString* allocateObjString(struct Vm* vm, char* chars) {
    ObjString* ptr = (ObjString*) allocateObj(vm, sizeof(ObjString), OBJ_STRING);

    ptr->chars = chars;
    ptr->length = strlen(chars);

    return ptr;
}

ObjFunction* allocateObjFunction(struct Vm* vm) {
    ObjFunction* ptr = (ObjFunction*) allocateObj(vm, sizeof(ObjFunction), OBJ_FUNCTION);

    ptr->arity = 0;
    ptr->name = NULL;

    initChunk(&ptr->chunk);

    return ptr;
}

void freeObj(Obj* obj) {
    switch (obj->type) {
        case OBJ_STRING:
            free(((ObjString*) obj)->chars);
            break;
        case OBJ_FUNCTION:
            freeChunk(&((ObjFunction*) obj)->chunk);
            break;
    }
}