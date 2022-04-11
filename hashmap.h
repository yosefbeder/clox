#ifndef clox_hashmap_h
#define clox_hashmap_h

#include "common.h"
#include "value.h"

struct ObjString;
typedef struct
{
    Value key;
    Value value;
    bool isTombstone;
} Entry;

typedef struct
{
    int capacity;
    int count;
    Entry *entries;
} HashMap;

uint32_t hashString(char[], int);

void initHashMap(HashMap *);

bool hashMapInsert(HashMap *, Value, Value);

void hashMapInsertAll(HashMap *, HashMap *);

Value *hashMapGet(HashMap *, Value);

struct ObjString *findKey(HashMap *, char *, int, uint32_t);

bool hashMapRemove(HashMap *, Value);

#endif