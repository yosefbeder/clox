#include "hashmap.h"
#include "memory.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h> 
#include <stdio.h> 

static uint32_t hashString(char key[]) {
    uint32_t hash = 2166136261u;
    
    int i = 0;

    while (key[i] != '\0') {
        hash ^= (uint8_t) key[i];
        hash *= 16777619;
        i++;
    }

    return hash;
}

void initHashMap(HashMap* hashMap) {
    hashMap->capacity = 0;
    hashMap->count = 0;
    hashMap->entries = NULL;
}

static Entry* findEntry(Entry* entries, int capacity, ObjString* key) {
    #define NEXT_INDEX(index, capacity) (index + 1) % capacity

    uint32_t hash = hashString(key->chars);
    int index = hash % capacity;
    Entry* tombstone = NULL;

    while (true) {
        Entry* entry = &entries[index];

        if (entry->key == NULL) {
            if (tombstone) return tombstone; 

            return entry;
        }

        if (entry->isTombstone) {
            if (tombstone == NULL) tombstone = entry;

            index = NEXT_INDEX(index, capacity);
            continue;
        }

        if (strcmp(entry->key->chars, key->chars)) return entry;

        index = NEXT_INDEX(index, capacity);
    }

    return &entries[index];

    #undef NEXT_INDEX
}

// returns whether the entry was new or not
bool hashMapInsert(HashMap* hashMap, ObjString* key, Value* value) {
    if (hashMap->count == hashMap->capacity) {
        int capacity = GROW_CAPACITY(hashMap->capacity);
        Entry* entries = realloc(NULL, capacity * sizeof(Entry));

        // clear the new one
        for (int i = 0; i < capacity; i++) {
            entries[i].key = NULL;
            entries[i].isTombstone = false;
        }

        // transfer the content of the old to it
        for (int i = 0; i < hashMap->capacity; i++) {
            Entry* oldEntry = &hashMap->entries[i];

            if (oldEntry->key != NULL) {
                Entry* entry = findEntry(entries, capacity, oldEntry->key);

                entry->key = oldEntry->key;
                entry->value = oldEntry->value;
                entry->isTombstone = oldEntry->isTombstone;
            }
        }

        // free the old one
        free(hashMap->entries);

        hashMap->capacity = capacity;
        hashMap->entries = entries;
    }

    Entry* entry = findEntry(hashMap->entries, hashMap->capacity, key);
    bool isNew = entry == NULL;

    entry->key = key;
    entry->value = value;

    hashMap->count++;

    return isNew;
}

Value* hashMapGet(HashMap* hashMap, ObjString* key) {
    Entry* entry = findEntry(hashMap->entries, hashMap->capacity, key);

    if (!entry->isTombstone) {
        return entry->value;
    }

    return NULL;
}

// returns whether the entry existed or not
bool hashMapRemove(HashMap* hashMap, ObjString* key) {
    // find the key
    Entry* entry = findEntry(hashMap->entries, hashMap->capacity, key);

    // turn it to a tombstone
    if (entry->key != NULL) {
        hashMap->count--;
        entry->isTombstone = true;
        return true;
    }

    return false;
}