#ifndef clox_hashmap_h
#define clox_hashmap_h

#include "common.h"
#include "value.h"

struct ObjString;
typedef struct
{
    struct ObjString *key;
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

bool hashMapInsert(HashMap *, struct ObjString *, Value *);

Value *hashMapGet(HashMap *, struct ObjString *);

bool hashMapRemove(HashMap *, struct ObjString *);

#endif