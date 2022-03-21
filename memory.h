#ifndef clox_memory_h
#define clox_memory_h

#include "hashmap.h"
#include "compiler.h"
#include "vm.h"
#include "object.h"

#define GROW_CAPACITY(capacity) capacity < 8 ? 8 : capacity * 2

#define ALLOCATE(vm, compiler, type, count) (type *)reallocate(vm, compiler, NULL, 0, sizeof(type) * (count))

#define FREE(vm, compiler, type, pointer) reallocate(vm, compiler, pointer, sizeof(type), 0);

#define GROW_ARRAY(vm, compiler, type, pointer, oldCount, newCount)      \
    (type *)reallocate(vm, compiler, pointer, sizeof(type) * (oldCount), \
                       sizeof(type) * (newCount))

#define FREE_ARRAY(vm, compiler, type, pointer, oldCount) \
    reallocate(vm, compiler, pointer, sizeof(type) * (oldCount), 0)

void *reallocate(struct Vm *, struct Compiler *, void *, size_t, size_t);

void collectGarbage(struct Vm *vm, struct Compiler *compiler);

#endif