#ifndef clox_memory_h
#define clox_memory_h

#include "hashmap.h"
#include "compiler.h"
#include "vm.h"
#include "object.h"

#define GC_GROW_FACTOR 2

#define GROW_CAPACITY(capacity) capacity < 8 ? 8 : capacity * 2

#define ALLOCATE(type, count) (type *)reallocate(NULL, 0, sizeof(type) * (count))

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0);

#define GROW_ARRAY(type, pointer, oldCount, newCount)      \
    (type *)reallocate(pointer, sizeof(type) * (oldCount), \
                       sizeof(type) * (newCount))

#define FREE_ARRAY(type, pointer, count) \
    reallocate(pointer, sizeof(type) * (count), 0)

void *reallocate(void *, size_t, size_t);

void collectGarbage();

#endif