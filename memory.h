#ifndef clox_memory_h
#define clox_memory_h

#include "hashmap.h"
#include "compiler.h"
#include "vm.h"
#include "object.h"

#define GC_STRESS_TEST_MODE
#define GC_DEBUG_MODE

#define GROW_CAPACITY(capacity) capacity < 8 ? 8 : capacity * 2

void collectGarbage(struct Vm *vm, struct Compiler *compiler);

#endif