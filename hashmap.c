#include "hashmap.h"
#include "memory.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

uint32_t hashString(char key[], int length)
{
    uint32_t hash = 2166136261u;

    int i = 0;

    while (i < length)
    {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
        i++;
    }

    return hash;
}

void initHashMap(HashMap *hashMap)
{
    hashMap->capacity = 0;
    hashMap->count = 0;
    hashMap->entries = NULL;
}

// returns a tombstone, newEntry, or existingEntry
static Entry *findEntry(Entry *entries, int capacity, struct ObjString *key)
{
#define NEXT_INDEX(index, capacity) (index + 1) % capacity

    if (capacity == 0)
        return NULL;

    int index = key->hash % capacity;
    Entry *tombstone = NULL;

    while (true)
    {
        Entry *entry = &entries[index];

        if (entry->key == NULL)
        {
            if (tombstone)
                return tombstone;

            return entry;
        }

        if (entry->isTombstone)
        {
            if (tombstone == NULL)
                tombstone = entry;

            index = NEXT_INDEX(index, capacity);
            continue;
        }

        if (entry->key == key)
            return entry;

        index = NEXT_INDEX(index, capacity);
    }

    return NULL;

#undef NEXT_INDEX
}

// returns whether the entry was new or not
bool hashMapInsert(HashMap *hashMap, struct ObjString *key, Value value)
{
    push(OBJ(key));
    push(value);

    if (hashMap->count == hashMap->capacity)
    {
        int capacity = GROW_CAPACITY(hashMap->capacity);
        Entry *entries = ALLOCATE(Entry, capacity);

        // clear the new one
        for (int i = 0; i < capacity; i++)
        {
            entries[i].key = NULL;
            entries[i].isTombstone = false;
            entries[i].value = NIL;
        }

        // transfer the content of the old to it
        for (int i = 0; i < hashMap->capacity; i++)
        {
            Entry *oldEntry = &hashMap->entries[i];

            if (oldEntry->key != NULL)
            {
                Entry *entry = findEntry(entries, capacity, oldEntry->key);

                entry->key = oldEntry->key;
                entry->value = oldEntry->value;
                entry->isTombstone = oldEntry->isTombstone;
            }
        }

        // free the old one
        FREE_ARRAY(Entry, hashMap->entries, hashMap->capacity);

        hashMap->capacity = capacity;
        hashMap->entries = entries;
    }

    Entry *entry = findEntry(hashMap->entries, hashMap->capacity, key);
    bool isNew = entry->key == NULL;

    entry->key = key;
    entry->value = value;

    hashMap->count++;

    pop();
    pop();

    return isNew;
}

void hashMapInsertAll(HashMap *target, HashMap *source)
{
    for (int i = 0; i < source->capacity; i++)
    {
        Entry *entry = &source->entries[i];
        if (entry->key != NULL && !entry->isTombstone)
            hashMapInsert(target, entry->key, entry->value);
    }
}

Value *hashMapGet(HashMap *hashMap, struct ObjString *key)
{
    Entry *entry = findEntry(hashMap->entries, hashMap->capacity, key);

    if (entry->key != key)
        return NULL;

    return &entry->value;
}

struct ObjString *findKey(HashMap *hashMap, char *keyChars, int keyLength, uint32_t keyHash)
{
    for (int i = 0; i < hashMap->capacity; i++)
    {
        Entry *entry = &hashMap->entries[i];

        if (entry->key != NULL && !entry->isTombstone && entry->key->hash == keyHash && entry->key->length == keyLength && strncmp(entry->key->chars, keyChars, keyLength) == 0)
            return entry->key;
    }

    return NULL;
}

// returns whether the entry existed or not
bool hashMapRemove(HashMap *hashMap, struct ObjString *key)
{
    // find the key
    Entry *entry = findEntry(hashMap->entries, hashMap->capacity, key);

    // turn it to a tombstone
    if (entry->key != NULL)
    {
        hashMap->count--;
        entry->isTombstone = true;
        return true;
    }

    return false;
}
