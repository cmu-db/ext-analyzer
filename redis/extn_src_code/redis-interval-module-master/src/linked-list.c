#include "linked-list.h"
#include "redismodule.h"

LinkedList *newList() {
    LinkedList *list = RedisModule_Alloc(sizeof (LinkedList));
    list->len = 0;
    list->head = NULL;
    list->tail = NULL;
    return list;
}

LinkedListNode *newListNode(Item *item) {
    LinkedListNode *node = RedisModule_Alloc(sizeof (LinkedListNode));
    node->item = item;
    node->next = NULL;
    return node;
}

void push(LinkedList *list, Item *item) {
    LinkedListNode *newNode = newListNode(item);
    if (list->head == NULL) {
        list->head = newNode;
    }
    if (list->tail == NULL) {
        list->tail = newNode;
    } else {
        list->tail->next = newNode;
        list->tail = newNode;
    }
    list->len++;
}