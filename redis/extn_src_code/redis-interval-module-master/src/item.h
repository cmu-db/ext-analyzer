#ifndef NODE_H
#define NODE_H

#include "interval.h"

typedef struct item {
    Interval *interval;
    char *member;
} Item;

Item *newItem(char *member, Interval *interval);
void freeItem(Item *item);

#endif