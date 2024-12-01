/*
 * Copyright (c) 2018, Hamid Alaei Varnosfaderani
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef LARAVEL_QUEUE_CONTAINERS_H
#define LARAVEL_QUEUE_CONTAINERS_H

#include "redismodule.h"

/**
 * Doubly Linked Node
 */
typedef struct DLNode
{
    void *data;
    struct DLNode *next;
    struct DLNode *prev;
} DLNode;

/**
 * Doubly Linked List
 */
typedef struct DLList
{
    uint64_t size;
    DLNode *front;
    DLNode *back;
} DLList;

/**
 * Create a doubly linked list.
 */
DLList * DLList_Create();

/**
 * Delete a doubly linked list.
 *
 * @param list
 * @param del function to be called on every deleted node.
 * @param param to pass to del function.
 */
void DLList_Drop(DLList *list, void (*del)(DLNode *, void *), void *param);

/**
 * Push a node to the front of the list.
 *
 * @param list
 * @param node
 */
void DLList_Push_Front(DLList *list, DLNode *node);

/**
 * Push a node to the back of the list.
 *
 * @param list
 * @param node
 */
void DLList_Push_Back(DLList *list, DLNode *node);

/**
 * Delete a node from the list.
 *
 * @param list
 * @param node
 */
void DLList_Delete(DLList *list, DLNode *node);

/**
 * Double linked dictionary
 */
typedef struct DLDictionary
{
    RedisModuleDict *dict;
    DLList *list;
} DLDictionary;

/**
 * Key-value pair
 */
typedef struct DLDKeyValue
{
    RedisModuleString *key;
    void *value;
} DLDKeyValue;

/**
 * Create a doulby linked dictionary.
 */
DLDictionary * DLDictionary_Create();

/**
 * Delete a doubly linked dictionary.
 *
 * @param dictionary
 * @param del function to be called on every deleted key-value.
 * @param param to pass to del function.
 */
void DLDictionary_Drop(DLDictionary *dictionary, void (*del)(RedisModuleString *key, void *value, void *param), void *param);

/**
 * Set value for a given key.
 * If the key exists, it replaces the value.
 * If the key doesn't exists, it adds the key-value to the front of the dictionary.
 *
 * @param dictionary
 * @param key
 * @param value
 * @return null on insert, the previous value on update.
 */
void * DLDictionary_Set_Front(DLDictionary *dictionary, RedisModuleString *key, void *value);

/**
 * Set value for a given key.
 * If the key exists, it replaces the value.
 * If the key doesn't exists, it adds the key-value to the back of the dictionary.
 *
 * @param dictionary
 * @param key
 * @param value
 * @return null on insert, the previous value on update.
 */
void * DLDictionary_Set_Back(DLDictionary *dictionary, RedisModuleString *key, void *value);

/**
 * Get the value of a given key.
 *
 * @param dictionary
 * @param key
 * @return value
 */
void * DLDictionary_Get(DLDictionary *dictionary, RedisModuleString *key);

/**
 * Get the key-value of the front of the dictionary.
 *
 * @param dictionary
 * @return
 */
DLDKeyValue * DLDictionary_Front(DLDictionary *dictionary);

/**
 * Get the key-value of the back of the dictionary.
 *
 * @param dictionary
 * @return
 */
DLDKeyValue * DLDictionary_Back(DLDictionary *dictionary);

/**
 * Delete a key from dictionary
 * @param dictionary
 * @param key
 * @return the corresponding value if key exists, otherwise null.
 */
void * DLDictionary_Delete(DLDictionary *dictionary, RedisModuleString *key);

#endif //LARAVEL_QUEUE_CONTAINERS_H
