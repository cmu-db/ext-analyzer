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

#include "containers.h"

/**
 * Create a doubly linked list.
 */
DLList * DLList_Create()
{
    DLList *list = RedisModule_Alloc(sizeof(DLList));
    list->size = 0;
    list->front = list->back = NULL;
    return list;
}

/**
 * Delete a doubly linked list.
 */
void DLList_Drop(DLList *list, void (*del)(DLNode *, void *), void *param)
{
    while (list->front) {
        DLNode *front = list->front;
        DLList_Delete(list, front);
        if (del) {
            del(front, param);
        }
    }
    RedisModule_Free(list);
}

/**
 * Push a node to the front of the list.
 *
 * @param list
 * @param node
 */
void DLList_Push_Front(DLList *list, DLNode *node)
{
    if (!list->front) {
        list->front = list->back = node;
        node->prev = node->next = NULL;
    } else {
        node->next = list->front;
        node->prev = NULL;
        list->front->prev = node;
        list->front = node;
    }
    list->size++;
}

/**
 * Push a node to the back of the list.
 *
 * @param list
 * @param node
 */
void DLList_Push_Back(DLList *list, DLNode *node)
{
    if (!list->back) {
        list->front = list->back = node;
        node->prev = node->next = NULL;
    } else {
        node->prev = list->back;
        node->next = NULL;
        list->back->next = node;
        list->back = node;
    }
    list->size++;
}

/**
 * Delete a node from the list.
 *
 * @param list
 * @param node
 */
void DLList_Delete(DLList *list, DLNode *node)
{
    if (node->prev) {
        node->prev->next = node->next;
    } else {
        list->front = node->next;
    }
    if (node->next) {
        node->next->prev = node->prev;
    } else {
        list->back = node->prev;
    }
    node->next = node->prev = NULL;

    list->size--;
}

/**
 * Create a doulby linked dictionary.
 */
DLDictionary * DLDictionary_Create()
{
    DLDictionary *dictionary = RedisModule_Alloc(sizeof(DLDictionary));
    dictionary->list = DLList_Create();
    dictionary->dict = RedisModule_CreateDict(NULL);
    return dictionary;
}

typedef struct DelParam
{
    void (*del)(RedisModuleString *key, void *value, void *param);
    void *param;
    RedisModuleDict *dict;
} DelParam;

/**
 * Delete a doubly linked dictionary.
 *
 * @param dictionary
 * @param del function to be called on every deleted key-value.
 * @param param to pass to del function.
 */
void DLDictionary_Drop(DLDictionary *dictionary, void (*del)(RedisModuleString *key, void *value, void *param), void *param)
{
    DLNode *node;
    while (node = dictionary->list->front) {
        DLDKeyValue *pair = node->data;
        DLDictionary_Delete(dictionary, pair->key);
        if (del) {
            del(pair->key, pair->value, param);
        }
    }
    RedisModule_Free(dictionary->list);
    RedisModule_FreeDict(NULL, dictionary->dict);
    RedisModule_Free(dictionary);
}

/**
 * Set value for a given key.
 *
 * @param dictionary
 * @param key
 * @param value
 * @param front
 * @return
 */
void * DLDictionary_Set(DLDictionary *dictionary, RedisModuleString *key, void *value, int front)
{
    DLNode *node = RedisModule_DictGet(dictionary->dict, key, NULL);
    DLDKeyValue *pair;
    if (node) {
        pair = node->data;
        void *old = pair->value;
        pair->value = value;
        return old;
    }

    pair = RedisModule_Alloc(sizeof(DLDKeyValue));
    node = RedisModule_Alloc(sizeof(DLNode));

    pair->key = key;
    pair->value = value;
    node->data = pair;

    RedisModule_DictSet(dictionary->dict, key, node);
    if (front) {
        DLList_Push_Front(dictionary->list, node);
    } else {
        DLList_Push_Back(dictionary->list, node);
    }

    return NULL;
}

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
void * DLDictionary_Set_Front(DLDictionary *dictionary, RedisModuleString *key, void *value)
{
    return DLDictionary_Set(dictionary, key, value, 1);
}

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
void * DLDictionary_Set_Back(DLDictionary *dictionary, RedisModuleString *key, void *value)
{
    return DLDictionary_Set(dictionary, key, value, 0);
}

/**
 * Get the value of a given key.
 *
 * @param dictionary
 * @param key
 * @return value
 */
void * DLDictionary_Get(DLDictionary *dictionary, RedisModuleString *key)
{
    DLNode *node = RedisModule_DictGet(dictionary->dict, key, NULL);
    if (node) {
        DLDKeyValue *pair = node->data;
        return pair->value;
    }
    return NULL;
}

/**
 * Get the key-value of the front of the dictionary.
 *
 * @param dictionary
 * @return
 */
DLDKeyValue * DLDictionary_Front(DLDictionary *dictionary)
{
    if (dictionary->list->front) {
        return dictionary->list->front->data;
    }
    return NULL;
}

/**
 * Get the key-value of the back of the dictionary.
 *
 * @param dictionary
 * @return
 */
DLDKeyValue * DLDictionary_Back(DLDictionary *dictionary)
{
    if (dictionary->list->back) {
        return dictionary->list->back->data;
    }
    return NULL;
}

/**
 * Delete a key from dictionary
 * @param dictionary
 * @param key
 * @return the corresponding value if key exists, otherwise null.
 */
void * DLDictionary_Delete(DLDictionary *dictionary, RedisModuleString *key)
{
    DLNode *node;
    if (RedisModule_DictDel(dictionary->dict, key, &node) == REDISMODULE_OK) {
        DLList_Delete(dictionary->list, node);
        DLDKeyValue *pair = node->data;
        void *value = pair->value;
        RedisModule_Free(pair);
        RedisModule_Free(node);
        return value;
    }
    return NULL;
}
