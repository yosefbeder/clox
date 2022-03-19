#ifndef clox_hashmap_h
#define clox_hashmap_h

#include "common.h"
#include "object.h"
#include "value.h"

typedef struct
{
    ObjString *key;
    Value value;
    bool isTombstone;
} Entry;

typedef struct
{
    int capacity;
    int count;
    Entry *entries;
} HashMap;

void initHashMap(HashMap *);

bool hashMapInsert(HashMap *, ObjString *, Value *);

Value *hashMapGet(HashMap *, ObjString *);

bool hashMapRemove(HashMap *, ObjString *);

#endif