#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include "linked-list.h"
#include "item.h"

typedef struct LinkedListNode {
    Item *item;
    struct LinkedListNode *next;
} LinkedListNode;

typedef struct LinkedList {
    size_t len;
    LinkedListNode *head;
    LinkedListNode *tail;
} LinkedList;

LinkedList *newList();
void push(LinkedList *list, Item *item);

#endif