#ifndef HASH_MAP_H
#define HASH_MAP_H

#include "item.h"
#include "linked-list.h"

typedef struct HashMapArray {
    size_t capacity;
    Item **array;
} HashMapArray;

typedef struct HashMap {
    size_t len;
    HashMapArray **arrays;
    size_t arraysLen;
    size_t primaryArray;
    size_t arraysCapacity;
    size_t reHashItemIndex;
    size_t reHashArrayIndex;
} HashMap;

HashMap *createHashMap();
void freeHashMap(HashMap *hashMap);
Item *put(HashMap *hashMap, char *key, Interval *interval);
Item *get(HashMap *hashMap, const char *key);
void delete(HashMap *hashMap, char *key);
LinkedList *scanHash(HashMap *hashMap, long long int *cursor, const char *match, long long int count);
#endif