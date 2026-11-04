/* Copyright (C) 2007-2010 Open Information Security Foundation
 *
 * You can copy, redistribute or modify this Program under the terms of
 * the GNU General Public License version 2 as published by the Free
 * Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

/**
 * \file
 *
 * \author Victor Julien <victor@inliniac.net>
 *
 * Chained hash table implementation
 *
 * The 'Free' pointer can be used to have the API free your
 * hashed data. If it's NULL it's the callers responsibility
 */


#include "util-hash.h"
#include<string.h>
#include<stdlib.h>
#include<stdio.h>

HashTable* HashTableInit(uint32_t size, uint32_t (*Hash)(struct HashTable_ *, void *, uint16_t), char (*Compare)(void *, uint16_t, void *, uint16_t), void (*Free)(void *)) {

    HashTable *ht = NULL;

    if (size == 0) {
        goto error;
    }

    if (Hash == NULL) {
        //printf("ERROR: HashTableInit no Hash function\n");
        goto error;
    }

    /* setup the filter */
    ht = calloc(1, sizeof(HashTable));
    if (unlikely(ht == NULL))
        goto error;
    ht->array_size = size;
    ht->Hash = Hash;
    ht->Free = Free;

    if (Compare != NULL)
        ht->Compare = Compare;
    else
        ht->Compare = HashTableDefaultCompare;

    /* setup the bitarray */
    ht->array = calloc(ht->array_size, sizeof(HashTableBucket *));
    if (ht->array == NULL)
        goto error;

    return ht;

error:
    if (ht != NULL) {
        if (ht->array != NULL)
           free(ht->array);

        free(ht);
    }
    return NULL;
}

void HashTableFree(HashTable *ht)
{
    uint32_t i = 0;

    if (ht == NULL)
        return;

    /* free the buckets */
    for (i = 0; i < ht->array_size; i++) {
        HashTableBucket *hashbucket = ht->array[i];
        while (hashbucket != NULL) {
            HashTableBucket *next_hashbucket = hashbucket->next;
            if (ht->Free != NULL)
                ht->Free(hashbucket->data);
            free(hashbucket);
            hashbucket = next_hashbucket;
        }
    }

    /* free the array */
    if (ht->array != NULL)
        free(ht->array);

    free(ht);
}

int HashTableAdd(HashTable *ht, void *data, uint16_t datalen)
{
    if (ht == NULL || data == NULL)
        return -1;

    uint32_t hash = ht->Hash(ht, data, datalen);

    HashTableBucket *hb = calloc(1, sizeof(HashTableBucket));
    if (unlikely(hb == NULL))
        goto error;
    hb->data = data;
    hb->size = datalen;
    hb->next = NULL;

    if (hash >= ht->array_size) {
       printf("attempt to insert element out of hash array\n");
        goto error;
    }

    if (ht->array[hash] == NULL) {
        ht->array[hash] = hb;
    } else {
        hb->next = ht->array[hash];
        ht->array[hash] = hb;
    }


    return 0;

error:
    if (hb != NULL)
        free(hb);
    return -1;
}

int HashTableRemove(HashTable *ht, void *data, uint16_t datalen)
{
    uint32_t hash = ht->Hash(ht, data, datalen);

    if (ht->array[hash] == NULL) {
        return -1;
    }

    if (ht->array[hash]->next == NULL) {
        if (ht->Free != NULL)
            ht->Free(ht->array[hash]->data);
        free(ht->array[hash]);
        ht->array[hash] = NULL;
        return 0;
    }

    HashTableBucket *hashbucket = ht->array[hash], *prev_hashbucket = NULL;
    do {
        if (ht->Compare(hashbucket->data,hashbucket->size,data,datalen) == 1) {
            if (prev_hashbucket == NULL) {
                /* root bucket */
                ht->array[hash] = hashbucket->next;
            } else {
                /* child bucket */
                prev_hashbucket->next = hashbucket->next;
            }

            /* remove this */
            if (ht->Free != NULL)
                ht->Free(hashbucket->data);
            free(hashbucket);
            return 0;
        }

        prev_hashbucket = hashbucket;
        hashbucket = hashbucket->next;
    } while (hashbucket != NULL);

    return -1;
}

void *HashTableLookup(HashTable *ht, void *data, uint16_t datalen)
{
    uint32_t hash = 0;

    if (ht == NULL)
        return NULL;

    hash = ht->Hash(ht, data, datalen);

    if (hash >= ht->array_size) {
        return NULL;
    }

    if (ht->array[hash] == NULL)
        return NULL;

    HashTableBucket *hashbucket = ht->array[hash];
    do {
        if (ht->Compare(hashbucket->data, hashbucket->size, data, datalen) == 1)
            return hashbucket->data;

        hashbucket = hashbucket->next;
    } while (hashbucket != NULL);

    return NULL;
}

uint32_t HashTableGenericHash(HashTable *ht, void *data, uint16_t datalen)
{
     uint8_t *d = (uint8_t *)data;
     uint32_t i;
     uint32_t hash = 0;

     for (i = 0; i < datalen; i++) {
         if (i == 0)      hash += (((uint32_t)*d++));
         else if (i == 1) hash += (((uint32_t)*d++) * datalen);
         else             hash *= (((uint32_t)*d++) * i) + datalen + i;
     }

     hash *= datalen;
     hash %= ht->array_size;
     return hash;
}

char HashTableDefaultCompare(void *data1, uint16_t len1, void *data2, uint16_t len2)
{
    if (len1 != len2)
        return 0;

    if (memcmp(data1,data2,len1) != 0)
        return 0;

    return 1;
}


