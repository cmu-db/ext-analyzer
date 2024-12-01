#ifndef TREE_H
#define TREE_H

#include <stdio.h>
#include <stdlib.h>
#include "interval.h"
#include "item.h"
#include "output.h"

typedef struct TreeNode {
    double val;
    double maxUpper;
    double minLower;
    long priority;
    struct TreeNode *parent;
    struct TreeNode *left;
    struct TreeNode *right;
    Item *item;
} TreeNode;

TreeNode *newTreeNode(Item *member);
void freeTreeNode(TreeNode *node);

void insertNode(TreeNode **root, Item *member);
void deleteNode(TreeNode **root, Item *toDelete);
void findContains(TreeNode *node, double value, struct RedisModuleCtx *ctx, int *len, long long count, OutputContext *outputContext);
void findOverlaps(TreeNode *node, Interval *intervalToSearch, struct RedisModuleCtx *ctx, int *len, long long count, OutputContext *outputContext);

#endif