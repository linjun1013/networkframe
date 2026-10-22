

#ifndef NETWORK_HASH_H
#define NETWORK_HASH_H

#include"util.h"



/* hash bucket structure */
typedef struct HashTableBucket_ {
    void *data;
    uint16_t size;
    struct HashTableBucket_ *next;
} HashTableBucket;

/* hash table structure */
typedef struct HashTable_ {
    HashTableBucket **array;
    uint32_t array_size;
    uint32_t (*Hash)(struct HashTable_ *, void *, uint16_t);
    char (*Compare)(void *, uint16_t, void *, uint16_t);
    void (*Free)(void *);
} HashTable;

#define HASH_NO_SIZE 0

/* prototypes */
HashTable* HashTableInit(uint32_t, uint32_t (*Hash)(struct HashTable_ *, void *, uint16_t), char (*Compare)(void *, uint16_t, void *, uint16_t), void (*Free)(void *));
void HashTableFree(HashTable *);
int HashTableAdd(HashTable *, void *, uint16_t);
int HashTableRemove(HashTable *, void *, uint16_t);
void *HashTableLookup(HashTable *, void *, uint16_t);
uint32_t HashTableGenericHash(HashTable *, void *, uint16_t);
char HashTableDefaultCompare(void *, uint16_t, void *, uint16_t);


#endif /* NETWORK_HASH_H */
